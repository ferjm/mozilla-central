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

#include "TelephoneCall.h"

#include "nsThreadUtils.h"

#include "Telephone.h"

#define NOTIFY_OBSERVERS(func_, params_) \
  PR_BEGIN_MACRO                                                               \
    nsAutoTObserverArray<nsCOMPtr<nsITelephoneCallCallback>, 1>::              \
      ForwardIterator iter_(mCallbacks);                                       \
    while (iter_.HasMore()) {                                                  \
      nsCOMPtr<nsITelephoneCallCallback> obs_ = iter_.GetNext();               \
      obs_ -> func_ params_ ;                                                  \
    }                                                                          \
  PR_END_MACRO

USING_TELEPHONY_NAMESPACE

TelephoneCall::TelephoneCall()
: mTelephone(nsnull), mId(0), mState(nsITelephoneCall::STATE_INITIAL)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

TelephoneCall::~TelephoneCall()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mTelephone->OnDyingCall(this);
}

// static
already_AddRefed<TelephoneCall>
TelephoneCall::Create(Telephone* aTelephone, const nsAString& aPhoneNumber,
                      PRUint16 aState)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTelephone, "Null pointer!");
  NS_ASSERTION(!aPhoneNumber.IsEmpty(), "Empty phone number!");

  nsRefPtr<TelephoneCall> call = new TelephoneCall();

  call->mTelephone = aTelephone;
  call->mPhoneNumber = aPhoneNumber;
  call->mState = aState;

  aTelephone->OnNewCall(call);

  return call.forget();
}

nsresult
TelephoneCall::HangUp()
{
  return mTelephone->HangUp(this);
}

void
TelephoneCall::ChangeState(PRUint16 aState)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mState = aState;

  NOTIFY_OBSERVERS(OnStateChange, (aState));

  if (mState == nsITelephoneCall::STATE_DISCONNECTED) {
    mCallbacks.Clear();
  }
}

NS_IMPL_ISUPPORTS1(TelephoneCall, nsITelephoneCall)

NS_IMETHODIMP
TelephoneCall::GetPhoneNumber(nsAString& aPhoneNumber)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  aPhoneNumber = mPhoneNumber;
  return NS_OK;
}

NS_IMETHODIMP
TelephoneCall::GetState(PRUint16* aState)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aState);

  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
TelephoneCall::RegisterCallback(nsITelephoneCallCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCallback, "Null pointer!");

  if (mState == nsITelephoneCall::STATE_DISCONNECTED) {
    NS_WARNING("No callbacks may be added after inactive!");
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

  NS_ASSERTION(!mCallbacks.Contains(aCallback),
               "Already registered this callback!");

  mCallbacks.AppendElement(aCallback);
  return NS_OK;
}

NS_IMETHODIMP
TelephoneCall::UnregisterCallback(nsITelephoneCallCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCallback, "Null pointer!");

  if (mState == nsITelephoneCall::STATE_DISCONNECTED) {
    NS_WARNING("Callbacks already removed at inactive!");
    return NS_OK;
  }

  NS_ASSERTION(mCallbacks.Contains(aCallback),
               "Don't know anything about this callback!");

  mCallbacks.RemoveElement(aCallback);
  return NS_OK;
}
