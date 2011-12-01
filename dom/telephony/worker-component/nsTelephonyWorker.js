/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Telephony.
 *
 * The Initial Developer of the Original Code is
 *   The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Turner <bent.mozilla@gmail.com> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const DEBUG = false; /* set to false to suppress debug messages */

const TELEPHONYWORKER_CONTRACTID        = "@mozilla.org/telephony/worker;1";
const TELEPHONYWORKER_CID               = Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const nsITelephonyWorker                = Components.interfaces.nsITelephonyWorker;

function onerror(evt) {
    // It is very important to call preventDefault on the event here.
    // If an exception is thrown on the worker, it bubbles out to the component
    // that created it. If that component doesn't have an onerror handler, the
    // worker will try to call the error reporter on the context it was created
    // on. However, That doesn't work for component contexts and can result in
    // crashes. This onerror handler has to make sure that it calls preventDefault
    // on the incoming event.

    evt.preventDefault();

    dump("Got an error: " + evt.filename + ":" +
         evt.lineno + ": " + evt.message + "\n");
}

function onmessage(evt) {
    dump("Received from worker: " + evt.data + "\n");
}

function nsTelephonyWorker() {
    var worker = this.worker =
        new ChromeWorker("resource://gre/modules/nsRILDecoder.js");
    worker.onerror = onerror;
    worker.onmessage = onmessage;
}

nsTelephonyWorker.prototype.classID = TELEPHONYWORKER_CID;

nsTelephonyWorker.prototype.classInfo = XPCOMUtils.generateCI({classID: TELEPHONYWORKER_CID,
                                                       contractID: TELEPHONYWORKER_CONTRACTID,
                                                       classDescription: "TelephonyWorker",
                                                       interfaces: [nsITelephonyWorker]});

nsTelephonyWorker.prototype.QueryInterface = XPCOMUtils.generateQI([nsITelephonyWorker]);

var NSGetFactory = XPCOMUtils.generateNSGetFactory([nsTelephonyWorker]);

/* static functions */
if (DEBUG)
    debug = function (s) { dump("-*- TelephonyWorker component: " + s + "\n"); }
else
    debug = function (s) {}
