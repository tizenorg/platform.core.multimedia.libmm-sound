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
#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>

#define SHM_OPEN

/* workaround for AF volume gain tuning */
#define MM_SOUND_AF_FILE_PREFIX "/opt/ug/res/sounds/ug-camera-efl/sounds/af_"


/******************************************************************************************
	Functions For handling request from client
******************************************************************************************/
// except msgid
int _MMSoundMgrIpcPlayFile(char* filename,int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int handle_route,
			   gboolean enable_session, int *codechandle, char *stream_type, int stream_index)
{
	mmsound_mgr_codec_param_t param = {0,};
	MMSourceType *source = NULL;
	int ret = MM_ERROR_NONE;

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
	param.session_type = session_type;
	param.param = (void*)client_pid;
	param.source = source;
	param.handle_route = handle_route;
	param.enable_session = enable_session;
	param.stream_index = stream_index;
	MMSOUND_STRNCPY(param.stream_type, stream_type, MM_SOUND_STREAM_TYPE_LEN);

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

int _MMSoundMgrIpcClearFocus(int pid)
{
	int ret = MM_ERROR_NONE;

	ret = MMSoundMgrCodecClearFocus(pid);

	if (ret != MM_ERROR_NONE) {
		debug_error("Fail to clear focus\n");
		return ret;
	}

	return MM_ERROR_NONE;
}

int _MMSoundMgrIpcPlayFileWithStreamInfo(char* filename, int repeat, int volume,
			   int priority, int client_pid, int handle_route, int *codechandle, char *stream_type, int stream_index)
{
	mmsound_mgr_codec_param_t param = {0,};
	MMSourceType *source = NULL;
	int ret = MM_ERROR_NONE;

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
	param.repeat_count = repeat;
	param.volume = volume;
	param.priority = priority;
	param.param = (void*)client_pid;
	param.source = source;
	param.handle_route = handle_route;
	param.stream_index = stream_index;
	MMSOUND_STRNCPY(param.stream_type, stream_type, MM_SOUND_STREAM_TYPE_LEN);

	/* workaround for AF volume gain tuning */
	if (strncmp(filename, MM_SOUND_AF_FILE_PREFIX, strlen(MM_SOUND_AF_FILE_PREFIX)) == 0) {
		param.volume_config |= VOLUME_GAIN_AF;
		debug_msg("Volume Gain AF\n");
	}

	ret = MMSoundMgrCodecPlayWithStreamInfo(codechandle, &param);
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

int _MMSoundMgrIpcPlayDTMF(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   gboolean enable_session, int *codechandle, char *stream_type, int stream_index)
{
	mmsound_mgr_codec_param_t param = {0,};
	int ret = MM_ERROR_NONE;

	/* Set sound player parameter */
	param.tone = tone;
	param.repeat_count = repeat;
	param.volume = volume;
	param.volume_config = volume_config;
	param.priority = 0;
	param.param = (void*)client_pid;
	param.session_type = session_type;
	param.session_options = session_options;
	param.enable_session = enable_session;
	param.stream_index = stream_index;
	MMSOUND_STRNCPY(param.stream_type, stream_type, MM_SOUND_STREAM_TYPE_LEN);

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

int _MMSoundMgrIpcPlayDTMFWithStreamInfo(int tone, int repeat, int volume, int client_pid, int *codechandle, char *stream_type, int stream_index)
{
	mmsound_mgr_codec_param_t param = {0,};
	int ret = MM_ERROR_NONE;

	/* Set sound player parameter */
	param.tone = tone;
	param.repeat_count = repeat;
	param.volume = volume;
	param.priority = 0;
	param.param = (void*)client_pid;
	param.stream_index = stream_index;
	MMSOUND_STRNCPY(param.stream_type, stream_type, MM_SOUND_STREAM_TYPE_LEN);

	debug_msg("DTMF %d\n", param.tone);
	debug_msg("Loop %d\n", param.repeat_count);
	debug_msg("Volume %d\n",param.volume);
	debug_msg("Priority %d\n", param.priority);
	debug_msg("stream type %s\n", param.stream_type);
	debug_msg("stream index %d\n", param.stream_index);


	ret = MMSoundMgrCodecPlayDtmfWithStreamInfo(codechandle, &param);
	if ( ret != MM_ERROR_NONE) {
		debug_error("Will be closed a sources, codec handle : [0x%d]\n", *codechandle);
		return ret;
	}

	return ret;
}


int __mm_sound_mgr_ipc_set_sound_path_for_active_device(mm_sound_device_in _device_in, mm_sound_device_out _device_out)
{
	int ret = MM_ERROR_NONE;
#if 0 /* FIXME */
	mm_sound_device_in device_in = MM_SOUND_DEVICE_IN_NONE;
	mm_sound_device_out device_out = MM_SOUND_DEVICE_OUT_NONE;

	device_in = _device_in;
	device_out = _device_out;
	ret = _mm_sound_mgr_device_set_sound_path_for_active_device(device_out, device_in);
#endif

	return ret;
}

int __mm_sound_mgr_ipc_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

//	ret = _mm_sound_mgr_device_get_audio_path(device_in, device_out);

	return ret;
}

int __mm_sound_mgr_ipc_get_current_connected_device_list(int device_flags, mm_sound_device_t **device_list, int *total_num)
{
	int ret = MM_ERROR_NONE;

//	ret = _mm_sound_mgr_device_get_current_connected_dev_list(device_flags, device_list, total_num);

	return ret;
}

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
