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

#include "Telephony.h"

#include "nsIDocument.h"
#include "nsIRadio.h"
#include "nsIURI.h"
#include "nsPIDOMWindow.h"

#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"

#include "Telephone.h"
#include "TelephoneCall.h"
#include "TelephonySession.h"
#include "TelephonySessionGroup.h"

USING_TELEPHONY_NAMESPACE
using mozilla::Preferences;

#define DOM_TELEPHONY_APP_PHONE_URL_PREF "dom.telephony.app.phone.url"

namespace {

template <class T>
inline nsresult
nsTArrayToJSArray(JSContext* aCx, const nsTArray<nsRefPtr<T> >& aSourceArray,
                  JSObject** aResultArray)
{
  JSObject* arrayObj;

  if (aSourceArray.IsEmpty()) {
    arrayObj = JS_NewArrayObject(aCx, 0, nsnull);
  }
  else {
    nsTArray<jsval> valArray;
    valArray.SetLength(aSourceArray.Length());

    JSObject* global = JS_GetGlobalForScopeChain(aCx);

    for (PRUint32 index = 0; index < valArray.Length(); index++) {
      nsISupports* obj = aSourceArray[index]->ToISupports();
      nsresult rv =
        nsContentUtils::WrapNative(aCx, global, obj, &valArray[index]);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    arrayObj = JS_NewArrayObject(aCx, valArray.Length(), valArray.Elements());
  }

  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // XXX This is not what Jonas wants. He wants it to be live.
  if (!JS_FreezeObject(aCx, arrayObj)) {
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

} // anonymous namespace

Telephony::Telephony()
: mActiveSession(nsnull), mActiveGroup(nsnull), mSessionsArray(nsnull),
  mGroupsArray(nsnull), mMuted(false), mUsingSpeaker(false), mEnabled(true)
{
  NS_HOLD_JS_OBJECTS(this, Telephony);
}

Telephony::~Telephony()
{
  NS_DROP_JS_OBJECTS(this, Telephony);

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

// static
already_AddRefed<Telephony>
Telephony::Create(nsIRadio* aRadio, const nsACString& aPhoneAppURL)
{
  nsRefPtr<Telephony> telephony = new Telephony();

  nsresult rv =
    Preferences::AddStrongObserver(telephony, DOM_TELEPHONY_APP_PHONE_URL_PREF);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsRefPtr<Telephone> telephone = Telephone::Create(aRadio);
  NS_ASSERTION(telephone, "This should never fail!");

  rv = telephone->RegisterCallback(telephony);
  NS_ENSURE_SUCCESS(rv, nsnull);

  telephony->mTelephone.swap(telephone);
  telephony->mPhoneAppURL = aPhoneAppURL;

  return telephony.forget();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(Telephony)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(Telephony,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnincomingListener)
  for (PRUint32 index = 0; index < tmp->mSessions.Length(); index++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSessions[i]");
    cb.NoteXPCOMChild(tmp->mSessions[index]->ToISupports());
  }
  for (PRUint32 index = 0; index < tmp->mGroups.Length(); index++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mGroups[i]");
    cb.NoteXPCOMChild(tmp->mGroups[index]->ToISupports());
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(Telephony,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->mSessionsArray,
                                             "mSessionsArray")
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->mGroupsArray, "mGroupsArray")
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(Telephony,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnincomingListener)
  tmp->mSessions.Clear();
  tmp->mActiveSession = nsnull;
  tmp->mGroups.Clear();
  tmp->mActiveGroup = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(Telephony)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephony)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsITelephoneCallback)
  if (aIID.Equals(NS_GET_IID(Telephony))) {
    foundInterface = ToISupports();
  }
  else
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Telephony)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(Telephony, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(Telephony, nsDOMEventTargetHelper)

DOMCI_DATA(Telephony, Telephony)

NS_IMETHODIMP
Telephony::Connect(const nsAString& aNumber, nsIDOMTelephonySession** aResult)
{
  NS_ENSURE_ARG(!aNumber.IsEmpty());

  nsRefPtr<TelephoneCall> call;
  nsresult rv = mTelephone->DialConcrete(aNumber, getter_AddRefs(call));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<TelephonySession> session =
    TelephonySession::Create(call, this, aNumber);
  NS_ENSURE_TRUE(session, NS_ERROR_FAILURE);

  rv = call->RegisterCallback(session);
  NS_ENSURE_SUCCESS(rv, rv);

  session.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
Telephony::StartTone(const nsAString& aTone)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Telephony::StopTone()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Telephony::SendTones(const nsAString& aTones, PRUint32 aToneDuration,
                     PRUint32 aIntervalDuration)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Telephony::GetMute(bool* aMute)
{
  *aMute = mMuted;
  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetMute(bool aMute)
{
  mMuted = aMute;
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetSpeaker(bool* aSpeaker)
{
  *aSpeaker = mUsingSpeaker;
  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetSpeaker(bool aSpeaker)
{
  mUsingSpeaker = aSpeaker;
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetActive(JSContext* aCx, jsval* aActive)
{
  if (!mActiveSession && !mActiveGroup) {
    *aActive = JSVAL_NULL;
    return NS_OK;
  }

  nsISupports* result = mActiveSession ?
                        static_cast<nsISupports*>(mActiveSession) :
                        static_cast<nsISupports*>(mActiveGroup);

  JSObject* global = JS_GetGlobalForScopeChain(aCx);

  nsresult rv = nsContentUtils::WrapNative(aCx, global, result, aActive);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetActive(JSContext* aCx, const jsval& aActive)
{
  if (!JSVAL_IS_PRIMITIVE(aActive)) {
    JSObject* obj = JSVAL_TO_OBJECT(aActive);

    nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative;
    nsContentUtils::XPConnect()->
      GetWrappedNativeOfJSObject(aCx, obj, getter_AddRefs(wrappedNative));

    nsCOMPtr<nsIDOMTelephonySession> session =
      do_QueryInterface(wrappedNative->Native());
    if (session) {
      nsRefPtr<TelephonySession> concreteSession = do_QueryToConcrete(session);
      if (!concreteSession) {
        NS_WARNING("Non-native nsIDOMTelephonySession implementation not "
                   "supported!");
        return NS_ERROR_FAILURE;
      }

      if (concreteSession->mTelephony == this) {
        mActiveSession = session;
        mActiveGroup = nsnull;
        return NS_OK;
      }
    }
    else {
      nsCOMPtr<nsIDOMTelephonySessionGroup> group =
        do_QueryInterface(wrappedNative->Native());
      if (group) {
        nsRefPtr<TelephonySessionGroup> concreteGroup =
          do_QueryToConcrete(group);
        if (!concreteGroup) {
          NS_WARNING("Non-native nsIDOMTelephonySessionGroup implementation "
                     "not supported!");
          return NS_ERROR_FAILURE;
        }

        if (!concreteGroup->IsEmpty()) {
          mActiveSession = nsnull;
          mActiveGroup = group;
          return NS_OK;
        }
      }
    }
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
Telephony::GetLiveSessions(JSContext* aCx, jsval* aSessions)
{
  JSObject* sessions = mSessionsArray;
  if (!sessions) {
    nsresult rv = nsTArrayToJSArray(aCx, mSessions, &sessions);
    NS_ENSURE_SUCCESS(rv, rv);

    mSessionsArray = sessions;
  }

  *aSessions = OBJECT_TO_JSVAL(sessions);
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetLiveGroups(JSContext* aCx, jsval* aGroups)
{
  JSObject* groups = mGroupsArray;
  if (!groups) {
    nsresult rv = nsTArrayToJSArray(aCx, mGroups, &groups);
    NS_ENSURE_SUCCESS(rv, rv);

    mGroupsArray = groups;
  }

  *aGroups = OBJECT_TO_JSVAL(groups);
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetOnincoming(nsIDOMEventListener** aOnincoming)
{
  return GetInnerEventListener(mOnincomingListener, aOnincoming);
}

NS_IMETHODIMP
Telephony::SetOnincoming(nsIDOMEventListener* aOnincoming)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("incoming"),
                                mOnincomingListener, aOnincoming);
}

NS_IMETHODIMP
Telephony::Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    NS_ConvertUTF16toUTF8 newPhoneAppURL(aData);
    mEnabled =
      newPhoneAppURL.Equals(mPhoneAppURL, nsCaseInsensitiveCStringComparator());
  }
  else {
    NS_WARNING("Unknown observer topic!");
  }

  return NS_OK;
}

NS_IMETHODIMP
Telephony::OnIncomingCall(nsITelephoneCall* aCall)
{
  return NS_OK;
}

NS_IMETHODIMP
Telephony::OnShutdown()
{
  return NS_OK;
}

nsresult
NS_NewTelephony(nsPIDOMWindow* aWindow, nsIDOMTelephony** aTelephony)
{
  NS_ASSERTION(aWindow, "Null pointer!");

  // Make sure we're dealing with an inner window.
  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
                               aWindow :
                               aWindow->GetCurrentInnerWindow();
  NS_ENSURE_TRUE(innerWindow, NS_ERROR_FAILURE);

  if (!nsContentUtils::CanCallerAccess(innerWindow)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDocument> document =
    do_QueryInterface(innerWindow->GetExtantDocument());
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  nsCOMPtr<nsIURI> documentURI;
  nsresult rv = document->NodePrincipal()->GetURI(getter_AddRefs(documentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString documentURL;
  rv = documentURI->GetSpec(documentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString phoneAppURL;
  rv = Preferences::GetCString(DOM_TELEPHONY_APP_PHONE_URL_PREF, &phoneAppURL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<Telephony> telephony;
  if (phoneAppURL.Equals(documentURL, nsCaseInsensitiveCStringComparator())) {
    nsCOMPtr<nsIRadio> radio = do_GetService(TELEPHONYRADIO_CONTRACTID);
    NS_ENSURE_TRUE(radio, NS_ERROR_FAILURE);

    telephony = Telephony::Create(radio, phoneAppURL);
  }

  telephony.forget(aTelephony);
  return NS_OK;
}
