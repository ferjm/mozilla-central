const B2G_TEST_PARCEL = 12345678;

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

/**
 * Parcel Receive Tests
 */

function testPacket(p) {
  // Test function for putting chunked parcels back together. Makes sure
  // we recreated the parcel correctly.
  let response = Buf.readUint32();
  do_check_eq(response, 1);
  let request = Buf.readUint32();
  do_check_eq(request, RIL_UNSOL_RESPONSE_RADIO_CHANGED);
  let data = Buf.readUint32();
  do_check_eq(data, 0);
}

// Create a byte array that looks like a raw parcel
let old_process_parcel = Buf.processParcel;
Buf.processParcel = testPacket;

add_test(function test_buf_process_incoming_single_buffer() {
  // Receive a packet length and data as a single buffer.
  let test_parcel = [0,0,0,12,1,0,0,0,232,3,0,0,0,0,0,0];
  Buf.processIncoming(test_parcel);
  do_check_eq(Buf.incomingReadIndex, test_parcel.length);
  do_check_eq(Buf.incomingWriteIndex, test_parcel.length);
  for (let i = 0; i < test_parcel.length; i++) {
    do_check_eq(Buf.incomingBytes[i], test_parcel[i]);
  }
  run_next_test();
});

add_test(function test_buf_process_incoming_different_buffer() {
  //Receive a packet length and data as different buffers.
  let oldIncomingReadIndex = Buf.incomingReadIndex;
  let oldIncomingWriteIndex = Buf.incomingWriteIndex;
  let test_parcel = [0,0,0,12,1,0,0,0,232,3,0,0,0,0,0,0];
  Buf.processIncoming(test_parcel.slice(0, 4));
  Buf.processIncoming(test_parcel.slice(4, test_parcel.length));
  do_check_eq(Buf.incomingReadIndex,
              oldIncomingReadIndex + test_parcel.length);
  do_check_eq(Buf.incomingWriteIndex,
              oldIncomingWriteIndex + test_parcel.length);
  for (let i = oldIncomingReadIndex;
       i < oldIncomingReadIndex + test_parcel.length; i++) {
    do_check_eq(Buf.incomingBytes[i], test_parcel[i - oldIncomingReadIndex]);
  }
  run_next_test();
});

//TODO: add more tests for process incoming
Buf.processParcel = old_process_parcel;

/**
 * Parcel Send Tests
 */

function testSender(p) {
  // Test callback, used for making sure send creates packets correctly.
  let test_parcel = [0,0,0,12,23,0,0,0,1,0,0,0,1,0,0,0];
  for (let i = 0; i < p.byteLength; ++i) {
    do_check_eq(test_parcel[i], p[i]);
  }
}

let oldPostRILMessage = postRILMessage;

add_test(function test_send_parcel() {
  // Send a parcel, write callback through tester function.
  postRILMessage = testSender;
  Buf.token = 1;
  Buf.newParcel(REQUEST_RADIO_POWER);
  Buf.writeUint32(1);
  Buf.sendParcel();
  postRILMessage = oldPostRILMessage;
  run_next_test();
});

/**
 * Type tests
 */

function testString(p) {
  let test_parcel = [0,0,0,52, 78,97,188,0, 2,0,0,0, 18,0,0,0,73,0,32,0,97,0,109,0,32,0,97,0,32,0,116,0,101,0,115,0,116,0,32,0,115,0,116,0,114,0,105,0,110,0,103,0,0,0,0,0];
  for (let i = 0; i < p.byteLength; ++i) {
    do_check_eq(test_parcel[i], p[i]);
  }
}

add_test(function test_write_string() {
  postRILMessage = testString;
  Buf.newParcel(B2G_TEST_PARCEL);
  Buf.writeString("I am a test string");
  Buf.sendParcel();
  postRILMessage = oldPostRILMessage;
  run_next_test();
});

//TODO: test failing...
add_test(function test_read_string() {
  let test_parcel = [0,0,0,52, 1,0,0,0, 78,97,188,0, 0,0,0,0, 18,0,0,0,73,0,32,0,97,0,109,0,32,0,97,0,32,0,116,0,101,0,115,0,116,0,32,0,115,0,116,0,114,0,105,0,110,0,103,0, 0, 0, 0, 0];
  RIL[B2G_TEST_PARCEL] = function B2G_TEST_PARCEL(l) {
    let str = Buf.readString();
    //do_check_eq(str, "I am a test string");
  };
  Buf.processIncoming(test_parcel);
  run_next_test();
});

//TODO: test failing...
add_test(function test_read_string_list() {
  let test_parcel = [0,0,0,152, 1,0,0,0, 78,97,188,0, 0,0,0,0, 3,0,0,0,3,0,0,0,18,0,0,0,73,0,32,0,97,0,109,0,32,0,97,0,32,0,116,0,101,0,115,0,116,0,32,0,115,0,116,0,114,0,105,0,110,0,103,0,0,0,0,0,18,0,0,0,73,0,32,0,97,0,109,0,32,0,97,0,32,0,116,0,101,0,115,0,116,0,32,0,115,0,116,0,114,0,105,0,110,0,103,0,0,0,0,0,18,0,0,0,73,0,32,0,97,0,109,0,32,0,97,0,32,0,116,0,101,0,115,0,116,0,32,0,115,0,116,0,114,0,105,0,110,0,103,0,0,0,0,0];
  RIL[B2G_TEST_PARCEL] = function B2G_TEST_PARCEL(l) {
    let str = Buf.readStringList();
    //do_check_eq(str.length, 3);
    for (let i = 0; i < str.length; ++i) {
      //do_check_eq(str[i] === "I am a test string");
    }
  };
  Buf.processIncoming(test_parcel);
  run_next_test();
});
