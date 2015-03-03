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
//#include <pthread.h>
#include "include/mm_sound_mgr_common.h"
#include "include/mm_sound_mgr_ipc.h"
#include "include/mm_sound_mgr_ipc_dbus.h"

#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
//#include "include/mm_sound_thread_pool.h"
#include "include/mm_sound_mgr_codec.h"
#include "include/mm_sound_mgr_device.h"
#include "include/mm_sound_mgr_asm.h"
#ifdef USE_FOCUS
#include "include/mm_sound_mgr_focus.h"
#endif
#include <mm_error.h>
#include <mm_debug.h>

#include <audio-session-manager.h>

#include <gio/gio.h>

#define SHM_OPEN

#ifdef PULSE_CLIENT
#include "include/mm_sound_mgr_pulse.h"
#endif

/* workaround for AF volume gain tuning */
#define MM_SOUND_AF_FILE_PREFIX "/opt/ug/res/sounds/ug-camera-efl/sounds/af_"


/******************************************************************************************
	Functions For handling request from client
******************************************************************************************/
// except msgid
int _MMSoundMgrIpcPlayFile(char* filename,int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int keytone, int handle_route,
			   gboolean enable_session, int *codechandle)
{
	mmsound_mgr_codec_param_t param = {0,};
	MMSourceType *source = NULL;
	int ret = MM_ERROR_NONE;
	int mm_session_type = MM_SESSION_TYPE_MEDIA;

	/* Set source */
	source = (MMSourceType*)malloc(sizeof(MMSourceType));
	if (!source) {
		debug_error("malloc fail!!\n");
		return MM_ERROR_OUT_OF_MEMORY;
	}

	ret = mm_source_open_file(filename, source, MM_SOURCE_CHECK_DRM_CONTENTS);
	if(ret != MM_ERROR_NONE) {
		debug_error("Fail to open file\n");
		if (source) {
			free(source);
		}
		return ret;
	}

	/* Set sound player parameter */
	param.tone = tone;
	param.repeat_count = repeat;
	param.volume = volume;
	param.volume_config = volume_config;
	param.priority = priority;
	mm_session_type = session_type;
	param.keytone =  keytone;
	param.param = client_pid;
	param.source = source;
	param.handle_route = handle_route;
	param.enable_session = enable_session;

	/* workaround for AF volume gain tuning */
	if (strncmp(filename, MM_SOUND_AF_FILE_PREFIX, strlen(MM_SOUND_AF_FILE_PREFIX)) == 0) {
		param.volume_config |= VOLUME_GAIN_AF;
		debug_msg("Volume Gain AF\n");
	}

/*
	debug_msg("File[%s] DTMF[%d] Loop[%d] Volume[%f] Priority[%d] VolCfg[0x%x] callback[%p] param[%d] src_type[%d] src_ptr[%p] keytone[%d] route[%d] enable_session[%d]",
			filename,
			param.tone, param.repeat_count, param.volume, param.priority, param.volume_config, param.callback,
			(int)param.param, param.source->type, param.source->ptr, param.keytone, param.handle_route, param.enable_session);
			*/

	//convert mm_session_type to asm_event_type
	switch(mm_session_type)
	{
	case MM_SESSION_TYPE_MEDIA:
		param.session_type = ASM_EVENT_MEDIA_MMSOUND;
		break;
	case MM_SESSION_TYPE_NOTIFY:
		param.session_type = ASM_EVENT_NOTIFY;
		break;
	case MM_SESSION_TYPE_ALARM:
		param.session_type = ASM_EVENT_ALARM;
		break;
	case MM_SESSION_TYPE_EMERGENCY:
		param.session_type = ASM_EVENT_EMERGENCY;
		break;
	case MM_SESSION_TYPE_CALL:
		param.session_type = ASM_EVENT_CALL;
		break;
	case MM_SESSION_TYPE_VIDEOCALL:
		param.session_type = ASM_EVENT_VIDEOCALL;
		break;
	case MM_SESSION_TYPE_VOIP:
		param.session_type = ASM_EVENT_VOIP;
		break;
	default:
		debug_error("Unknown session type - use default shared type. %s %d\n", __FUNCTION__, __LINE__);
		param.session_type = ASM_EVENT_MEDIA_MMSOUND;
		break;
	}

	ret = MMSoundMgrCodecPlay(codechandle, &param);
	if (ret != MM_ERROR_NONE) {
		debug_error("Will be closed a sources, codechandle : 0x%08X\n", *codechandle);
		mm_source_close(source);
		if (source) {
			free(source);
			source = NULL;
		}
		return ret;
	}

	return MM_ERROR_NONE;
}

