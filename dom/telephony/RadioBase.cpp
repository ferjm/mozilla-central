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

#include "RadioBase.h"

#include "nsIObserverService.h"

#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"

#define PROFILE_BEFORE_CHANGE_TOPIC "profile-before-change"

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

RadioBase::RadioBase()
: mShutdown(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

RadioBase::~RadioBase()
{
  // Can't assert main thread here because we may be running PhauxPhone. See
  // PhauxPhone::AssertIsOnMainThread for more details.
}

nsresult
RadioBase::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (!obs) {
    NS_WARNING("Failed to get observer service!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = obs->AddObserver(this, PROFILE_BEFORE_CHANGE_TOPIC, false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
RadioBase::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NOTIFY_OBSERVERS(OnShutdown, ());
  mCallbacks.Clear();

  mShutdown = true;
}

NS_IMPL_ISUPPORTS2(RadioBase, nsIRadio, nsIObserver)

nsresult
RadioBase::RegisterCallback(nsIRadioCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mShutdown) {
    NS_WARNING("No callbacks may be added after shutdown!");
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

  NS_ASSERTION(!mCallbacks.Contains(aCallback),
               "Already registered this callback!");

  mCallbacks.AppendElement(aCallback);
  return NS_OK;
}

nsresult
RadioBase::UnregisterCallback(nsIRadioCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mShutdown) {
    NS_WARNING("Callbacks already removed at shutdown!");
    return NS_OK;
  }

  NS_ASSERTION(mCallbacks.Contains(aCallback),
               "Didn't know anything about this callback!");

  mCallbacks.RemoveElement(aCallback);
  return NS_OK;
}

NS_IMETHODIMP
RadioBase::Observe(nsISupports* aSubject, const char* aTopic,
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
