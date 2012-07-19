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

#include <pulse/pulseaudio.h>
#include "../../include/mm_ipc.h"
#include <vconf.h>

typedef struct _server_struct
{
	mm_ipc_msg_t *msg;
	 int (*func)(mm_ipc_msg_t*);
} server_struct;

typedef struct _sink_struct
{
	int old_priority;
	mm_ipc_msg_t *msg;
	int is_speaker_on;
	int is_headset_on;
	int is_bt_on;
	int route_to;
	char speaker_name[256];
	char headset_name[256];
	char bt_name[256];
	int (*func)(mm_ipc_msg_t*);
} sink_struct;

typedef struct _bt_struct
{
	int old_priority;
	mm_ipc_msg_t *msg;
	int bt_found;
	char bt_name[256];
	int (*func)(mm_ipc_msg_t*);
} bt_struct;

int MMSoundMgrPulseInit(void);
int MMSoundMgrPulseFini(void);

int MMSoundMgrPulseHandleGetAudioRouteReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*));
int MMSoundMgrPulseHandleSetAudioRouteReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*));
int MMSoundMgrPulseHandleIsBtA2DPOnReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*));

int MMSoundMgrPulseHandleRegisterMonoAudio ();

#endif /* __MM_SOUND_MGR_PULSE_H__ */

