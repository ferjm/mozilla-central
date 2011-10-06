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

#include "TelephonySessionGroup.h"

#include "nsIDOMNavigator.h"
#include "nsIDOMNavigatorTelephony.h"
#include "nsIDOMWindow.h"

#include "nsDOMClassInfo.h"

#include "Telephony.h"
#include "TelephonySession.h"

USING_TELEPHONY_NAMESPACE

TelephonySessionGroup::TelephonySessionGroup()
: mLive(false)
{
}

TelephonySessionGroup::~TelephonySessionGroup()
{
  NS_ASSERTION(!mLive, "Should never die while we're live!");
}

// static
already_AddRefed<TelephonySessionGroup>
TelephonySessionGroup::Create()
{
  nsRefPtr<TelephonySessionGroup> group = new TelephonySessionGroup();

  return group.forget();
}

// static
nsresult
TelephonySessionGroup::Construct(nsISupports** aResult)
{
  nsRefPtr<TelephonySessionGroup> group = Create();
  *aResult = group->ToISupports();
  group.forget();
  return NS_OK;
}

void
TelephonySessionGroup::UpdateLiveness()
{
  bool shouldBeLive = false;
  for (PRUint32 index = 0; index < mSessions.Length(); index++) {
    if (mSessions[index]->mLive) {
      shouldBeLive = true;
      break;
    }
  }

  if (mLive && !shouldBeLive) {
    mTelephony->RemoveGroup(this);
    mLive = false;
  }
  else if (!mLive && shouldBeLive) {
    mTelephony->AddGroup(this);
    mLive = true;
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(TelephonySessionGroup)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(TelephonySessionGroup)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mTelephony->ToISupports(),
                                               Telephony, "mTelephony")
  for (PRUint32 index = 0; index < tmp->mSessions.Length(); index++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSessions[i]");
    cb.NoteXPCOMChild(tmp->mSessions[index]->ToISupports());
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(TelephonySessionGroup)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTelephony)
  tmp->mSessions.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TelephonySessionGroup)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephonySessionGroup)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TelephonySessionGroup)
  if (aIID.Equals(NS_GET_IID(TelephonySessionGroup)) ||
      aIID.Equals(NS_GET_IID(nsISupports))) {
    foundInterface = ToISupports();
  }
  else
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(TelephonySessionGroup)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TelephonySessionGroup)

DOMCI_DATA(TelephonySessionGroup, TelephonySessionGroup)

NS_IMETHODIMP
TelephonySessionGroup::Item(PRUint32 aIndex, nsIDOMTelephonySession** aResult)
{
  if (aIndex >= mSessions.Length()) {
    return NS_ERROR_INVALID_ARG;
  }

  nsRefPtr<TelephonySession> session = mSessions[aIndex];
  session.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
TelephonySessionGroup::GetLength(PRUint32* aLength)
{
  *aLength = mSessions.Length();
  return NS_OK;
}

NS_IMETHODIMP
TelephonySessionGroup::Initialize(nsISupports* aOwner, JSContext* aCx,
                                  JSObject* aObj, PRUint32 aArgc, jsval* aArgv)
{
  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(aOwner);
  NS_ENSURE_TRUE(window, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMNavigator> navigator;
  nsresult rv = window->GetNavigator(getter_AddRefs(navigator));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNavigatorTelephony> navigatorTelephony =
    do_QueryInterface(navigator);
  NS_ENSURE_TRUE(navigatorTelephony, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMTelephony> telephony;
  rv = navigatorTelephony->GetMozTelephony(getter_AddRefs(telephony));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<Telephony> concreteTelephony = do_QueryToConcrete(telephony);
  if (!concreteTelephony) {
    NS_WARNING("Non-native nsIDOMTelephony implementation not supported!");
    return NS_ERROR_FAILURE;
  }

  mTelephony = concreteTelephony;
  return NS_OK;
}
