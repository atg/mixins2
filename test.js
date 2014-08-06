var choc = require('./index');
/*
var t0 = new Date().getTime();
var n = 10000;
for (var i = 0; i < n; i++) {
    choc.objc_msgSend('controller', 'doSomething:', [1, 2, 3], {});
}
var t1 = new Date().getTime();
var t = t1 - t0;
t = t / n;
console.log(String(t));
*/

var x = choc.objc_msgSend('controller', 'doSomething:', [1, 2, 3], {});
console.log(x);

