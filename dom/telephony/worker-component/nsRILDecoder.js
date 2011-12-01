/* TODO Fill me in! */
onmessage = function () {
    postRILMessage("Hello off the main thread");
    setTimeout(function() { postMessage("test passed"); }, 10);
}

function terminate(s) {
    return s[s.length - 1] == '\n' ? s : s + '\n';
}

function onRILMessage(data) {
    var asChars = [String.fromCharCode(data[i]) for (i in data)];
    postMessage(asChars.toString());
    postRILMessage(terminate(asChars.join('').toUpperCase()));
}
