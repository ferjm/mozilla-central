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

#ifndef mozilla_dom_telephony_test_phauxphone_phauxphone_h__
#define mozilla_dom_telephony_test_phauxphone_phauxphone_h__

#include "RadioBase.h"

#include "nsIPhauxPhone.h"

// {2bb1a165-b919-4c15-a104-46113829f281}
#define PHAUXPHONE_CID \
  {0x2bb1a165, 0xb919, 0x4c15, {0xa1, 0x4, 0x46, 0x11, 0x38, 0x29, 0xf2, 0x81}}

#define PHAUXPHONE_CONTRACTID \
  "@mozilla.org/telephony/phauxphone;1"

BEGIN_TELEPHONY_NAMESPACE

namespace test {

class PhauxPhone : public RadioBase,
                   public nsIPhauxPhone
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPHAUXPHONE

  static already_AddRefed<PhauxPhone>
  FactoryCreate();

  // IRadio methods
  virtual nsresult
  MakeRequest(PRUint64 aToken, PRUint64 aRequest, void* aData, size_t aDataLen);

  void
  SimulateResponse(PRUint64 aToken, nsresult aErrorCode, void* aData,
                   size_t aDataLen);

  // We use special assertion magic here because NS_IsMainThread stops working
  // for non-MOZILLA_INTERNAL_API callers after the service manager is stopped.
  static void
  AssertIsOnMainThread()
#ifdef DEBUG
  ; // Implemented in Module.cpp
#else
  { }
#endif

private:
  PhauxPhone();
  ~PhauxPhone();
};

} // namespace test

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_test_phauxphone_phauxphone_h__
