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

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include "include/mm_sound_mgr_focus_ipc.h"
#include "include/mm_sound_mgr_focus_dbus.h"

#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
#include "include/mm_sound_mgr_focus.h"
#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>

#ifdef USE_FOCUS

#ifdef SUPPORT_CONTAINER
// method + add callback
int __mm_sound_mgr_focus_ipc_register_focus(int client_pid, int handle_id, const char* stream_type, bool is_for_session, const char* container_name, int container_pid)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = client_pid;
	param.handle_id = handle_id;
	param.is_for_session = is_for_session;
	memcpy(param.stream_type, stream_type, MAX_STREAM_TYPE_LEN);
	debug_error("param.handle_id = %d, handle_id = %d",param.handle_id, handle_id); 
	ret = mm_sound_mgr_focus_create_node(&param);

	mm_sound_mgr_focus_update_container_data(client_pid, handle_id, container_name, container_pid);

	return ret;
}
#else
int __mm_sound_mgr_focus_ipc_register_focus(int client_pid, int handle_id, const char* stream_type, bool is_for_session)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = client_pid;
	param.handle_id = handle_id;
	param.is_for_session = is_for_session;
	memcpy(param.stream_type, stream_type, MAX_STREAM_TYPE_LEN);

	ret = mm_sound_mgr_focus_create_node(&param);

	return ret;
}
#endif

// method + remove callback
int __mm_sound_mgr_focus_ipc_unregister_focus(int pid, int handle_id)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;

	ret = mm_sound_mgr_focus_destroy_node(&param);

	return ret;
}

// method -> callback
int __mm_sound_mgr_focus_ipc_acquire_focus(int pid, int handle_id, int focus_type, const char* name )
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;
	param.request_type = focus_type;
	memcpy(param.option, name, MM_SOUND_NAME_NUM);
	ret = mm_sound_mgr_focus_request_acquire(&param);

	return ret;
}

// method -> callback
int __mm_sound_mgr_focus_ipc_release_focus(int pid, int handle_id, int focus_type, const char* name)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;
	param.request_type = focus_type;
	memcpy(param.option, name, MM_SOUND_NAME_NUM);

	ret = mm_sound_mgr_focus_request_release(&param);

	return ret;
}

#ifdef SUPPORT_CONTAINER
// method + add callback
int __mm_sound_mgr_focus_ipc_watch_focus(int pid, int handle_id, int focus_type, const char* container_name, int container_pid)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;
	param.request_type = focus_type;

	ret = mm_sound_mgr_focus_set_watch_cb(&param);
	mm_sound_mgr_focus_update_container_data(pid, -1, container_name, container_pid);

	return ret;
}
#else
int __mm_sound_mgr_focus_ipc_watch_focus(int pid, int handle_id, int focus_type)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;
	param.request_type = focus_type;

	ret = mm_sound_mgr_focus_set_watch_cb(&param);

	return ret;
}
#endif

// method + remove callback
int __mm_sound_mgr_focus_ipc_unwatch_focus(int pid, int handle_id)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.handle_id = handle_id;

	ret = mm_sound_mgr_focus_unset_watch_cb(&param);

	return ret;
}

int __mm_sound_mgr_focus_ipc_emergent_exit(int pid)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;

	ret = mm_sound_mgr_focus_emergent_exit(&param);

	return ret;
}
#endif

