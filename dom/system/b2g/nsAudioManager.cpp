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
 * The Original Code is B2G Audio Manager.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Philipp von Weitershausen <philipp@weitershausen.de>
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

#include "nsAudioManager.h"

using namespace mozilla::dom::b2g;
using namespace android;

NS_IMETHODIMP
nsAudioManager::GetMicrophoneMuted(bool* microphoneMuted)
{
  if (!AudioSystem::isMicrophoneMuted(microphoneMuted))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::SetMicrophoneMuted(bool aMicrophoneMuted)
{
  if (!AudioSystem::muteMicrophone(aMicrophoneMuted))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::GetMasterVolume(float* masterVolume)
{
  if (!AudioSystem::getMasterVolume(masterVolume))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::SetMasterVolume(float aMasterVolume)
{
  if (!AudioSystem::setMasterVolume(aMasterVolume))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::GetMasterMuted(bool* masterMuted)
{
  if (!AudioSystem::getMasterMute(masterMuted))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::SetMasterMuted(bool aMasterMuted)
{
  if (!AudioSystem::setMasterMute(aMasterMuted))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::SetPhoneState(PRInt32 aState)
{
  if (!AudioSystem::setPhoneState(aState))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::SetForceForUse(PRInt32 aUsage, PRInt32 aForce)
{
  if (!AudioSystem::setForceUse((AudioSystem::force_use)aUsage,
                                (AudioSystem::forced_config)aForce))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
nsAudioManager::GetForceForUse(PRInt32 aUsage, PRInt32* force) {
  *((AudioSystem::forced_config*)force) =
    AudioSystem::getForceUse((AudioSystem::force_use)aUsage);
  return NS_OK;
}
