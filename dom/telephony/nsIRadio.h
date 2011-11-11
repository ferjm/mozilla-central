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

#ifndef mozilla_dom_telephony_nsiradio_h__
#define mozilla_dom_telephony_nsiradio_h__

#include "TelephonyCommon.h"

#include "nsISupports.h"

// {dbf87f1a-eaf8-41e3-af32-0ee5d9a91210}
#define NSIRADIO_IID \
  {0xdbf87f1a, 0xeaf8, 0x41e3, {0xaf, 0x32, 0x0e, 0xe5, 0xd9, 0xa9, 0x12, 0x10}}

// {1699a387-1011-481b-96ba-941e76b7bd87}
#define NSIRADIOCALLBACK_IID \
  {0x1699a387, 0x1011, 0x481b, {0x96, 0xba, 0x94, 0x1e, 0x76, 0xb7, 0xbd, 0x87}}

#define TELEPHONYRADIO_CONTRACTID "@mozilla.org/telephony/radio;1"

class nsIRadioCallback;

class nsIRadio : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NSIRADIO_IID)

  struct REQUEST_DIAL_Args
  {
    nsString mPhoneNumber;
  };

  struct REQUEST_HANGUP_Args
  {
    PRUint64 mId;
  };

  // XXX Totally fake list of requests. Eventually use the ones in ril.h.
  enum
  {
    REQUEST_DIAL = 0,
    REQUEST_HANGUP,
    REQUEST_INCOMING
  };

  virtual nsresult
  RegisterCallback(nsIRadioCallback* aCallback) = 0;

  virtual nsresult
  UnregisterCallback(nsIRadioCallback* aCallback) = 0;

  // Callee takes ownership of aData on success.
  virtual nsresult
  MakeRequest(PRUint64 aToken, PRUint64 aRequest, void* aData,
              size_t aDataLen) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadio, NSIRADIO_IID)

class nsIRadioCallback : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NSIRADIOCALLBACK_IID)

  // aData owned by caller, not to be modified by callee.
  virtual void
  OnResponse(nsIRadio* aRadio, PRUint64 aToken, nsresult aErrorCode,
             void* aData, size_t aDataLen) = 0;

  // aData owned by caller, not to be modified by callee.
  virtual void
  OnNotification(nsIRadio* aRadio, PRUint64 aRequest, void* aData,
                 size_t aDataLen) = 0;

  virtual void
  OnShutdown() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioCallback, NSIRADIOCALLBACK_IID)

#define NS_DECL_NSIRADIOCALLBACK                                               \
  virtual void                                                                 \
  OnResponse(nsIRadio* aRadio, PRUint64 aToken, nsresult aErrorCode,           \
             void* aData, size_t aDataLen);                                    \
                                                                               \
  virtual void                                                                 \
  OnNotification(nsIRadio* aRadio, PRUint64 aRequest, void* aData,             \
                 size_t aDataLen);                                             \
                                                                               \
  virtual void                                                                 \
  OnShutdown();

#endif // mozilla_dom_telephony_nsiradio_h__
