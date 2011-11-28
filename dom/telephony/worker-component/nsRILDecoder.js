/* TODO Fill me in! */
onmessage = function () {
    postRILMessage("Hello off the main thread");
    setTimeout(function() { postMessage("test passed"); }, 10);
}

function terminate(s) {
    return s[s.length - 1] == '\n' ? s : s + '\n';
}

function DoRIL(evt) {
    var asChars = [String.fromCharCode(evt.data[i]) for (i in evt.data)];
    postMessage(asChars.toString());
    postRILMessage(terminate(asChars.join('').toUpperCase()));
}

addEventListener("RILMessageEvent", DoRIL, false, false);
