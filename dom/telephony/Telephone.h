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

#ifndef mozilla_dom_telephony_telephone_h__
#define mozilla_dom_telephony_telephone_h__

#include "TelephonyCommon.h"

#include "nsIRadio.h"
#include "nsITelephone.h"

BEGIN_TELEPHONY_NAMESPACE

class TelephoneCall;

class Telephone : public nsITelephone,
                  public nsIRadioCallback
{
  friend class TelephoneCall;

  nsCOMPtr<nsIRadio> mRadio;
  nsAutoTObserverArray<nsCOMPtr<nsITelephoneCallback>, 1> mCallbacks;

  // Weak references.
  nsTArray<TelephoneCall*> mLiveCalls;

  struct RequestInfo
  {
    RequestInfo()
    : mToken(0), mRequest(0), mAssociatedCall(nsnull)
    { }

    PRUint64 mToken;
    PRUint64 mRequest;
    TelephoneCall* mAssociatedCall;
  };

  nsAutoTArray<RequestInfo, 10> mLiveRequests;

  PRUint64 mNextRequestToken;
  bool mShutdown;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONE
  NS_DECL_NSIRADIOCALLBACK

  static already_AddRefed<Telephone>
  Create(nsIRadio* aRadio);

  nsresult
  DialConcrete(const nsAString& aPhoneNumber,
               TelephoneCall** aResult);

  nsresult
  HangUp(TelephoneCall* aCall);

protected:
  Telephone();
  ~Telephone();

  PRUint64
  GetNewRequestToken()
  {
    return mNextRequestToken++;
  }

  void
  OnNewCall(TelephoneCall* aCall);

  void
  OnDyingCall(TelephoneCall* aCall);

  void
  HandleResponse(nsIRadio* aRadio, PRUint64 aRequest, TelephoneCall* aCall,
                 nsresult aErrorCode, void* aData, size_t aDataLen);
};

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_telephone_h__
