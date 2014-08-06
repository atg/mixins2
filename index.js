/*
* Copyright (c) 2014, Nicholas Penree <nick@penree.com>
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

var bindings = require('bindings')('bindings');
var crypto = require('crypto');
var chocolat_sendJSON = bindings.chocolat_sendJSON;
var callbacks = {};

function random_key() {
    var buf = crypto.randomBytes(16);
    return buf.toString('base64');
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
    callbacks[key] = function() {
        delete callbacks[key];
        cb.apply({}, arguments);
    };
    return key;
}

function unregister(key) {
    delete callbacks[key];
}

function objc_msgSend(receiver, selector, args, options) {
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

    var jsonIn = JSON.stringify(obj);
    var jsonStrOut = chocolat_sendJSON(jsonIn);
    var jsonOut = JSON.parse(jsonStrOut);

    return jsonOut;
}

exports.objc_msgSend = objc_msgSend;
exports.register_oneshot = register_oneshot;
exports.register_reusable = register_reusable;
exports.unregister = unregister;
