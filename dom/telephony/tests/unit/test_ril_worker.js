let onmessage;
let onRILMessage;
let onerror;

function postRILMessage() {
}

function importScripts() {}

/**
 * Fake APIs for sending messages to the worker.
 */
let worker = {
  postRILMessage: function postRILMessage(message) {
    onRILMessage(data);
  },
  postMessage: function postMessage(message) {
    let event = {type: "message",
                 data: message};
    onmessage(event);
  }
};

load('ril_consts.js');
load('ril_worker.js');

function run_test() {
  run_next_test();
}

add_test(function test_buf_process_incoming() {
  let test_parcel = [0,0,0,12,1,0,0,0,232,3,0,0,0,0,0,0];
  Buf.processIncoming(test_parcel);
  do_check_eq(Buf.incomingReadIndex, 16);
  do_check_eq(Buf.incomingWriteIndex, 16);
  for (let i = 0; i < test_parcel.length; i++) {
    do_check_eq(Buf.incomingBytes[i], test_parcel[i]);
  }
  run_next_test();
});
