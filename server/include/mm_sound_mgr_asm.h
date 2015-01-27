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

#ifndef __MM_SOUND_MGR_ASM_H__
#define __MM_SOUND_MGR_ASM_H__

#include "mm_ipc.h"
#include <audio-session-manager.h>

int MMSoundMgrASMInit(void);
int MMSoundMgrASMFini(void);

int _mm_sound_mgr_asm_register_sound(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                     int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_sound_state);
int _mm_sound_mgr_asm_unregister_sound(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource);
int _mm_sound_mgr_asm_register_watcher(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                       int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_sound_state);
int _mm_sound_mgr_asm_unregister_watcher(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource);
int _mm_sound_mgr_asm_get_mystate(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                  int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_state);
int _mm_sound_mgr_asm_set_state(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
				int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_sound_state, int *snd_error_code);
int _mm_sound_mgr_asm_get_state(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
				int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_state);
int _mm_sound_mgr_asm_set_subsession(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                     int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id);
int _mm_sound_mgr_asm_get_subsession(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                     int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command);
int _mm_sound_mgr_asm_set_subevent(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                   int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_sound_state);
int _mm_sound_mgr_asm_get_subevent(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                   int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command);
int _mm_sound_mgr_asm_set_session_option(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                         int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_error_code);
int _mm_sound_mgr_asm_get_session_option(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                         int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_option_flag);
int _mm_sound_mgr_asm_reset_resume_tag(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource,
                                       int *snd_pid, int *snd_alloc_handle, int *snd_cmd_handle, int *snd_request_id, int *snd_sound_command, int *snd_sound_state);
int _mm_sound_mgr_asm_dump(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state, int rcv_resource);
int _mm_sound_mgr_asm_emergent_exit(int rcv_pid, int rcv_handle, int rcv_sound_event, int rcv_request_id, int rcv_sound_state);

int __asm_process_message (void *asm_rcv_msg, void *asm_ret_msg);

#ifdef SUPPORT_CONTAINER
void _mm_sound_mgr_asm_update_container_data(int instance_id, const char* container_name, int container_pid);
#endif

#endif /* __MM_SOUND_MGR_ASM_H__ */

