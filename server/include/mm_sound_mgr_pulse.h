/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungbae Shin <seungbae.shin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __MM_SOUND_MGR_PULSE_H__
#define __MM_SOUND_MGR_PULSE_H__

#if 0

#include "../../include/mm_ipc.h"
#include "mm_sound_mgr_session.h"

typedef void (*pa_disconnect_cb)(void* user_data);


void MMSoundMgrPulseSetUSBDefaultSink (int usb_device);
void MMSoundMgrPulseSetDefaultSink (char* device_api_name, char* device_bus_name);
void MMSoundMgrPulseSetDefaultSinkByName (char* name);
void MMSoundMgrPulseSetSourcemutebyname (char* sourcename, int mute);

int MMSoundMgrPulseHandleRegisterBluetoothStatus (void* pinfo);

int MMSoundMgrPulseHandleIsBtA2DPOnReq (bool* is_bt_on, char** bt_name);
void MMSoundMgrPulseGetInitialBTStatus (bool *a2dp, bool *sco);
int MMSoundMgrPulseGetBluetoothInfo(bool* is_nrec, int* bandwidth);

void MMSoundMgrPulseSetSession(session_t session, session_state_t state);
void MMSoundMgrPulseSetSubsession(subsession_t subsession, int subsession_opt);
void MMSoundMgrPulseSetActiveDevice(mm_sound_device_in device_in, mm_sound_device_out device_out);

void MMSoundMgrPulseSetCorkAll (bool cork);
void MMSoundMgrPulseUnLoadHDMI();
void MMSoundMgrPulseGetPathInfo(mm_sound_device_out *device_out, mm_sound_device_in *device_in);
#ifdef TIZEN_MICRO
void MMSoundMgrPulseSetVolumeLevel(volume_type_t volume_type, unsigned int volume_level);
#endif
void MMSoundMgrPulseSetVoicecontrolState (bool state);

void* MMSoundMgrPulseInit(pa_disconnect_cb cb, void* user_data);
int MMSoundMgrPulseFini(void* handle);
#endif

#endif /* __MM_SOUND_MGR_PULSE_H__ */

