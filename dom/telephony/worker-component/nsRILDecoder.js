/* TODO Fill me in! */
onmessage = function () {
    postRILMessage("Hello off the main thread");
    setTimeout(function() { postMessage("test passed"); }, 10);
}
