/* TODO Fill me in! */
onmessage = function () {
    postRILMessage("Hello off the main thread");
    setTimeout(function() { postMessage("test passed"); }, 10);
}

function DoRIL(evt) {
    postMessage(Array.prototype.join.call([String.fromCharCode(i) for each (i in evt.data)], ","));
}

addEventListener("RILMessageEvent", DoRIL, false, false);
