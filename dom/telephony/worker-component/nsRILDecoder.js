importScripts("ril_worker.js");

/* TODO Fill me in! */
onmessage = function () {};

function onRILMessage(data) {
  Buf.processIncoming(data);
}
