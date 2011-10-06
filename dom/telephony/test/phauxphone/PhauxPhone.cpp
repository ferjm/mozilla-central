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

#include "PhauxPhone.h"

#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonySession.h"

#include "nsIClassInfoImpl.h"
#include "nsMemory.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"

#define NOTIFY_OBSERVERS(func_, params_) \
  PR_BEGIN_MACRO                                                               \
    nsAutoTObserverArray<nsCOMPtr<nsIRadioCallback>, 1>::ForwardIterator       \
      iter_(mCallbacks);                                                       \
    while (iter_.HasMore()) {                                                  \
      nsCOMPtr<nsIRadioCallback> obs_ = iter_.GetNext();                       \
      obs_ -> func_ params_ ;                                                  \
    }                                                                          \
  PR_END_MACRO


USING_TELEPHONY_NAMESPACE
using namespace mozilla::dom::telephony::test;

namespace {

// Doesn't carry a reference, we're owned by the service manager.
PhauxPhone* gInstance = nsnull;

class ResponseRunnable : public nsIRunnable
{
  nsRefPtr<PhauxPhone> mPhauxPhone;
  PRUint64 mToken;
  nsresult mErrorCode;
  void* mData;
  size_t mDataLen;

public:
  NS_DECL_ISUPPORTS

  ResponseRunnable(PhauxPhone* aPhauxPhone, PRUint64 aToken,
                   nsresult aErrorCode = NS_OK, void* aData = nsnull,
                   size_t aDataLen = 0)
  : mPhauxPhone(aPhauxPhone), mToken(aToken), mErrorCode(aErrorCode),
    mData(aData), mDataLen(aDataLen)
  { }

  NS_IMETHOD
  Run()
  {
    PhauxPhone::AssertIsOnMainThread();
    mPhauxPhone->SimulateResponse(mToken, mErrorCode, mData, mDataLen);
    return NS_OK;
  }
};

// Not threadsafe because we never make a separate thread.
NS_IMPL_ISUPPORTS1(ResponseRunnable, nsIRunnable)

} // anonymous namespace

PhauxPhone::PhauxPhone()
{
  AssertIsOnMainThread();
  NS_ASSERTION(!gInstance, "There should only be one instance!");
}

PhauxPhone::~PhauxPhone()
{
  AssertIsOnMainThread();
  NS_ASSERTION(!gInstance || gInstance == this,
               "There should only be one instance!");
  gInstance = nsnull;
}

// static
already_AddRefed<PhauxPhone>
PhauxPhone::FactoryCreate()
{
  AssertIsOnMainThread();

  nsRefPtr<PhauxPhone> instance(gInstance);

  if (!instance) {
    instance = new PhauxPhone();
    if (NS_FAILED(instance->Init())) {
      return nsnull;
    }

    gInstance = instance;
  }

  return instance.forget();
}

void
PhauxPhone::SimulateResponse(PRUint64 aToken, nsresult aErrorCode, void* aData,
                             size_t aDataLen)
{
  AssertIsOnMainThread();
  NOTIFY_OBSERVERS(OnResponse, (this, aToken, aErrorCode, aData, aDataLen));
}

NS_IMPL_CLASSINFO(PhauxPhone, nsnull,
                  nsIClassInfo::SINGLETON | nsIClassInfo::MAIN_THREAD_ONLY |
                  nsIClassInfo::DOM_OBJECT,
                  PHAUXPHONE_CID)

NS_IMPL_CI_INTERFACE_GETTER1(PhauxPhone, nsIPhauxPhone)

NS_IMPL_ADDREF_INHERITED(PhauxPhone, RadioBase)
NS_IMPL_RELEASE_INHERITED(PhauxPhone, RadioBase)

NS_INTERFACE_MAP_BEGIN(PhauxPhone)
  NS_INTERFACE_MAP_ENTRY(nsIPhauxPhone)
  NS_IMPL_QUERY_CLASSINFO(PhauxPhone)
NS_INTERFACE_MAP_END_INHERITING(RadioBase)

nsresult
PhauxPhone::MakeRequest(PRUint64 aToken, PRUint64 aRequest, void* aData,
                        size_t aDataLen)
{
  AssertIsOnMainThread();

  nsresult rv = NS_OK;
  nsRefPtr<ResponseRunnable> response;

  if (aRequest == nsIRadio::REQUEST_DIAL) {
    response = new ResponseRunnable(this, aToken);

    NS_ASSERTION(aDataLen == sizeof(nsIRadio::REQUEST_DIAL_Args), "Bad size!");
    nsIRadio::REQUEST_DIAL_Args* data =
      static_cast<nsIRadio::REQUEST_DIAL_Args*>(aData);
    delete data;
  }
  else if (aRequest == nsIRadio::REQUEST_HANGUP) {
    response = new ResponseRunnable(this, aToken);

    NS_ASSERTION(aDataLen == sizeof(nsIRadio::REQUEST_HANGUP_Args), "Bad size!");
    nsIRadio::REQUEST_HANGUP_Args* data =
      static_cast<nsIRadio::REQUEST_HANGUP_Args*>(aData);
    delete data;
  }
  else {
    NS_NOTREACHED("Unknown request!");
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  if (response && NS_SUCCEEDED(rv)) {
    rv = NS_DispatchToCurrentThread(response);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

NS_IMETHODIMP
PhauxPhone::SimulateIncomingCall(nsIDOMTelephony* aTelephony,
                                 const nsAString& aNumber)
{
  AssertIsOnMainThread();

  NS_ENSURE_ARG_POINTER(aTelephony);
  NS_ENSURE_ARG(!aNumber.IsEmpty());

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
PhauxPhone::SimulateOutgoingCall(nsIDOMTelephonySession* aSession, bool aBusy)
{
  AssertIsOnMainThread();

  NS_ENSURE_ARG_POINTER(aSession);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
PhauxPhone::SimulateHangup(nsIDOMTelephonySession* aSession)
{
  AssertIsOnMainThread();

  NS_ENSURE_ARG_POINTER(aSession);

  return NS_ERROR_NOT_IMPLEMENTED;
}