int _MMSoundMgrIpcStop(int handle)
{
	int ret = MM_ERROR_NONE;

	ret = MMSoundMgrCodecStop(handle);

	if (ret != MM_ERROR_NONE) {
		debug_error("Fail to stop sound\n");
		return ret;
	}

	return MM_ERROR_NONE;
}

int _MMSoundMgrIpcPlayDTMF(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   gboolean enable_session, int *codechandle)
{
	mmsound_mgr_codec_param_t param = {0,};
	int ret = MM_ERROR_NONE;

	/* Set sound player parameter */
	param.tone = tone;
	param.repeat_count = repeat;
	param.volume = volume;
	param.volume_config = volume_config;
	param.priority = 0;
	param.param = client_pid;
	param.session_options = session_options;
	param.enable_session = enable_session;

	//convert mm_session_type to asm_event_type
	switch(session_type)
	{
		case MM_SESSION_TYPE_MEDIA:
			param.session_type = ASM_EVENT_MEDIA_MMSOUND;
			break;
		case MM_SESSION_TYPE_NOTIFY:
			param.session_type = ASM_EVENT_NOTIFY;
			break;
		case MM_SESSION_TYPE_ALARM:
			param.session_type = ASM_EVENT_ALARM;
			break;
		case MM_SESSION_TYPE_EMERGENCY:
			param.session_type = ASM_EVENT_EMERGENCY;
			break;
		case MM_SESSION_TYPE_CALL:
			param.session_type = ASM_EVENT_CALL;
			break;
		case MM_SESSION_TYPE_VIDEOCALL:
			param.session_type = ASM_EVENT_VIDEOCALL;
			break;
		case MM_SESSION_TYPE_VOIP:
			param.session_type = ASM_EVENT_VOIP;
			break;
		default:
			debug_error("Unknown session type - use default media type. %s %d\n", __FUNCTION__, __LINE__);
			param.session_type = ASM_EVENT_MEDIA_MMSOUND;
			break;
	}

	debug_msg("DTMF %d\n", param.tone);
	debug_msg("Loop %d\n", param.repeat_count);
	debug_msg("Volume %d\n",param.volume);
	debug_msg("VolumeConfig %x\n",param.volume_config);
	debug_msg("Priority %d\n", param.priority);
	debug_msg("session %d\n", param.session_type);
	debug_msg("session options %x\n", param.session_options);
	debug_msg("enable_session %d\n", param.enable_session);

	ret = MMSoundMgrCodecPlayDtmf(codechandle, &param);
	if ( ret != MM_ERROR_NONE) {
		debug_error("Will be closed a sources, codec handle : [0x%d]\n", *codechandle);
		return ret;
	}

	return ret;
}

int __mm_sound_mgr_ipc_set_sound_path_for_active_device(mm_sound_device_in _device_in, mm_sound_device_out _device_out)
{
	int ret = MM_ERROR_NONE;
	mm_sound_device_in device_in = MM_SOUND_DEVICE_IN_NONE;
	mm_sound_device_out device_out = MM_SOUND_DEVICE_OUT_NONE;

	device_in = _device_in;
	device_out = _device_out;
	ret = _mm_sound_mgr_device_set_sound_path_for_active_device(device_out, device_in);

	return ret;
}

int __mm_sound_mgr_ipc_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	ret = _mm_sound_mgr_device_get_audio_path(device_in, device_out);

	return ret;
}

