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

#include "Telephone.h"

#include "nsIRadio.h"

#include "nsThreadUtils.h"

#include "TelephoneCall.h"

#define NOTIFY_OBSERVERS(func_, params_) \
  PR_BEGIN_MACRO                                                               \
    nsAutoTObserverArray<nsCOMPtr<nsITelephoneCallback>, 1>::ForwardIterator   \
      iter_(mCallbacks);                                                       \
    while (iter_.HasMore()) {                                                  \
      nsCOMPtr<nsITelephoneCallback> obs_ = iter_.GetNext();                   \
      obs_ -> func_ params_ ;                                                  \
    }                                                                          \
  PR_END_MACRO

USING_TELEPHONY_NAMESPACE

Telephone::Telephone()
: mNextRequestToken(0), mShutdown(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

Telephone::~Telephone()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

// static
already_AddRefed<Telephone>
Telephone::Create(nsIRadio* aRadio)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aRadio, "Null pointer!");

  nsRefPtr<Telephone> telephone = new Telephone();

  nsresult rv = aRadio->RegisterCallback(telephone);
  NS_ENSURE_SUCCESS(rv, nsnull);

  telephone->mRadio = aRadio;

  return telephone.forget();
}

nsresult
Telephone::DialConcrete(const nsAString& aPhoneNumber, TelephoneCall** aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG(!aPhoneNumber.IsEmpty());
  NS_ENSURE_ARG_POINTER(aResult);

  PRUint64 token = GetNewRequestToken();

  nsAutoPtr<nsIRadio::REQUEST_DIAL_Args> args(new nsIRadio::REQUEST_DIAL_Args());
  args->mPhoneNumber = aPhoneNumber;

  nsresult rv = mRadio->MakeRequest(token, nsIRadio::REQUEST_DIAL, args,
                                    sizeof(*(args.get())));
  NS_ENSURE_SUCCESS(rv, rv);

  args.forget();

  nsRefPtr<TelephoneCall> call = TelephoneCall::Create(this, aPhoneNumber);
  NS_ASSERTION(call, "This should never fail!");

  RequestInfo* info = mLiveRequests.AppendElement();

  info->mToken = token;
  info->mRequest = nsIRadio::REQUEST_DIAL;
  info->mAssociatedCall = call;

  call.forget(aResult);
  return NS_OK;
}

nsresult
Telephone::HangUp(TelephoneCall* aCall)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aCall);

  PRUint64 token = GetNewRequestToken();

  nsAutoPtr<nsIRadio::REQUEST_HANGUP_Args> args(new nsIRadio::REQUEST_HANGUP_Args());
  args->mId = aCall->GetId();

  nsresult rv = mRadio->MakeRequest(token, nsIRadio::REQUEST_HANGUP, args,
                                    sizeof(*(args.get())));
  NS_ENSURE_SUCCESS(rv, rv);

  args.forget();

  RequestInfo* info = mLiveRequests.AppendElement();

  info->mToken = token;
  info->mRequest = nsIRadio::REQUEST_HANGUP;
  info->mAssociatedCall = aCall;

  return NS_OK;
}

void
Telephone::OnNewCall(TelephoneCall* aCall)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCall, "Null pointer!");
  NS_ASSERTION(!mLiveCalls.Contains(aCall), "Impossible!");
  mLiveCalls.AppendElement(aCall);
}

void
Telephone::OnDyingCall(TelephoneCall* aCall)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCall, "Null pointer!");
  NS_ASSERTION(mLiveCalls.Contains(aCall), "Impossible!");

  mLiveCalls.RemoveElement(aCall);

  for (PRUint32 index = 0; index < mLiveRequests.Length(); ) {
    if (mLiveRequests[index].mAssociatedCall == aCall) {
      mLiveRequests.RemoveElementAt(index);
    }
    else {
      index++;
    }
  }
}

void
Telephone::HandleResponse(nsIRadio* aRadio, PRUint64 aRequest,
                          TelephoneCall* aCall, nsresult aErrorCode,
                          void* aData, size_t aDataLen)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (NS_FAILED(aErrorCode)) {
    NS_WARNING("Got a failure code!");
    return;
  }

  NS_ASSERTION(!aCall || mLiveCalls.Contains(aCall), "Dead call!");

  nsRefPtr<TelephoneCall> kungFuDeathGrip(aCall);

  if (aRequest == nsIRadio::REQUEST_DIAL) {
    NS_ASSERTION(aCall, "Must have a call!");

    // XXX Figure out the id.
    aCall->SetId(1);

    aCall->ChangeState(nsITelephoneCall::STATE_DIALING);
  }
  else if (aRequest == nsIRadio::REQUEST_HANGUP) {
    NS_ASSERTION(aCall, "Must have a call!");
    aCall->ChangeState(nsITelephoneCall::STATE_DISCONNECTED);
  }
  else {
    NS_NOTREACHED("Unknown request type!");
  }
}

NS_IMPL_ISUPPORTS2(Telephone, nsITelephone, nsIRadioCallback)

NS_IMETHODIMP
Telephone::Dial(const nsAString& aPhoneNumber,
                nsITelephoneCall** aResult)
{
  nsRefPtr<TelephoneCall> call;
  nsresult rv = DialConcrete(aPhoneNumber, getter_AddRefs(call));
  NS_ENSURE_SUCCESS(rv, rv);

  call.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
Telephone::RegisterCallback(nsITelephoneCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCallback, "Null pointer!");

  if (mShutdown) {
    NS_WARNING("No callbacks may be added after shutdown!");
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

  NS_ASSERTION(!mCallbacks.Contains(aCallback),
               "Already registered this callback!");

  mCallbacks.AppendElement(aCallback);
  return NS_OK;
}

NS_IMETHODIMP
Telephone::UnregisterCallback(nsITelephoneCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCallback, "Null pointer!");

  if (mShutdown) {
    NS_WARNING("Callbacks already removed at shutdown!");
    return NS_OK;
  }

  NS_ASSERTION(mCallbacks.Contains(aCallback),
               "Don't know anything about this callback!");

  mCallbacks.RemoveElement(aCallback);
  return NS_OK;
}

void
Telephone::OnResponse(nsIRadio* aRadio, PRUint64 aToken, nsresult aErrorCode,
                      void* aData, size_t aDataLen)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  for (PRUint32 index = 0; index < mLiveRequests.Length(); index++) {
    RequestInfo& info = mLiveRequests[index];
    if (info.mToken == aToken) {
      HandleResponse(aRadio, info.mRequest, info.mAssociatedCall, aErrorCode,
                     aData, aDataLen);
      mLiveRequests.RemoveElementAt(index);
      return;
    }
  }

  NS_NOTREACHED("Token not in our list!");
}

void
Telephone::OnNotification(nsIRadio* aRadio, PRUint64 aRequest, void* aData,
                          size_t aDataLen)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

void
Telephone::OnShutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mLiveCalls.IsEmpty()) {
    NS_WARNING("Live calls at shutdown!");

    nsAutoTArray<nsRefPtr<TelephoneCall>, 10> liveCalls;
    liveCalls.AppendElements(mLiveCalls);

    for (PRUint32 index = 0; index < liveCalls.Length(); index++) {
      liveCalls[index]->ChangeState(nsITelephoneCall::STATE_DISCONNECTED);
    }
  }

  NOTIFY_OBSERVERS(OnShutdown, ());
  mCallbacks.Clear();

  mLiveRequests.Clear();

  mShutdown = true;
}
