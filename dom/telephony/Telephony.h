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

#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonySession.h"
#include "nsIDOMTelephonySessionGroup.h"
#include "nsIObserver.h"
#include "nsITelephone.h"

#include "nsDOMEventTargetHelper.h"

// {3103cc5e-0194-42ab-bf17-b8686ee35a6c}
#define TELEPHONY_IID \
  {0x3103cc5e, 0x194, 0x42ab, {0xbf, 0x17, 0xb8, 0x68, 0x6e, 0xe3, 0x5a, 0x6c}}

class nsIRadio;

BEGIN_TELEPHONY_NAMESPACE

class Telephone;

class Telephony : public nsDOMEventTargetHelper,
                  public nsIDOMTelephony,
                  public nsIObserver,
                  public nsITelephoneCallback
{
  friend class TelephonySession;
  friend class TelephonySessionGroup;

  nsRefPtr<Telephone> mTelephone;

  nsRefPtr<nsDOMEventListenerWrapper> mOnincomingListener;

  nsIDOMTelephonySession* mActiveSession;
  nsIDOMTelephonySessionGroup* mActiveGroup;

  nsTArray<nsRefPtr<TelephonySession> > mSessions;
  nsTArray<nsRefPtr<TelephonySessionGroup> > mGroups;

  JSObject* mSessionsArray;
  JSObject* mGroupsArray;

  nsCString mPhoneAppURL;

  bool mMuted;
  bool mUsingSpeaker;
  bool mEnabled;

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(TELEPHONY_IID)
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITELEPHONECALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(Telephony,
                                                         nsDOMEventTargetHelper)

  static already_AddRefed<Telephony>
  Create(nsIRadio* aRadio, const nsACString& aPhoneAppURL);

  nsISupports*
  ToISupports() const
  {
    return static_cast<nsDOMEventTargetHelper*>(const_cast<Telephony*>(this));
  }

private:
  Telephony();
  ~Telephony();

  void
  AddSession(TelephonySession* aSession)
  {
    NS_ASSERTION(!mSessions.Contains(aSession), "Already know about this one!");
    mSessions.AppendElement(aSession);
    mSessionsArray = nsnull;
  }

  void
  RemoveSession(TelephonySession* aSession)
  {
    NS_ASSERTION(mSessions.Contains(aSession), "Didn't know about this one!");
    mSessions.RemoveElement(aSession);
    mSessionsArray = nsnull;
  }

  void
  AddGroup(TelephonySessionGroup* aGroup)
  {
    NS_ASSERTION(!mGroups.Contains(aGroup), "Already know about this one!");
    mGroups.AppendElement(aGroup);
    mGroupsArray = nsnull;
  }

  void
  RemoveGroup(TelephonySessionGroup* aGroup)
  {
    NS_ASSERTION(mGroups.Contains(aGroup), "Didn't know about this one!");
    mGroups.RemoveElement(aGroup);
    mGroupsArray = nsnull;
  }
};

NS_DEFINE_STATIC_IID_ACCESSOR(Telephony, TELEPHONY_IID)

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_telephony_h__
