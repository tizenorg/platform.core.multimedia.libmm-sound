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

#ifndef __MM_SOUND_MGR_FOCUS_IPC_H__
#define __MM_SOUND_MGR_FOCUS_IPC_H__

#include "../../include/mm_sound_msg.h"

#ifdef SUPPORT_CONTAINER
typedef struct container_info
{
	int pid;
	char name[64];
} container_info_t;
#endif

//int __mm_sound_mgr_ipc_create_focus_node(mm_ipc_msg_t *msg);
#ifdef SUPPORT_CONTAINER
int __mm_sound_mgr_focus_ipc_register_focus(int client_pid, int handle_id, const char* stream_type, bool is_for_session, const char* container_name, int container_pid);
#else
int __mm_sound_mgr_focus_ipc_register_focus(int client_pid, int handle_id, const char* stream_type, bool is_for_session);
#endif
//int __mm_sound_mgr_ipc_destroy_focus_node(mm_ipc_msg_t *msg);
int __mm_sound_mgr_focus_ipc_unregister_focus(int pid, int handle_id);

int __mm_sound_mgr_focus_ipc_acquire_focus(int pid, int handle_id, int focus_type, const char* name );

int __mm_sound_mgr_focus_ipc_release_focus(int pid, int handle_id, int focus_type, const char* name);
//int __mm_sound_mgr_ipc_set_focus_watch_cb(mm_ipc_msg_t *msg);
#ifdef SUPPORT_CONTAINER
int __mm_sound_mgr_focus_ipc_watch_focus(int pid, int handle_id, int focus_type, const char* container_name, int container_pid);
#else
int __mm_sound_mgr_focus_ipc_watch_focus(int pid, int handle_id, int focus_type);
#endif
//int __mm_sound_mgr_ipc_unset_focus_watch_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_focus_ipc_unwatch_focus(int pid, int handle_id);

int __mm_sound_mgr_focus_ipc_emergent_exit(int pid);

#endif /* __MM_SOUND_MGR_FOCUS_IPC_H__ */

