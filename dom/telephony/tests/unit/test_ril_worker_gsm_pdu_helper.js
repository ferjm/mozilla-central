let onmessage;
let onRILMessage;
let onerror;

function importScripts() {}

load('ril_consts.js');
load('ril_worker.js');

function run_test() {
  run_next_test();
}

/**
 * PDU parser tests
 */
// Overwrite 'Buf' object from ril_worker.js with this test dummy.
let Buf = {
  pdu: null,
  current: null,

  init: function init(pdu) {
    this.pdu = pdu;
    this.buffer = [chr.charCodeAt() for each (chr in pdu)];
    this.current = 0;
  },

  readUint16: function readUint16() {
    if (this.current >= this.buffer.length) {
      if (DEBUG) debug("Ran out of data.");
        throw "End of buffer!";
      }
    return this.buffer[this.current++];
  },

  readString: function readString() {
    if (this.current >= this.pdu.length) {
      if (DEBUG) debug("Ran out of data.");
        throw "End of buffer!";
      }
    return this.pdu[this.current++];
  }
};

add_test(function test_real_sms() {
  let pdu = "0791889653784434040c91889678674542000011211181027023044f74380d";
  Buf.init(pdu);
  let parsed = GsmPDUHelper.readMessage();
  do_check_neq(parsed, null);
  do_check_eq(parsed.SMSC, "+886935874443");
  do_check_eq(parsed.sender, "+886987765424");
  do_check_eq(parsed.body, "Ohai");
  do_check_eq(parsed.timestamp, 1323566346000);
  run_next_test();
});
