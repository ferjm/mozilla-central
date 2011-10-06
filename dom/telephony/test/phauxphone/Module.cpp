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

#include "PhauxPhone.h"

#include "mozilla/ModuleUtils.h"
#include "nsThreadUtils.h"

using namespace mozilla::dom::telephony::test;

namespace {

#ifdef DEBUG
PRThread* gMainPRThread = nsnull;

nsresult
ModuleLoad()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  gMainPRThread = PR_GetCurrentThread();
  NS_ASSERTION(gMainPRThread, "PR_GetCurrentThread failed!");

  return NS_OK;
}

void
ModuleUnload()
{
  PhauxPhone::AssertIsOnMainThread();
  gMainPRThread = nsnull;
}
#endif // DEBUG

NS_DEFINE_NAMED_CID(PHAUXPHONE_CID);

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(PhauxPhone, PhauxPhone::FactoryCreate)

const mozilla::Module::CIDEntry gPhauxPhoneCIDs[] = {
  { &kPHAUXPHONE_CID, true, nsnull, PhauxPhoneConstructor },
  { nsnull }
};

const mozilla::Module::ContractIDEntry gPhauxPhoneContracts[] = {
    { PHAUXPHONE_CONTRACTID, &kPHAUXPHONE_CID },
    { TELEPHONYRADIO_CONTRACTID, &kPHAUXPHONE_CID },
    { nsnull }
};

const mozilla::Module gPhauxPhoneModule = {
  mozilla::Module::kVersion,
  gPhauxPhoneCIDs,
  gPhauxPhoneContracts,
  nsnull, // category entries
  nsnull, // constructor
#ifdef DEBUG
  ModuleLoad, ModuleUnload
#else
  nsnull, nsnull
#endif
};

} // anonymous namespace

#ifdef DEBUG
// static
void
PhauxPhone::AssertIsOnMainThread()
{
  NS_ASSERTION(gMainPRThread, "Calling AssertIsOnMainThread after unload?!");
  NS_ASSERTION(PR_GetCurrentThread() == gMainPRThread, "Wrong thread!");
}
#endif // DEBUG

NSMODULE_DEFN(PhauxPhone) = &gPhauxPhoneModule;
