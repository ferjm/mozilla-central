importScripts("ril_worker.js");

/* TODO Fill me in! */
onmessage = function () {
  
}

function DoRIL(evt) {
  Buf.processIncoming(evt.data);

  // var asChars = [String.fromCharCode(evt.data[i]) for (i in evt.data)];
  // dump(asChars.toString());
  //postMessage(asChars.toString());
  //postRILMessage(terminate(asChars.join('').toUpperCase()));
}

addEventListener("RILMessageEvent", DoRIL, false, false);
