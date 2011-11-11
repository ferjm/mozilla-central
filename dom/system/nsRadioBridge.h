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
 * The Original Code is the gonk RIL bridge.
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2___
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Blake Kaplan <mrbkap@gmail.com>
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

#include "nsISupports.h"
#include "nsCOMPtr.h"

#define NS_RADIOBRIDGE_CONTRACTID \
    "@mozilla.org/telephony/rilbridge;1"

// d5bd378b-7c8f-4241-b389-f2ee674940fe
#define NS_RADIOBRIDGE_CID \
{ 0xd5bd378b, 0x7c8f, 0x4241, \
  { 0xb3, 0x89, 0xf2, 0xee, 0x67, 0x49, 0x40, 0xfe } }

class nsRadioBridge : public nsISupports
{
public:
    virtual ~nsRadioBridge();
    NS_DECL_ISUPPORTS

    static already_AddRefed<nsRadioBridge> Create();

private:
    nsresult Init();

    static nsRadioBridge *sInstance;
};