int __mm_sound_mgr_ipc_get_current_connected_device_list(int device_flags, mm_sound_device_t **device_list, int *total_num)
{
	int ret = MM_ERROR_NONE;

	ret = _mm_sound_mgr_device_get_current_connected_dev_list(device_flags, device_list, total_num);

	return ret;
}
#ifdef USE_FOCUS
// method + add callback
int __mm_sound_mgr_ipc_register_focus(int client_pid, int handle_id, char* stream_type)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = client_pid;
	param.handle_id = handle_id;
//	param.callback = msg->sound_msg.callback;
//	param.cbdata = msg->sound_msg.cbdata;
	memcpy(param.stream_type, stream_type, MAX_STREAM_TYPE_LEN);

	ret = mm_sound_mgr_focus_create_node(&param);

	return ret;
}


// method + remove callback
int __mm_sound_mgr_ipc_unregister_focus(int pid, int handle_id)
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
int __mm_sound_mgr_ipc_acquire_focus(int pid, int handle_id, int focus_type, const char* name )
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
int __mm_sound_mgr_ipc_release_focus(int pid, int handle_id, int focus_type, const char* name)
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

// method + add callback
int __mm_sound_mgr_ipc_watch_focus(int pid, int focus_type)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;
	param.request_type = focus_type;
//	param.callback = msg->sound_msg.callback;
//	param.cbdata = msg->sound_msg.cbdata;

	ret = mm_sound_mgr_focus_set_watch_cb(&param);

	return ret;
}

// method + remove callback
int __mm_sound_mgr_ipc_unwatch_focus(int pid)
{
	_mm_sound_mgr_focus_param_t param;
	int ret = MM_ERROR_NONE;

	memset(&param, 0x00, sizeof(_mm_sound_mgr_focus_param_t));
	param.pid = pid;

	ret = mm_sound_mgr_focus_unset_watch_cb(&param);

	return ret;
}
#endif

/************************************** ASM ***************************************/
int __mm_sound_mgr_ipc_asm_register_sound(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
#ifdef SUPPORT_CONTAINER
						const char* container_name, int container_pid,
#endif
					  int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					  int* request_id_r, int* sound_command_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_register_sound(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, sound_state_r);
#ifdef SUPPORT_CONTAINER
	_mm_sound_mgr_asm_update_container_data(*alloc_handle_r, container_name, container_pid);
#endif
	return ret;
}

int __mm_sound_mgr_ipc_asm_unregister_sound(int pid, int handle, int sound_event, int request_id, int sound_state, int resource)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_unregister_sound(pid, handle, sound_event, request_id, sound_state, resource);
	return ret;
}

int __mm_sound_mgr_ipc_asm_register_watcher(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
#ifdef SUPPORT_CONTAINER
						const char* container_name, int container_pid,
#endif
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_register_watcher(pid, handle, sound_event, request_id, sound_state, resource,
						  pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, sound_state_r);
#ifdef SUPPORT_CONTAINER
	_mm_sound_mgr_asm_update_container_data(*alloc_handle_r, container_name, container_pid);
#endif

	return ret;
}

int __mm_sound_mgr_ipc_asm_unregister_watcher(int pid, int handle, int sound_event, int request_id, int sound_state, int resource)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_unregister_watcher(pid, handle, sound_event, request_id, sound_state, resource);

	return ret;
}

