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

#ifndef __MM_SOUND_MGR_IPC_H__
#define __MM_SOUND_MGR_IPC_H__

#include "../../include/mm_sound_msg.h"

#define SOUND_MSG_SET(sound_msg, x_msgtype, x_handle, x_code, x_msgid) \
do { \
	sound_msg.msgtype = x_msgtype; \
	sound_msg.handle = x_handle; \
	sound_msg.code = x_code; \
	sound_msg.msgid = x_msgid; \
} while(0)

#define FREEZE_COMMAND_EXCLUDE	"exclude"
#define FREEZE_COMMAND_INCLUDE	"include"
#define FREEZE_COMMAND_WAKEUP	"wakeup"

#ifdef SUPPORT_CONTAINER
typedef struct container_info
{
	int pid;
	char name[64];
} container_info_t;
#endif

int MMSoundMgrIpcInit(void);
int MMSoundMgrIpcFini(void);

#ifdef USE_FOCUS
//int __mm_sound_mgr_ipc_create_focus_node(mm_ipc_msg_t *msg);
#ifdef SUPPORT_CONTAINER
int __mm_sound_mgr_ipc_register_focus(int client_pid, int handle_id, char* stream_type ,const char* container_name, int container_pid);
#else
int __mm_sound_mgr_ipc_register_focus(int client_pid, int handle_id, char* stream_type);
#endif
//int __mm_sound_mgr_ipc_destroy_focus_node(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_unregister_focus(int pid, int handle_id);

int __mm_sound_mgr_ipc_acquire_focus(int pid, int handle_id, int focus_type, const char* name );

int __mm_sound_mgr_ipc_release_focus(int pid, int handle_id, int focus_type, const char* name);
//int __mm_sound_mgr_ipc_set_focus_watch_cb(mm_ipc_msg_t *msg);
#ifdef SUPPORT_CONTAINER
int __mm_sound_mgr_ipc_watch_focus(int pid, int handle_id, int focus_type, const char* container_name, int container_pid);
#else
int __mm_sound_mgr_ipc_watch_focus(int pid, int handle_id, int focus_type);
#endif
//int __mm_sound_mgr_ipc_unset_focus_watch_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_unwatch_focus(int pid, int handle_id);
#endif

#endif /* __MM_SOUND_MGR_H__ */

