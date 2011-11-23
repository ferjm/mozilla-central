/* TODO Fill me in! */
onmessage = function () {
    postRILMessage("Hello off the main thread");
    setTimeout(function() { postMessage("test passed"); }, 10);
}

function DoRIL(evt) {
    var asChars = [String.fromCharCode(evt.data[i]) for (i in evt.data)];
    postMessage(asChars.toString(''));
}

addEventListener("RILMessageEvent", DoRIL, false, false);
