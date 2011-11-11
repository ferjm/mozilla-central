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

#ifndef mozilla_dom_telephony_telephonysessiongroup_h__
#define mozilla_dom_telephony_telephonysessiongroup_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephonySessionGroup.h"
#include "nsIJSNativeInitializer.h"

// {77cc9c7a-720f-4afc-91a2-28ba4bb8d033}
#define TELEPHONYSESSIONGROUP_IID \
  {0x77cc9c7a, 0x720f, 0x4afc, {0x91, 0xa2, 0x28, 0xba, 0x4b, 0xb8, 0xd0, 0x33}}

BEGIN_TELEPHONY_NAMESPACE

class TelephonySessionGroup : public nsIDOMTelephonySessionGroup,
                              public nsIJSNativeInitializer
{
  friend class Telephony;
  friend class TelephonySession;

  nsRefPtr<Telephony> mTelephony;
  nsTArray<nsRefPtr<TelephonySession> > mSessions;
  bool mLive;

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(TELEPHONYSESSIONGROUP_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMTELEPHONYSESSIONGROUP
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(TelephonySessionGroup,
                                           nsIDOMTelephonySessionGroup)

  static already_AddRefed<TelephonySessionGroup>
  Create();

  // Called by nsDOMClassInfo.
  static nsresult
  Construct(nsISupports** aResult);

  nsISupports*
  ToISupports() const
  {
    return static_cast<nsIDOMTelephonySessionGroup*>(
             const_cast<TelephonySessionGroup*>(this));
  }

  NS_IMETHOD
  Initialize(nsISupports* aOwner, JSContext* aCx, JSObject* aObj,
             PRUint32 aArgc, jsval* aArgv);

private:
  TelephonySessionGroup();
  ~TelephonySessionGroup();

  // Called by Telephony.
  bool
  IsEmpty() const
  {
    return mSessions.IsEmpty();
  }

  // Called by TelephonySession.
  void
  AddSession(TelephonySession* aSession)
  {
    NS_ASSERTION(!mSessions.Contains(aSession), "Already know about this one!");
    mSessions.AppendElement(aSession);
    UpdateLiveness();
  }

  // Called by TelephonySession.
  void
  RemoveSession(TelephonySession* aSession)
  {
    NS_ASSERTION(mSessions.Contains(aSession), "Didn't know about this one!");
    mSessions.RemoveElement(aSession);
    UpdateLiveness();
  }

  void
  UpdateLiveness();
};

NS_DEFINE_STATIC_IID_ACCESSOR(TelephonySessionGroup, TELEPHONYSESSIONGROUP_IID)

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_telephonysessiongroup_h__
