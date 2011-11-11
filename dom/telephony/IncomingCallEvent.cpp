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

#include "IncomingCallEvent.h"

#include "nsDOMClassInfo.h"

USING_TELEPHONY_NAMESPACE

// static
already_AddRefed<IncomingCallEvent>
IncomingCallEvent::Create(TelephonySession* aSession)
{
  NS_ASSERTION(aSession, "Null pointer!");

  nsRefPtr<IncomingCallEvent> event = new IncomingCallEvent();

  event->mSession = aSession;

  return event.forget();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IncomingCallEvent)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IncomingCallEvent,
                                                  nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mSession->ToISupports(),
                                               TelephonySession, "mSession")
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IncomingCallEvent,
                                                nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSession)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IncomingCallEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMIncomingCallEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IncomingCallEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(IncomingCallEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(IncomingCallEvent, nsDOMEvent)

DOMCI_DATA(IncomingCallEvent, IncomingCallEvent)

NS_IMETHODIMP
IncomingCallEvent::GetSession(nsIDOMTelephonySession** aSession)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
