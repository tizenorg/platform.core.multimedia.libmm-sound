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

#define CONTAINER_NAME_MAX 64

typedef struct container_info
{
	int pid;
	char name[CONTAINER_NAME_MAX];
} container_info_t;
#endif

int MMSoundMgrIpcInit(void);
int MMSoundMgrIpcFini(void);

/* Msg processing */
int _MMSoundMgrIpcPlayFile(char* filename, int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int handle_route,
			   gboolean enable_session, int *codechandle, char *stream_type, int stream_index);
int _MMSoundMgrIpcPlayFileWithStreamInfo(char* filename, int repeat, int volume,
			   int priority, int client_pid, int handle_route, int *codechandle, char *stream_type, int stream_index);
//int _MMSoundMgrIpcStop(mm_ipc_msg_t *msg);
int _MMSoundMgrIpcStop(int handle);
int _MMSoundMgrIpcClearFocus(int pid);
//int _MMSoundMgrIpcPlayDTMF(int *codechandle, mm_ipc_msg_t *msg);
int _MMSoundMgrIpcPlayDTMF(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   gboolean enable_session, int *codechandle, char *stream_type, int stream_index);
int _MMSoundMgrIpcPlayDTMFWithStreamInfo(int tone, int repeat, int volume, int client_pid, int *codechandle, char *stream_type, int stream_index);

/*
int _MMSoundMgrIpcPlayMemory(int *codechandle, mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_is_route_available(mm_ipc_msg_t *msg, bool *is_available);
int __mm_sound_mgr_ipc_foreach_available_route_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_set_active_route(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_set_active_route_auto(void);
int __mm_sound_mgr_ipc_get_active_device(mm_ipc_msg_t *msg, mm_sound_device_in *device_in, mm_sound_device_out *device_out);
int __mm_sound_mgr_ipc_add_active_device_changed_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_remove_active_device_changed_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_add_available_device_changed_cb(mm_ipc_msg_t *msg);
int __mm_sound_mgr_ipc_remove_available_device_changed_cb(mm_ipc_msg_t *msg);
*/
int __mm_sound_mgr_ipc_set_sound_path_for_active_device(mm_sound_device_in _device_in, mm_sound_device_out _device_out);
int __mm_sound_mgr_ipc_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out);
//int __mm_sound_mgr_ipc_get_current_connected_device_list(mm_ipc_msg_t *msg, GList **device_list, int *total_num);
int __mm_sound_mgr_ipc_get_current_connected_device_list(int device_flags, mm_sound_device_t **device_list, int *total_num);

int __mm_sound_mgr_ipc_asm_register_sound(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
#ifdef SUPPORT_CONTAINER
						const char* container_name, int container_pid,
#endif
					  int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					  int* request_id_r, int* sound_command_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_unregister_sound(int pid, int handle, int sound_event, int request_id, int sound_state, int resource);
int __mm_sound_mgr_ipc_asm_register_watcher(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
#ifdef SUPPORT_CONTAINER
						const char* container_name, int container_pid,
#endif
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_unregister_watcher(int pid, int handle, int sound_event, int request_id, int sound_state, int resource);
int __mm_sound_mgr_ipc_asm_get_mystate(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_set_state(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* sound_state_r , int* error_code_r);
int __mm_sound_mgr_ipc_asm_get_state(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_set_subsession(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r, int* request_id_r);
int __mm_sound_mgr_ipc_asm_get_subsession(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r, int* request_id_r, int* sound_command_r);

int __mm_sound_mgr_ipc_asm_set_subevent(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_get_subevent(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r);
int __mm_sound_mgr_ipc_asm_set_session_option(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r, int* error_code_r );
int __mm_sound_mgr_ipc_asm_get_session_option(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r, int* option_flag_r );
int __mm_sound_mgr_ipc_asm_reset_resume_tag(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_cmd_r, int* sound_state_r );
int __mm_sound_mgr_ipc_asm_dump(int pid, int handle, int sound_event, int request_id, int sound_state, int resource);
int __mm_sound_mgr_ipc_asm_emergent_exit(int pid, int handle, int sound_event, int request_id, int sound_state);


/* send signal : mgr_xxx -> mgr_ipc_dbus */
int _MMIpcCBSndMsg(mm_ipc_msg_t *msg);
int _MMIpcCBRecvMsg(mm_ipc_msg_t *msg);
int _MMIpcCBMsgEnQueueAgain(mm_ipc_msg_t *msg);

int __mm_sound_mgr_ipc_freeze_send (char* command, int pid);

int __mm_sound_mgr_ipc_notify_play_file_end (int handle);
int __mm_sound_mgr_ipc_notify_device_connected (mm_sound_device_t *device, gboolean is_connected);
int __mm_sound_mgr_ipc_notify_device_info_changed (mm_sound_device_t *device, int changed_device_info_type);
int __mm_sound_mgr_ipc_notify_volume_changed(unsigned int vol_type, unsigned int value);
int __mm_sound_mgr_ipc_notify_active_device_changed(int device_in, int device_out);
int __mm_sound_mgr_ipc_notify_available_device_changed(int device_in, int device_out, int available);

#endif /* __MM_SOUND_MGR_H__ */

