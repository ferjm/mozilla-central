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

#ifndef mozilla_dom_telephony_telephonysession_h__
#define mozilla_dom_telephony_telephonysession_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephonySession.h"
#include "nsITelephoneCall.h"

#include "nsDOMEventTargetHelper.h"

// {dc019ade-a90d-4ab7-8a7f-362dbb3edf53}
#define TELEPHONYSESSION_IID \
  {0xdc019ade, 0xa90d, 0x4ab7, {0x8a, 0x7f, 0x36, 0x2d, 0xbb, 0x3e, 0xdf, 0x53}}

BEGIN_TELEPHONY_NAMESPACE

class TelephoneCall;

class TelephonySession : public nsDOMEventTargetHelper,
                         public nsIDOMTelephonySession,
                         public nsITelephoneCallCallback
{
  friend class Telephony;
  friend class TelephonySessionGroup;

  nsRefPtr<TelephoneCall> mTelephoneCall;
  nsRefPtr<Telephony> mTelephony;
  nsString mNumber;
  nsString mReadyState;
  nsRefPtr<TelephonySessionGroup> mGroup;

  nsRefPtr<nsDOMEventListenerWrapper> mOnreadystatechangeListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOndialingListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnringingListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnbusyListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnconnectedListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOndisconnectingListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOndisconnectedListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnheldListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnincomingListener;

  bool mLive;

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(TELEPHONYSESSION_IID)
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONYSESSION
  NS_DECL_NSITELEPHONECALLCALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TelephonySession,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<TelephonySession>
  Create(TelephoneCall* aTelephoneCall, Telephony* aTelephony,
         const nsAString& aNumber);

  nsISupports*
  ToISupports() const
  {
    return static_cast<nsDOMEventTargetHelper*>(
              const_cast<TelephonySession*>(this));
  }

private:
  TelephonySession();
  ~TelephonySession();

  void
  ChangeReadyState(PRUint16 aState, const nsAString& aStateString);
};

NS_DEFINE_STATIC_IID_ACCESSOR(TelephonySession, TELEPHONYSESSION_IID)

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_telephonysession_h__
