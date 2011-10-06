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

#include "TelephonySession.h"

#include "nsDOMClassInfo.h"
#include "nsDOMEvent.h"

#include "TelephoneCall.h"
#include "Telephony.h"
#include "TelephonySessionGroup.h"

USING_TELEPHONY_NAMESPACE

TelephonySession::TelephonySession()
: mLive(false)
{
}

TelephonySession::~TelephonySession()
{
  NS_ASSERTION(!mLive, "Should never die while we're live!");

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

 // static
already_AddRefed<TelephonySession>
TelephonySession::Create(TelephoneCall* aTelephoneCall, Telephony* aTelephony,
                         const nsAString& aNumber)
{
  NS_ASSERTION(aTelephoneCall, "Null pointer!");
  NS_ASSERTION(aTelephony, "Null pointer!");
  NS_ASSERTION(!aNumber.IsEmpty(), "Empty number!");

  nsRefPtr<TelephonySession> session = new TelephonySession();

  session->mTelephoneCall = aTelephoneCall;
  session->mTelephony = aTelephony;
  session->mNumber = aNumber;
  session->mReadyState.AssignLiteral("initial");

  return session.forget();
}

void
TelephonySession::ChangeReadyState(PRUint16 aState,
                                   const nsAString& aStateString)
{
  nsRefPtr<TelephonySession> kungFuDeathGrip(this);

  if (aState == nsITelephoneCall::STATE_DISCONNECTED) {
    NS_ASSERTION(mLive, "Should be live!");
    mTelephony->RemoveSession(this);
    if (mGroup) {
      mGroup->RemoveSession(this);
    }
    mLive = false;
  }
  else if (!mLive) {
    mTelephony->AddSession(this);
    mLive = true;
  }

  mReadyState.Assign(aStateString);

  // Fire events.
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nsnull, nsnull);

  bool dummy;
  if (NS_FAILED(event->InitEvent(NS_LITERAL_STRING("readystatechange"), false,
                                 false)) ||
      NS_FAILED(event->SetTrusted(true)) ||
      NS_FAILED(DispatchEvent(event, &dummy))) {
    NS_WARNING("Failed to dispatch readystatechange event!");
  }

  if (!aStateString.IsEmpty()) {
    event = new nsDOMEvent(nsnull, nsnull);
    if (NS_FAILED(event->InitEvent(aStateString, false, false)) ||
        NS_FAILED(event->SetTrusted(true)) ||
        NS_FAILED(DispatchEvent(event, &dummy))) {
      NS_WARNING("Failed to dispatch specific event!");
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(TelephonySession)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TelephonySession,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mTelephony->ToISupports(),
                                               Telephony, "mTelephony")
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnreadystatechangeListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOndialingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnringingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnbusyListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnconnectedListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOndisconnectingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOndisconnectedListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnheldListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnincomingListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TelephonySession,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnreadystatechangeListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOndialingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnringingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnbusyListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnconnectedListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOndisconnectingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOndisconnectedListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnheldListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnincomingListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TelephonySession)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephonySession)
  NS_INTERFACE_MAP_ENTRY(nsITelephoneCallCallback)
  if (aIID.Equals(NS_GET_IID(TelephonySession))) {
    foundInterface = ToISupports();
  }
  else
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TelephonySession)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(TelephonySession, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TelephonySession, nsDOMEventTargetHelper)

DOMCI_DATA(TelephonySession, TelephonySession)

NS_IMETHODIMP
TelephonySession::Answer()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TelephonySession::HangUp()
{
  return mTelephoneCall->HangUp();
}

NS_IMETHODIMP
TelephonySession::Hold()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TelephonySession::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TelephonySession::GetNumber(nsAString& aNumber)
{
  aNumber.Assign(mNumber);
  return NS_OK;
}

NS_IMETHODIMP
TelephonySession::GetReadyState(nsAString& aReadyState)
{
  aReadyState.Assign(mReadyState);
  return NS_OK;
}

NS_IMETHODIMP
TelephonySession::GetGroup(nsIDOMTelephonySessionGroup** aGroup)
{
  nsCOMPtr<nsIDOMTelephonySessionGroup> group = mGroup;
  group.forget(aGroup);
  return NS_OK;
}

NS_IMETHODIMP
TelephonySession::SetGroup(nsIDOMTelephonySessionGroup* aGroup)
{
  if (mGroup) {
    mGroup->RemoveSession(this);
    mGroup = nsnull;
  }

  if (aGroup) {
    nsRefPtr<TelephonySessionGroup> concreteGroup = do_QueryToConcrete(aGroup);
    if (!concreteGroup) {
      NS_WARNING("Non-native nsIDOMTelephonySessionGroup implementation not "
                 "supported!");
      return NS_ERROR_FAILURE;
    }

    concreteGroup->AddSession(this);
    mGroup = concreteGroup;
  }

  return NS_OK;
}

#define IMPL_EVENT_LISTENER(_type) \
  NS_IMETHODIMP \
  TelephonySession::GetOn##_type(nsIDOMEventListener** aOn##_type) \
  { \
  return GetInnerEventListener(mOn##_type##Listener, aOn##_type); \
  } \
  NS_IMETHODIMP \
  TelephonySession::SetOn##_type(nsIDOMEventListener* aOn##_type) \
  { \
    return RemoveAddEventListener(NS_LITERAL_STRING(#_type), \
                                  mOn##_type##Listener, aOn##_type); \
  }

IMPL_EVENT_LISTENER(readystatechange)
IMPL_EVENT_LISTENER(dialing)
IMPL_EVENT_LISTENER(ringing)
IMPL_EVENT_LISTENER(busy)
IMPL_EVENT_LISTENER(connected)
IMPL_EVENT_LISTENER(disconnecting)
IMPL_EVENT_LISTENER(disconnected)
IMPL_EVENT_LISTENER(held)
IMPL_EVENT_LISTENER(incoming)

#undef IMPL_EVENT_LISTENER

NS_IMETHODIMP
TelephonySession::OnStateChange(PRUint16 aState)
{
  NS_ASSERTION(aState != nsITelephoneCall::STATE_INITIAL, "Huh?!");

  nsString stateString;
  if (aState == nsITelephoneCall::STATE_DIALING) {
    stateString.AssignLiteral("dialing");
  }
  else if (aState == nsITelephoneCall::STATE_RINGING) {
    stateString.AssignLiteral("ringing");
  }
  else if (aState == nsITelephoneCall::STATE_BUSY) {
    stateString.AssignLiteral("busy");
  }
  else if (aState == nsITelephoneCall::STATE_CONNECTED) {
    stateString.AssignLiteral("connected");
  }
  else if (aState == nsITelephoneCall::STATE_HELD) {
    stateString.AssignLiteral("held");
  }
  else if (aState == nsITelephoneCall::STATE_DISCONNECTING) {
    stateString.AssignLiteral("disconnecting");
  }
  else if (aState == nsITelephoneCall::STATE_DISCONNECTED) {
    stateString.AssignLiteral("disconnected");
  }
  else if (aState == nsITelephoneCall::STATE_INCOMING) {
    stateString.AssignLiteral("incoming");
  }
  else if (aState == nsITelephoneCall::STATE_CONNECTING ||
           aState == nsITelephoneCall::STATE_HOLDING ||
           aState == nsITelephoneCall::STATE_RESUMING) {
    // Nothing.
  }
  else {
    NS_NOTREACHED("Unknown state!");
  }

  if (!stateString.IsEmpty()) {
    ChangeReadyState(aState, stateString);
  }

  return NS_OK;
}