int __mm_sound_mgr_ipc_asm_get_mystate(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_get_mystate(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_state_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_set_state(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* sound_state_r , int* error_code_r)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_set_state(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, sound_state_r, error_code_r);

	return ret;
}


int __mm_sound_mgr_ipc_asm_get_state(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_get_state(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_state_r);

	return ret;
}
int __mm_sound_mgr_ipc_asm_set_subsession(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r, int* request_id_r)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_set_subsession(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_get_subsession(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r, int* request_id_r, int* sound_command_r)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_get_subsession(pid, handle, sound_event, request_id, sound_state, resource,
						      pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_set_subevent(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_set_subevent(pid, handle, sound_event, request_id, sound_state, resource,
						   pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, sound_state_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_get_subevent(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_get_subevent(pid, handle, sound_event, request_id, sound_state, resource,
						   pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_set_session_option(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* error_code_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_set_session_option(pid, handle, sound_event, request_id, sound_state, resource,
						pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, error_code_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_get_session_option(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* option_flag_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_get_session_option(pid, handle, sound_event, request_id, sound_state, resource,
					     pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, option_flag_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_reset_resume_tag(int pid, int handle, int sound_event, int request_id, int sound_state, int resource,
					    int* pid_r, int* alloc_handle_r, int* cmd_handle_r,
					    int* request_id_r, int* sound_command_r, int* sound_state_r )
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_reset_resume_tag(pid, handle, sound_event, request_id, sound_state, resource,
					  pid_r, alloc_handle_r, cmd_handle_r, request_id_r, sound_command_r, sound_state_r);

	return ret;
}

int __mm_sound_mgr_ipc_asm_dump(int pid, int handle, int sound_event, int request_id, int sound_state, int resource)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_dump(pid, handle, sound_event, request_id, sound_state, resource);

	return ret;
}

int __mm_sound_mgr_ipc_asm_emergent_exit(int pid, int handle, int sound_event, int request_id, int sound_state)
{
	int ret = MM_ERROR_NONE;
	ret = _mm_sound_mgr_asm_emergent_exit(pid, handle, sound_event, request_id, sound_state);

	return ret;
}

/**********************************************************************************/


/******************************************************************************************
	Functions For Server-Side to notify Clients
******************************************************************************************/

int __mm_sound_mgr_ipc_notify_play_file_end (int handle)
{
	return __mm_sound_mgr_ipc_dbus_notify_play_file_end(handle);
}

int __mm_sound_mgr_ipc_notify_device_connected (mm_sound_device_t *device, gboolean is_connected)
{
	return __mm_sound_mgr_ipc_dbus_notify_device_connected(device, is_connected);
}

int __mm_sound_mgr_ipc_notify_device_info_changed (mm_sound_device_t *device, int changed_device_info_type)
{
	return __mm_sound_mgr_ipc_dbus_notify_device_info_changed(device, changed_device_info_type);
}

int __mm_sound_mgr_ipc_notify_volume_changed(unsigned int vol_type, unsigned int value)
{
	return __mm_sound_mgr_ipc_dbus_notify_volume_changed(vol_type, value);
}

int __mm_sound_mgr_ipc_notify_active_device_changed(int device_in, int device_out)
{
	/* Not Implemented */
	return __mm_sound_mgr_ipc_dbus_notify_active_device_changed(device_in, device_out);
}

int __mm_sound_mgr_ipc_notify_available_device_changed(int device_in, int device_out, int available)
{
	/* Not Implemented */
	return __mm_sound_mgr_ipc_dbus_notify_available_device_changed(device_in, device_out, available);
}

int __mm_sound_mgr_ipc_freeze_send (char* command, int pid)
{
	return mm_sound_mgr_ipc_dbus_send_signal_freeze ( command, pid);
}

/* should be converted to general type */
int _MMIpcCBSndMsg(mm_ipc_msg_t *msg)
{
//	return _MMIpcMsgqCBSndMsg(msg);
	return MM_ERROR_NONE;
}
int _MMIpcCBRecvMsg(mm_ipc_msg_t *msg)
{
//	return _MMIpcMsgqCBRecvMsg(msg);
	return MM_ERROR_NONE;
}
int _MMIpcCBMsgEnQueueAgain(mm_ipc_msg_t *msg)
{
//	return _MMIpcMsgqCBMsgEnQueueAgain(msg);
	return MM_ERROR_NONE;
}

int MMSoundMgrIpcInit(void)
{
	return MM_ERROR_NONE;
}

int MMSoundMgrIpcFini(void)
{
	return MM_ERROR_NONE;
}
