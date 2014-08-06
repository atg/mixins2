
var mixinPath = process.argv[2];
var apiPath = process.argv[3];
var nativeSupportPath = process.argv[4];
global.underscore_path = process.argv[5];
var nodeID = process.argv[6];

console.log("I'm a mixin!");
console.log(mixinPath);
console.log(apiPath);
console.log(nativeSupportPath);
console.log(nodeID);
console.log([mixinPath, apiPath, nativeSupportPath, global.underscore_path, nodeID]);

var bindings = require('bindings')('bindings');
var crypto = require('crypto');
var chocolat_sendJSON = bindings.chocolat_sendJSON;
var callbacks = {};

function random_key(isOneshot) {
    // NODDY$$Function$$<type>$$<id>@@<mixinid>
    var type = isOneshot ? "oneshot" : "reusable";
    var buf = crypto.randomBytes(16);
    var randomness = buf.toString('base64');
    
    return "NODDY$$Function$$" + type + "$$" + randomness + "@@" + nodeID;
}

function register_oneshot(cb) {
    var key = random_key();
    callbacks[key] = function() {
        delete callbacks[key];
        cb.apply({}, arguments);
    };
    return key;
}

function register_reusable(cb) {
    var key = random_key();
    callbacks[key] = cb;
    return key;
}

function unregister(key) {
    delete callbacks[key];
}

function processJSON(k, v) {
    // Translate things like undefined to "NODDY$$undefined"
    if (v === undefined)
        return "NODDY$$undefined";
    if (typeof(v) == "function")
        return register_oneshot(v);
    return v;
}
function unprocessJSON(k, v) {
    // Translate things like "NODDY$$undefined" to undefined
    if (v === "NODDY$$undefined")
        return undefined;
    return v;
}

global.choc_register_oneshot = register_oneshot;
global.choc_register_reusable = register_reusable;
global.choc_unregister = unregister;


process.on('disconnect', function() {
    console.log('parent exited');
    process.exit();
});
/*
setInterval(function() {
    if (bindings.getppid() < 2) {
        process.exit();
    }
}, 3000);
*/

console.log(bindings);
console.log(chocolat_sendJSON);

bindings.chocolat_registerFunctionHandler(function(jsonText) {
    var json = JSON.parse(jsonText, unprocessJSON);
    var func = callbacks[json.key];
    if (func === undefined)
        return undefined;
    json = func.apply({}, json.args);
    return JSON.stringify({ "value": json }, processJSON);
});


var path = require('path');
var fs = require('fs');

// Require native support
var nativesupport = require(nativeSupportPath + "/index.js");

// This line is important! It sets up the globals for the api
require(apiPath);

// Temporary way to stop node.js quitting immediately until we add an events.EventEmitter
process.stdin.resume();

// Keep the event loop running until uv_async is less buggy
// setInterval(function () {}, 250);



function objc_msgSendCore(receiver, selector, args, options) {
    var obj = {
        "receiver": receiver,
        "selector": selector,
        "args": args,
    };

    for (var key in options) {
        if (options.hasOwnProperty(key)) {
            obj[key] = options[key];
        }
    }

    var jsonIn = JSON.stringify(obj, processJSON);
    console.log("Trying to send JSON");
    console.log(jsonIn);
    var jsonStrOut = chocolat_sendJSON(jsonIn);
    var jsonOut = JSON.parse(jsonStrOut, unprocessJSON);
    
    return jsonOut["value"];
}
function objc_msgSendShim() {
    var r = arguments[0];
    var s = arguments[1];
    var args = Array.prototype.slice.call(arguments, 2); //new Array(arguments.length);
//    for (var i = 0; i < arguments.length; i++) {
//        args[i] = arguments[i]; //.push(arguments[i]);
//    }
    objc_msgSendCore(r, s, args, this);
}


global.objc_msgSend = function() {
    objc_msgSendShim.apply({ sync: false }, arguments);
};
global.objc_msgSendSync = function() {
    return objc_msgSendShim.apply({ sync: true }, arguments);
};
global.objc_msgSendThread = function() {
    return objc_msgSendShim.apply({ sync: true, thread: true }, arguments);
};

global.call_function_as = function(mixinname, func, args) {
    try {
        global.current_mixin_name = mixinname;
        func.apply({}, args);
    }
    finally {
        global.current_mixin_name = null;
    }
};

// Read the source from disk
var pathToMain = path.join(mixinPath, "init.js");
var pathToPackageJson = path.join(mixinPath, "package.json");

// Try to use package.json if it exists
if (fs.existsSync(pathToPackageJson)) {
    try {
        var json = JSON.parse(fs.readFileSync(pathToPackageJson, "utf8"));
        if (typeof json !== "undefined" && typeof json.main !== "undefined") {
            pathToMain = path.join(mixinPath, json.main);
        }
    } catch(e) {
        // Parsing failed? Looks like we are using init.js
    }
}

// Remove from the module cache
var keys = Object.keys(require.cache);
for (var i = 0; i < keys.length; i++) {
    var key = keys[i];
    delete require.cache[key];
}

// Run the code!
try {
    if (fs.existsSync(pathToMain)) {
        require(pathToMain);
    }
}
finally {
    // Remember, the above line may throw, so put anything else here
}
