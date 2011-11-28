/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
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
#include "nsIObserverService.h"
#include "mozilla/dom/workers/Workers.h"

#include "nsThreadUtils.h"

USING_WORKERS_NAMESPACE
using namespace mozilla::ipc;

static NS_DEFINE_CID(kTelephonyWorkerCID, NS_TELEPHONYWORKER_CID);

// Topic we listen to for shutdown.
#define PROFILE_BEFORE_CHANGE_TOPIC "profile-before-change"

USING_TELEPHONY_NAMESPACE

namespace {

// Doesn't carry a reference, we're owned by services.
Radio* gInstance = nsnull;

// Called when the worker wants to talk to the DOM.
JSBool
ReceiveMessage(JSContext *cx, uintN argc, jsval *vp)
{
  NS_ASSERTION(NS_IsMainThread(), "postMessage posts to the main thread");
  jsval *argv = JS_ARGV(cx, vp);

  JS_ASSERT(argc == 1);
  JSObject *eventobj = JSVAL_TO_OBJECT(argv[0]);

  jsval v;
  if (!JS_GetProperty(cx, eventobj, "data", &v)) {
    return false;
  }

  // XXX Need to figure out what the protocol looks like here. Since we're doing
  // this in C++, it'll probably be something like "an object with a given
  // property specifying the type of event and another property with data about
  // the event.
  JSAutoByteString abs(cx, JSVAL_TO_STRING(v));
  printf("Received from worker: %s\n", abs.ptr());
  return true;
}

// Called when the worker throws an exception. This should never happen.
// For now printf the exception. It might be worth throwing something up on the
// developer console, though.
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

  jsval filenameval;
  jsval messageval;
  jsval linenoval;
  if (!JS_GetProperty(cx, eventobj, "filename", &filenameval) ||
      !JS_GetProperty(cx, eventobj, "lineno", &linenoval) ||
      !JS_GetProperty(cx, eventobj, "message", &messageval)) {
    return false;
  }

  // message must be a string.
  JSAutoByteString filenameabs(cx, JSVAL_TO_STRING(filenameval));
  JSAutoByteString messageabs(cx, JSVAL_TO_STRING(messageval));
  printf("Got an error: %s:%d: %s\n",
         filenameabs.ptr(), JSVAL_TO_INT(linenoval), messageabs.ptr());
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

class ConnectWorkerToRIL : public WorkerTask {
public:
  virtual void RunTask(JSContext *aCx);
};

JSBool
PostToRIL(JSContext *cx, uintN argc, jsval *vp)
{
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");

  if (argc != 1) {
    JS_ReportError(cx, "Expecting a single argument with the RIL message");
    return false;
  }

  jsval v = JS_ARGV(cx, vp)[0];

  nsAutoPtr<RilMessage> rm(new RilMessage());
  if (JSVAL_IS_STRING(v)) {
    JSString *str = JSVAL_TO_STRING(v);
    JSAutoByteString abs(cx, str);
    if (!abs.ptr()) {
      return false;
    }

    rm->mSize = JS_GetStringLength(str);
    memcpy(rm->mData, abs.ptr(), rm->mSize);
  } else {
    // TODO Deal with typed arrays.
    JS_ReportError(cx, "TODO typed arrays not yet handled.");
  }

  RilMessage *tosend = rm.forget();
  JS_ALWAYS_TRUE(SendRilMessage(&tosend));
  return true;
}

void
ConnectWorkerToRIL::RunTask(JSContext *aCx)
{
  // Set up the postRILMessage on the function for worker -> RIL thread
  // communication.
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");
  NS_ASSERTION(!JS_IsRunning(aCx), "Are we being called somehow?");
  JSObject *workerGlobal = JS_GetGlobalObject(aCx);

  JSAutoRequest ar(aCx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(aCx, workerGlobal)) {
    return;
  }

  if (!JS_DefineFunction(aCx, workerGlobal, "postRILMessage", PostToRIL, 1, 0)) {
    return;
  }
}

class RILReceiver : public RilConsumer
{
public:
  RILReceiver(WorkerCrossThreadDispatcher *aDispatcher)
    : mDispatcher(aDispatcher)
  { }

  virtual void MessageReceived(RilMessage *aMessage) {
    mDispatcher->DispatchRILEvent(aMessage->mData, aMessage->mSize);
  }

private:
  nsRefPtr<WorkerCrossThreadDispatcher> mDispatcher;
};

} // anonymous namespace

Radio::Radio()
  : mShutdown(false)
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

  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (!obs) {
    NS_WARNING("Failed to get observer service!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = obs->AddObserver(this, PROFILE_BEFORE_CHANGE_TOPIC, false);
  NS_ENSURE_SUCCESS(rv, rv);

  // The telephony worker component is a hack that gives us a global object for
  // our own functions and makes creating the worker possible.
  nsCOMPtr<nsITelephonyWorker> worker(do_CreateInstance(kTelephonyWorkerCID));
  if (!worker) {
    return NS_ERROR_FAILURE;
  }

  jsval workerval;
  rv = worker->GetWorker(&workerval);
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

  WorkerCrossThreadDispatcher *wctd = GetWorkerCrossThreadDispatcher(cx, workerval);
  if (!wctd) {
    return NS_ERROR_FAILURE;
  }

  // If an exception is thrown on the worker, it bubbles out to the component
  // that created it. If that component doesn't have an onerror handler, the
  // worker will try to call the error reporter on the context it was created
  // on. However, That doesn't work for component contexts and can result in
  // crashes. This onerror handler has to make sure that it calls preventDefault
  // on the incoming event.
  rv = SetHandler(cx, workerobj, HandleError, "onerror");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetHandler(cx, workerobj, ReceiveMessage, "onmessage");
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<ConnectWorkerToRIL> connection = new ConnectWorkerToRIL();
  if (!wctd->PostTask(connection)) {
    return NS_ERROR_UNEXPECTED;
  }

  // Now that we're set up, connect ourselves to the RIL thread.
  mozilla::RefPtr<RILReceiver> receiver = new RILReceiver(wctd);
  StartRil(receiver);

  return NS_OK;
}

void
Radio::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  StopRil();
  mWorker = nsnull;

  mShutdown = true;
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

NS_IMPL_ISUPPORTS1(Radio, nsIObserver)

NS_IMETHODIMP
Radio::Observe(nsISupports* aSubject, const char* aTopic,
               const PRUnichar* aData)
{
  if (!strcmp(aTopic, PROFILE_BEFORE_CHANGE_TOPIC)) {
    Shutdown();

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (obs) {
      if (NS_FAILED(obs->RemoveObserver(this, aTopic))) {
        NS_WARNING("Failed to remove observer!");
      }
    }
    else {
      NS_WARNING("Failed to get observer service!");
    }
  }

  return NS_OK;
}
