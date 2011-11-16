/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
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

#include "Radio.h"
#include "nsITelephonyWorker.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"

#include "nsThreadUtils.h"

static NS_DEFINE_CID(kTelephonyWorkerCID, NS_TELEPHONYWORKER_CID);

// Topic we listen to for shutdown.
#define PROFILE_BEFORE_CHANGE_TOPIC "profile-before-change"

USING_TELEPHONY_NAMESPACE

namespace {

// Doesn't carry a reference, we're owned by services.
Radio* gInstance = nsnull;

JSBool
ReceiveMessage(JSContext *cx, uintN argc, jsval *vp)
{
  jsval *argv = JS_ARGV(cx, vp);

  JS_ASSERT(argc == 1);
  JSObject *eventobj = JSVAL_TO_OBJECT(argv[0]);

  jsval v;
  if (!JS_GetProperty(cx, eventobj, "data", &v)) {
    return false;
  }

  JSAutoByteString abs(cx, JSVAL_TO_STRING(v));
  printf("Received from worker: %s\n", abs.ptr());
  return true;
}

JSBool
HandleError(JSContext *cx, uintN argc, jsval *vp)
{
  jsval *argv = JS_ARGV(cx, vp);

  JS_ASSERT(argc == 1);
  JSObject *eventobj = JSVAL_TO_OBJECT(argv[0]);

  jsval callee_argv;
  if (!JS_CallFunctionName(cx, eventobj, "preventDefault", 0, &callee_argv,
                           &callee_argv)) {
    return false;
  }

  printf("Got an error!\n");
  return true;
}

nsresult
SetHandler(JSContext *cx, JSObject *workerobj, JSNative native, const char *name)
{
  JSFunction *fun = JS_NewFunction(cx, native, 1, 0,
                                   JS_GetGlobalForObject(cx, workerobj), name);
  if (!fun) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  jsval v = OBJECT_TO_JSVAL(JS_GetFunctionObject(fun));
  if (!JS_SetProperty(cx, workerobj, name, &v)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

} // anonymous namespace

Radio::Radio()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "There should only be one instance!");
}

Radio::~Radio()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance || gInstance == this,
               "There should only be one instance!");
  gInstance = nsnull;
}

nsresult
Radio::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "We can only initialize on the main thread");

  nsCOMPtr<nsITelephonyWorker> worker(do_CreateInstance(kTelephonyWorkerCID));
  if (!worker) {
    return NS_ERROR_FAILURE;
  }

  jsval workerval;
  nsresult rv = worker->GetWorker(&workerval);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!JSVAL_IS_PRIMITIVE(workerval), "bad worker value");
  JSContext *cx;
  rv = nsContentUtils::ThreadJSContextStack()->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!cx) {
    return NS_ERROR_FAILURE;
  }

  JSObject *workerobj = JSVAL_TO_OBJECT(workerval);
  rv = nsContentUtils::XPConnect()->HoldObject(cx, workerobj,
                                               getter_AddRefs(mWorker));
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, workerobj)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = SetHandler(cx, workerobj, HandleError, "onerror");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetHandler(cx, workerobj, ReceiveMessage, "onmessage");
  NS_ENSURE_SUCCESS(rv, rv);

  // Poke!
  jsval argv = JSVAL_VOID;
  if (!JS_CallFunctionName(cx, workerobj, "postMessage", 1, &argv, &argv)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

// static
already_AddRefed<Radio>
Radio::FactoryCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<Radio> instance(gInstance);

  if (!instance) {
    instance = new Radio();
    if (NS_FAILED(instance->Init())) {
      return nsnull;
    }

    gInstance = instance;
  }

  return instance.forget();
}

NS_IMPL_ISUPPORTS_INHERITED0(Radio, RadioBase)
