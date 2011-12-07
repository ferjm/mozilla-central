/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */

"use strict";

importScripts("wifi_library.js");

var buf = ctypes.char.array(4096);
var len = ctypes.size_t();

function onmessage(e) {
  var data = e.data;
  var cmd = data.cmd;
  var args = data.args;

  if (cmd == "command") { // command(command, max-reply-length)
    len.value = 4096;
    var ret = wifi.command(args[0], buf, len.ptr);
    var result = "";
    if (!ret) {
      result = buf.readString().substr(len.value);
    }

  } else {
    var ret = wifi[cmd].apply(wifi, args);
    e.source.postMessage({ wifi_status: ret });
  }
}

dump("wifi started, yo!\n");
postMessage("I'm working");
