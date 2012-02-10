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
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <errno.h>
#include <avsystem.h>

#include <vconf.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_debug.h>
#include "include/mm_sound_private.h"
#include "include/mm_sound.h"
#include "include/mm_sound_client.h"
#include "include/mm_ipc.h"
#include "include/mm_sound_common.h"


#include <audio-session-manager.h>
#include <mm_session.h>
#include <mm_session_private.h>

#define MAX_FILE_LENGTH 256
#define MAX_MEMORY_SIZE 1048576	/* Max memory size 1024*1024 (1MB) */
#define _MIN_SYSTEM_SAMPLERATE	8000
#define _MAX_SYSTEM_SAMPLERATE	48000
#define MIN_TONE_PLAY_TIME 300

typedef struct {
	volume_callback_fn	func;
	void*				data;
	volume_type_t		type;
}volume_cb_param;

volume_cb_param g_volume_param[VOLUME_TYPE_MAX];

static pthread_mutex_t _volume_mutex = PTHREAD_MUTEX_INITIALIZER;

int _validate_volume(volume_type_t type, int value)
{
	if (value < 0)
		return -1;

	switch (type)
	{
	case VOLUME_TYPE_ALARM:
	case VOLUME_TYPE_CALL:
		if (value >= AVSYS_AUDIO_VOLUME_MAX_BASIC) {
			return -1;
		}
		break;
	case VOLUME_TYPE_SYSTEM:
	case VOLUME_TYPE_MEDIA:
	case VOLUME_TYPE_EXT_JAVA:
	case VOLUME_TYPE_NOTIFICATION:
	case VOLUME_TYPE_RINGTONE:
		if (value >= AVSYS_AUDIO_VOLUME_MAX_MULTIMEDIA) {
			return -1;
		}
		break;
	case VOLUME_TYPE_EXT_ANDROID:
		if (value >= AVSYS_AUDIO_VOLUME_MAX_SINGLE) {
			return -1;
		}
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

void volume_changed_cb(keynode_t* node, void* data)
{
	volume_cb_param* param = (volume_cb_param*) data;

	debug_msg("%s changed callback called\n",vconf_keynode_get_name(node));

	MMSOUND_ENTER_CRITICAL_SECTION( &_volume_mutex )

	if(param && (param->func != NULL)) {
		debug_msg("funcion 0x%x\n", param->func);
		((volume_callback_fn)param->func)(param->data);
	}

	MMSOUND_LEAVE_CRITICAL_SECTION( &_volume_mutex )
}

EXPORT_API
int mm_sound_volume_add_callback(volume_type_t type, volume_callback_fn func, void* user_data)
{
	char *keystr[] = {VCONF_KEY_VOLUME_TYPE_SYSTEM, VCONF_KEY_VOLUME_TYPE_NOTIFICATION, VCONF_KEY_VOLUME_TYPE_ALARM,
			VCONF_KEY_VOLUME_TYPE_RINGTONE, VCONF_KEY_VOLUME_TYPE_MEDIA, VCONF_KEY_VOLUME_TYPE_CALL,
			VCONF_KEY_VOLUME_TYPE_ANDROID,VCONF_KEY_VOLUME_TYPE_JAVA};

	debug_fenter();

	/* Check input param */
	if(type < VOLUME_TYPE_SYSTEM || type >=VOLUME_TYPE_MAX) {
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(!func) {
		debug_warning("callback function is null\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN( &_volume_mutex, MM_ERROR_SOUND_INTERNAL )

	g_volume_param[type].func = func;
	g_volume_param[type].data = user_data;
	g_volume_param[type].type = type;

	MMSOUND_LEAVE_CRITICAL_SECTION( &_volume_mutex )

	return vconf_notify_key_changed(keystr[type], volume_changed_cb, (void*)&g_volume_param[type]);
}

EXPORT_API
int mm_sound_volume_remove_callback(volume_type_t type)
{
	char *keystr[] = {VCONF_KEY_VOLUME_TYPE_SYSTEM, VCONF_KEY_VOLUME_TYPE_NOTIFICATION, VCONF_KEY_VOLUME_TYPE_ALARM,
			VCONF_KEY_VOLUME_TYPE_RINGTONE, VCONF_KEY_VOLUME_TYPE_MEDIA, VCONF_KEY_VOLUME_TYPE_CALL,
			VCONF_KEY_VOLUME_TYPE_ANDROID,VCONF_KEY_VOLUME_TYPE_JAVA};
	debug_fenter();

	if(type < VOLUME_TYPE_SYSTEM || type >=VOLUME_TYPE_MAX) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN( &_volume_mutex, MM_ERROR_SOUND_INTERNAL )

	g_volume_param[type].func = NULL;
	g_volume_param[type].data = NULL;
	g_volume_param[type].type = type;

	MMSOUND_LEAVE_CRITICAL_SECTION( &_volume_mutex )

	return vconf_ignore_key_changed(keystr[type], volume_changed_cb);
}

EXPORT_API
int mm_sound_get_volume_step(volume_type_t type, int *step)
{
	printf("\n**********\n\nTHIS FUNCTION HAS DEFPRECATED [%s]\n\n \
			use mm_sound_volume_get_step() instead\n\n**********\n", __func__);
	return mm_sound_volume_get_step(type, step);
}

EXPORT_API
int mm_sound_volume_get_step(volume_type_t type, int *step)
{
	int err;

	debug_fenter();

	/* Check input param */
	if(step == NULL) {
		debug_error("second parameter is null\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(type < VOLUME_TYPE_SYSTEM || type >= VOLUME_TYPE_MAX) {
		debug_error("Invalid type value %d\n", (int)type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	err = avsys_audio_get_volume_max_ex((int)type, step);
	if (AVSYS_FAIL(err)) {
		err = MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fleave();
	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_volume_set_value(volume_type_t type, const unsigned int value)
{
	int ret = MM_ERROR_NONE;
	char *keystr[] = {VCONF_KEY_VOLUME_TYPE_SYSTEM, VCONF_KEY_VOLUME_TYPE_NOTIFICATION, VCONF_KEY_VOLUME_TYPE_ALARM,
			VCONF_KEY_VOLUME_TYPE_RINGTONE, VCONF_KEY_VOLUME_TYPE_MEDIA, VCONF_KEY_VOLUME_TYPE_CALL,
			VCONF_KEY_VOLUME_TYPE_ANDROID,VCONF_KEY_VOLUME_TYPE_JAVA};

	debug_fenter();

	/* Check input param */
	if(0 > _validate_volume(type, (int)value)) {
		debug_error("invalid volume type %d, value %u\n", type, value);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Set volume value to VCONF */
	if(vconf_set_int(keystr[type], value)) {
		debug_error("Can not set %s as %d\n", keystr[type], value);
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		/* update shared memory value */
		ret = avsys_audio_set_volume_by_type(type, value);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set volume to shared memory 0x%x\n", ret);
		}
	}

	debug_fleave();
	return ret;
}

EXPORT_API
int mm_sound_volume_get_value(volume_type_t type, unsigned int *value)
{
	int ret = MM_ERROR_NONE;
	char *keystr[] = {VCONF_KEY_VOLUME_TYPE_SYSTEM, VCONF_KEY_VOLUME_TYPE_NOTIFICATION, VCONF_KEY_VOLUME_TYPE_ALARM,
			VCONF_KEY_VOLUME_TYPE_RINGTONE, VCONF_KEY_VOLUME_TYPE_MEDIA, VCONF_KEY_VOLUME_TYPE_CALL,
			VCONF_KEY_VOLUME_TYPE_ANDROID,VCONF_KEY_VOLUME_TYPE_JAVA};

	debug_fenter();

	/* Check input param */
	if(value == NULL)
		return MM_ERROR_INVALID_ARGUMENT;
	if(type < 0 || type >= VOLUME_TYPE_MAX) {
		debug_error("invalid volume type value %d\n", type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Get volume value from VCONF */
	if(vconf_get_int(keystr[type], (int*)value)) {
		debug_error("Can not get value of %s\n", keystr[type]);
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();
	return ret;
}

EXPORT_API
int mm_sound_volume_primary_type_set(volume_type_t type)
{
	pid_t mypid;
	int ret = MM_ERROR_NONE;

	/* Check input param */
	if(type < VOLUME_TYPE_SYSTEM || type >= VOLUME_TYPE_MAX)
		return MM_ERROR_INVALID_ARGUMENT;

	debug_fenter();

	mypid = getpid();
	if(AVSYS_FAIL(avsys_audio_set_primary_volume((int)mypid, type))) {
		debug_error("Can not set primary volume [%d, %d]\n", mypid, type);
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();
	return ret;
}

EXPORT_API
int mm_sound_volume_primary_type_clear()
{
	pid_t mypid;
	int ret = MM_ERROR_NONE;

	debug_fenter();

	mypid = getpid();
	if(AVSYS_FAIL(avsys_audio_clear_primary_volume((int)mypid))) {
		debug_error("Can not clear primary volume [%d]\n", mypid);
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();
	return ret;
}

EXPORT_API
int mm_sound_volume_get_current_playing_type(volume_type_t *type)
{
	int result = AVSYS_STATE_SUCCESS;
	int voltype = AVSYS_AUDIO_VOLUME_TYPE_SYSTEM;

	debug_fenter();

	/* Check input param */
	if(type == NULL) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = avsys_audio_get_current_playing_volume_type(&voltype);
	if(result == AVSYS_STATE_SUCCESS) {
		*type = voltype;
		return MM_ERROR_NONE;
	} else if(result ==AVSYS_STATE_ERR_ALLOCATION ) {
		return MM_ERROR_SOUND_VOLUME_NO_INSTANCE;
	} else if(result == AVSYS_STATE_ERR_INVALID_MODE) {
		return MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY;
	} else {
		return MM_ERROR_SOUND_INTERNAL;
	}
}

///////////////////////////////////
////     MMSOUND PCM APIs
///////////////////////////////////

typedef struct {
	avsys_handle_t		audio_handle;
	int					asm_handle;
	ASM_sound_events_t	asm_event;
	int					asm_valid_flag;

	MMMessageCallback	msg_cb;
	void *msg_cb_param;

} mm_sound_pcm_t;

int _get_asm_event_type(ASM_sound_events_t *type)
{
	int	sessionType = MM_SESSION_TYPE_SHARE;
	ASM_sound_events_t asm_event;

	if(type == NULL)
		return MM_ERROR_SOUND_INVALID_POINTER;

	/* read session type */
	if(_mm_session_util_read_type(-1, &sessionType) < 0) {
		debug_log("Read Session Type failed. Set default \"Share\" type\n");
		sessionType = MM_SESSION_TYPE_SHARE;
		if(mm_session_init(sessionType) < 0) {
			debug_error("mm_session_init() failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
	}

	/* convert MM_SESSION_TYPE to ASM_EVENT_TYPE */
	switch (sessionType)
	{
	case MM_SESSION_TYPE_SHARE:
		asm_event = ASM_EVENT_SHARE_MMSOUND;
		break;
	case MM_SESSION_TYPE_EXCLUSIVE:
		asm_event = ASM_EVENT_EXCLUSIVE_MMSOUND;
		break;
	case MM_SESSION_TYPE_NOTIFY:
		asm_event = ASM_EVENT_NOTIFY;
		break;
	case MM_SESSION_TYPE_ALARM:
		asm_event = ASM_EVENT_ALARM;
		break;
	case MM_SESSION_TYPE_CALL:
		asm_event = ASM_EVENT_CALL;
		break;
	case MM_SESSION_TYPE_VIDEOCALL:
		asm_event = ASM_EVENT_VIDEOCALL;
		break;
	default:
		debug_error("Unexpected %d\n", sessionType);
		return MM_ERROR_SOUND_INTERNAL;
	}

	*type = asm_event;
	return MM_ERROR_NONE;
}

void __sound_pcm_send_message (mm_sound_pcm_t *pcmHandle, int message, int code)
{
	int ret = 0;
	if (pcmHandle->msg_cb) {
		MMMessageParamType msg;
		msg.union_type = MM_MSG_UNION_CODE;
		msg.code = code;

		debug_log ("calling msg callback(%p) with message(%d), code(%d), msg callback param(%p)\n",
				pcmHandle->msg_cb, message, msg.code, pcmHandle->msg_cb_param);
		ret = pcmHandle->msg_cb(message, &msg, pcmHandle->msg_cb_param);
		debug_log ("msg callback returned (%d)\n", ret);
	} else {
		debug_log ("No pcm msg callback\n");
	}
}

ASM_cb_result_t sound_pcm_asm_callback(int handle, ASM_event_sources_t event_src, ASM_sound_commands_t command, unsigned int sound_status, void *cb_data)
{
	mm_sound_pcm_t *pcmHandle = NULL;
	ASM_cb_result_t	cb_res = ASM_CB_RES_IGNORE;

	/* Check input param */
	pcmHandle = (mm_sound_pcm_t *)cb_data;
	if(pcmHandle == NULL) {
		debug_error("sound_pcm_asm_callback cb_data is null\n");
		return cb_res;
	}

	debug_log ("command = %d, handle = %p, asm_valid_flag = %d\n",command, pcmHandle, pcmHandle->asm_valid_flag);
	switch(command)
	{
	case ASM_COMMAND_STOP:
	case ASM_COMMAND_PAUSE:
		pcmHandle->asm_valid_flag = 0;
		cb_res = ASM_CB_RES_PAUSE;
		break;
	case ASM_COMMAND_PLAY:
	case ASM_COMMAND_RESUME:
		pcmHandle->asm_valid_flag = 1;
		cb_res = ASM_CB_RES_PLAYING;
		break;
	}

	/* execute user callback if callback available */
	__sound_pcm_send_message (pcmHandle, MM_MESSAGE_SOUND_PCM_INTERRUPTED, event_src);

	return cb_res;
}

EXPORT_API
int mm_sound_pcm_capture_open(MMSoundPcmHandle_t *handle, const unsigned int rate, MMSoundPcmChannel_t channel, MMSoundPcmFormat_t format)
{
	avsys_audio_param_t param;
	mm_sound_pcm_t *pcmHandle = NULL;
	int size = 0;
	int result = AVSYS_STATE_SUCCESS;
	int errorcode = 0;

	debug_fenter();
	memset(&param, 0, sizeof(avsys_audio_param_t));

	if (rate < _MIN_SYSTEM_SAMPLERATE || rate > _MAX_SYSTEM_SAMPLERATE) {
		debug_error("unsupported sample rate %u", rate);
		return MM_ERROR_SOUND_DEVICE_INVALID_SAMPLERATE;
	} else {
		param.samplerate = rate;
	}

	switch(channel)
	{
	case MMSOUND_PCM_MONO:
		param.channels = 1;
		break;
	case MMSOUND_PCM_STEREO:
		param.channels = 2;
		break;

	default:
		debug_error("Unsupported channel type\n");
		return MM_ERROR_SOUND_DEVICE_INVALID_CHANNEL;
	}

	switch(format)
	{
	case MMSOUND_PCM_U8:
		param.format = AVSYS_AUDIO_FORMAT_8BIT;
		break;
	case MMSOUND_PCM_S16_LE:
		param.format = AVSYS_AUDIO_FORMAT_16BIT;
		break;
	default:
		debug_error("Unsupported format type\n");
		return MM_ERROR_SOUND_DEVICE_INVALID_FORMAT;
	}

	pcmHandle = calloc(sizeof(mm_sound_pcm_t), 1);
	if(pcmHandle == NULL)
		return MM_ERROR_OUT_OF_MEMORY;

	/* Register ASM */
	/* get session type */
	if(MM_ERROR_NONE != _get_asm_event_type(&pcmHandle->asm_event)) {
		free(pcmHandle);
		return MM_ERROR_POLICY_INTERNAL;
	}
	/* register asm as playing */
	if(pcmHandle->asm_event != ASM_EVENT_CALL && pcmHandle->asm_event != ASM_EVENT_VIDEOCALL) {
		if(!ASM_register_sound(-1, &pcmHandle->asm_handle, pcmHandle->asm_event,
				ASM_STATE_PLAYING, sound_pcm_asm_callback, (void*)pcmHandle, ASM_RESOURCE_NONE, &errorcode))
		{
			debug_error("ASM_register_sound() failed 0x%x\n", errorcode);
			free(pcmHandle);
			return MM_ERROR_POLICY_BLOCKED;
		}
	}
	pcmHandle->asm_valid_flag = 1;

	/* Open */
	param.mode = AVSYS_AUDIO_MODE_INPUT;
	param.vol_type = AVSYS_AUDIO_VOLUME_TYPE_SYSTEM; //dose not effect at capture mode
	param.priority = AVSYS_AUDIO_PRIORITY_0;		//This does not affect anymore.
	result = avsys_audio_open(&param, &pcmHandle->audio_handle, &size);
	if(AVSYS_FAIL(result)) {
		debug_error("Device Open Error 0x%x\n", result);
		free(pcmHandle);
		return MM_ERROR_SOUND_DEVICE_NOT_OPENED;
	}

	/* Set handle to return */
	*handle = (MMSoundPcmHandle_t)pcmHandle;

	debug_fleave();
	return size;
}

EXPORT_API
int mm_sound_pcm_capture_read(MMSoundPcmHandle_t handle, void *buffer, const unsigned int length )
{
	mm_sound_pcm_t *pcmHandle = NULL;

	/* Check input param */
	pcmHandle = (mm_sound_pcm_t*)handle;
	if(pcmHandle == NULL)
		return MM_ERROR_INVALID_ARGUMENT;
	if(buffer == NULL) {
		debug_error("Invalid buffer pointer\n");
		return MM_ERROR_SOUND_INVALID_POINTER;
	}
	if(length == 0 )
		return 0;

	/* Check ASM */
	if(!pcmHandle->asm_valid_flag) {
		return MM_ERROR_POLICY_INTERRUPTED;
	}

	if(length == 0 )
		return 0;

	/* Read */
	return avsys_audio_read(pcmHandle->audio_handle, buffer, length);
}

EXPORT_API
int mm_sound_pcm_capture_close(MMSoundPcmHandle_t handle)
{
	int result = MM_ERROR_NONE;
	mm_sound_pcm_t *pcmHandle = NULL;
	int errorcode = 0;

	debug_fenter();

	/* Check input param */
	pcmHandle = (mm_sound_pcm_t*)handle;
	if(pcmHandle == NULL)
		return MM_ERROR_INVALID_ARGUMENT;

	/* Close */
	result = avsys_audio_close(pcmHandle->audio_handle);
	if(AVSYS_FAIL(result)) {
		debug_error("handle close failed 0x%X", result);
		result = MM_ERROR_SOUND_INTERNAL;
	}

	/* Unregister ASM */
	if(pcmHandle->asm_event != ASM_EVENT_CALL && pcmHandle->asm_event != ASM_EVENT_VIDEOCALL) {
		if(!ASM_unregister_sound(pcmHandle->asm_handle, pcmHandle->asm_event, &errorcode)) {
    		debug_error("ASM_unregister failed in %s with 0x%x\n", __func__, errorcode);
    	}
    	pcmHandle->asm_valid_flag = 0;
    }

	/* Free handle */
	free(pcmHandle);    pcmHandle= NULL;

	debug_fleave();
	return result;
}

EXPORT_API
int mm_sound_pcm_set_message_callback (MMSoundPcmHandle_t handle, MMMessageCallback callback, void *user_param)
{
	mm_sound_pcm_t *pcmHandle =  (mm_sound_pcm_t*)handle;

	if(pcmHandle == NULL || callback == NULL)
		return MM_ERROR_INVALID_ARGUMENT;

	pcmHandle->msg_cb = callback;
	pcmHandle->msg_cb_param = user_param;

	debug_log ("set pcm message callback (%p,%p)\n", callback, user_param);

	return MM_ERROR_NONE;
}


EXPORT_API
int mm_sound_pcm_play_open_ex (MMSoundPcmHandle_t *handle, const unsigned int rate, MMSoundPcmChannel_t channel, MMSoundPcmFormat_t format, const volume_type_t vol_type, ASM_sound_events_t asm_event)
{
	avsys_audio_param_t param;
	mm_sound_pcm_t *pcmHandle = NULL;
	int size = 0;
	int result = AVSYS_STATE_SUCCESS;
	int lvol_type = vol_type;
	int errorcode = 0;

	debug_fenter();
	memset(&param, 0, sizeof(avsys_audio_param_t));

	/* Check input param */
	if(vol_type < 0) {
		debug_error("Volume type should not be negative value\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(vol_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type should be under VOLUME_TYPE_MAX\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(rate < _MIN_SYSTEM_SAMPLERATE || rate > _MAX_SYSTEM_SAMPLERATE) {
		debug_error("unsupported sample rate %u", rate);
		return MM_ERROR_SOUND_DEVICE_INVALID_SAMPLERATE;
	} else {
		param.samplerate = rate;
	}

	switch(channel)
	{
	case MMSOUND_PCM_MONO:
		param.channels = 1;
		break;
	case MMSOUND_PCM_STEREO:
		param.channels = 2;
		break;
	default:
		debug_error("Unsupported channel type\n");
		return MM_ERROR_SOUND_DEVICE_INVALID_CHANNEL;
	}

	switch(format)
	{
	case MMSOUND_PCM_U8:
		param.format = AVSYS_AUDIO_FORMAT_8BIT;
		break;
	case MMSOUND_PCM_S16_LE:
		param.format = AVSYS_AUDIO_FORMAT_16BIT;
		break;
	default:
		debug_error("Unsupported format type\n");
		return MM_ERROR_SOUND_DEVICE_INVALID_FORMAT;
	}

	pcmHandle = calloc(sizeof(mm_sound_pcm_t),1);
	if(pcmHandle == NULL)
		return MM_ERROR_OUT_OF_MEMORY;

	/* Register ASM */
	debug_log ("session start : input asm_event = %d-------------\n", asm_event);
	//get session type
	if (asm_event == ASM_EVENT_NONE) {
		if(MM_ERROR_NONE != _get_asm_event_type(&pcmHandle->asm_event)) {
			free(pcmHandle);
			return MM_ERROR_POLICY_INTERNAL;
		}
		//register asm as playing
		if(pcmHandle->asm_event != ASM_EVENT_CALL && pcmHandle->asm_event != ASM_EVENT_VIDEOCALL) {
			if(!ASM_register_sound(-1, &pcmHandle->asm_handle, pcmHandle->asm_event,
					ASM_STATE_PLAYING, sound_pcm_asm_callback, (void*)pcmHandle, ASM_RESOURCE_NONE, &errorcode)) {
				debug_error("ASM_register_sound() failed 0x%x\n", errorcode);
				free(pcmHandle);
				return MM_ERROR_POLICY_BLOCKED;
			}
		}
	} else {
		if(!ASM_register_sound(-1, &pcmHandle->asm_handle, asm_event,
				ASM_STATE_PLAYING, NULL, (void*)pcmHandle, ASM_RESOURCE_NONE, &errorcode)) {
			debug_error("ASM_register_sound() failed 0x%x\n", errorcode);
			free(pcmHandle);
			return MM_ERROR_POLICY_BLOCKED;
		}
	}
	pcmHandle->asm_valid_flag = 1;

	param.mode = AVSYS_AUDIO_MODE_OUTPUT;
	param.vol_type = lvol_type;
	param.priority = AVSYS_AUDIO_PRIORITY_0;

//	avsys_audio_ampon();

	/* Open */
	debug_log ("avsys open -------------\n");
	result = avsys_audio_open(&param, &pcmHandle->audio_handle, &size);
	if(AVSYS_FAIL(result)) {
		debug_error("Device Open Error 0x%x\n", result);
		free(pcmHandle);
		return MM_ERROR_SOUND_DEVICE_NOT_OPENED;
	}

	/* Set handle to return */
	*handle = (MMSoundPcmHandle_t)pcmHandle;

	debug_fleave();
	return size;
}

EXPORT_API
int mm_sound_pcm_play_open(MMSoundPcmHandle_t *handle, const unsigned int rate, MMSoundPcmChannel_t channel, MMSoundPcmFormat_t format, const volume_type_t vol_type)
{
	return mm_sound_pcm_play_open_ex (handle, rate, channel, format, vol_type, ASM_EVENT_NONE);
}

EXPORT_API
int mm_sound_pcm_play_write(MMSoundPcmHandle_t handle, void* ptr, unsigned int length_byte)
{
	mm_sound_pcm_t *pcmHandle = NULL;

	/* Check input param */
	pcmHandle = (mm_sound_pcm_t*)handle;
	if(pcmHandle == NULL)
		return MM_ERROR_INVALID_ARGUMENT;
	if(ptr == NULL) {
		debug_error("Invalid buffer pointer\n");
		return MM_ERROR_SOUND_INVALID_POINTER;
	}
	if(length_byte == 0 )
		return 0;

	/* Check ASM */
	if(!pcmHandle->asm_valid_flag)
		return MM_ERROR_POLICY_INTERRUPTED;

	/* Write */
	return avsys_audio_write(pcmHandle->audio_handle, ptr, length_byte);
}

EXPORT_API
int mm_sound_pcm_play_close(MMSoundPcmHandle_t handle)
{
	int result = AVSYS_STATE_SUCCESS;
	mm_sound_pcm_t *pcmHandle = NULL;
	int errorcode = 0;

	/* Check input param */
	pcmHandle = (mm_sound_pcm_t*)handle;
	if(pcmHandle == NULL)
		return MM_ERROR_INVALID_ARGUMENT;

	/* Drain */
	if(AVSYS_FAIL(avsys_audio_drain(pcmHandle->audio_handle))) {
		debug_error("drain failed\n");
	}

	/* Close */
	result = avsys_audio_close(pcmHandle->audio_handle);
	if(AVSYS_FAIL(result)) {
		debug_error("handle close failed 0x%X", result);
		result = MM_ERROR_SOUND_INTERNAL;
	}

	/* Unregister ASM */
	if(pcmHandle->asm_event != ASM_EVENT_CALL && pcmHandle->asm_event != ASM_EVENT_VIDEOCALL) {
		if(!ASM_unregister_sound(pcmHandle->asm_handle, pcmHandle->asm_event, &errorcode)) {
    		debug_error("ASM_unregister failed in %s with 0x%x\n",__func__, errorcode);
    	}
    	pcmHandle->asm_valid_flag = 0;
    }

	/* Free handle */
    free(pcmHandle);    pcmHandle= NULL;

	return result;
}

///////////////////////////////////
////     MMSOUND PLAY APIs
///////////////////////////////////
EXPORT_API
int mm_sound_play_loud_solo_sound(const char *filename, const volume_type_t volume_type, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundParamType param = { 0, };
	int err;
	int lhandle = -1;
	int lvol_type = volume_type;

	debug_fenter();

	/* Check input param */
	if(filename == NULL) {
		debug_error("filename is NULL\n");
		return MM_ERROR_SOUND_FILE_NOT_FOUND;
	}
	if(volume_type < 0 || volume_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type should not be negative value\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play sound */
	param.filename = filename;
	param.volume = 0; //volume value dose not effect anymore
	param.callback = callback;
	param.data = data;
	param.loop = 1;
	param.volume_table = lvol_type;
	param.priority = AVSYS_AUDIO_PRIORITY_SOLO;
	param.bluetooth = MMSOUNDPARAM_SPEAKER_ONLY;

	err = MMSoundClientPlaySound(&param, 0, 0, &lhandle);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	/* Set handle to return */
	if (handle) {
		*handle = lhandle;
	} else {
		debug_critical("The sound handle cannot be get [%d]\n", lhandle);
	}

	debug_fleave();
	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_play_solo_sound(const char *filename, const volume_type_t volume_type, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundParamType param = { 0, };
	int err;
	int lhandle = -1;
	int lvol_type = volume_type;

	debug_fenter();

	/* Check input param */
	if(filename == NULL) {
		debug_error("filename is NULL\n");
		return MM_ERROR_SOUND_FILE_NOT_FOUND;
	}
	if(volume_type < 0 || volume_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type should not be negative value\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play sound */
	param.filename = filename;
	param.volume = 0; /* volume value dose not effect anymore */
	param.callback = callback;
	param.data = data;
	param.loop = 1;
	param.volume_table = lvol_type;
	param.priority = AVSYS_AUDIO_PRIORITY_SOLO;
	param.bluetooth = MMSOUNDPARAM_FOLLOWING_ROUTE_POLICY;

	err = MMSoundClientPlaySound(&param, 0, 0, &lhandle);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	/* Set handle to return */
	if (handle) {
		*handle = lhandle;
	} else {
		debug_critical("The sound handle cannot be get [%d]\n", lhandle);
	}

	debug_fleave();
	return MM_ERROR_NONE;
}


EXPORT_API
int mm_sound_play_sound(const char *filename, const volume_type_t volume_type, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundParamType param = { 0, };
	int err;
	int lhandle = -1;
	int lvol_type = volume_type;

	debug_fenter();

	/* Check input param */
	if(filename == NULL) {
		debug_error("filename is NULL\n");
		return MM_ERROR_SOUND_FILE_NOT_FOUND;
	}
	if(volume_type < 0) {
		debug_error("Volume type should not be negative value\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(volume_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type should be under VOLUME_TYPE_MAX\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play sound */
	param.filename = filename;
	param.volume = 0; /* volume value dose not effect anymore */
	param.callback = callback;
	param.data = data;
	param.loop = 1;
	param.volume_table = lvol_type;
	param.priority = AVSYS_AUDIO_PRIORITY_NORMAL;
	param.bluetooth = AVSYS_AUDIO_HANDLE_ROUTE_FOLLOWING_POLICY;

	err = MMSoundClientPlaySound(&param, 0, 0, &lhandle);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	/* Set handle to return */
	if (handle) {
		*handle = lhandle;
	} else {
		debug_critical("The sound handle cannot be get [%d]\n", lhandle);
	}

	debug_fleave();
	return MM_ERROR_NONE;
}


EXPORT_API
int mm_sound_play_sound_ex(MMSoundParamType *param, int *handle)
{
	int err;
	int lhandle = -1;

	debug_fenter();

	/* Check input param */
	if (param == NULL) {
		debug_error("param is null\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play sound */
	err = MMSoundClientPlaySound(param, 0, 0, &lhandle);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	/* Set handle to return */
	if (handle) {
		*handle = lhandle;
	} else {
		debug_critical("The sound hadle cannot be get [%d]\n", lhandle);
	}

	debug_fleave();
	return MM_ERROR_NONE;
}


EXPORT_API
int mm_sound_stop_sound(int handle)
{
	int err;

	debug_fenter();

	/* Stop sound */
	err = MMSoundClientStopSound(handle);
	if (err < 0) {
		debug_error("Fail to stop sound\n");
		return err;
	}

	debug_fleave();
	return MM_ERROR_NONE;
}

///////////////////////////////////
////     MMSOUND TONE APIs
///////////////////////////////////
EXPORT_API
int mm_sound_play_tone (MMSoundTone_t num, const volume_type_t vol_type, const double volume, const int duration, int *handle)
{
	int lhandle = -1;
	int err = MM_ERROR_NONE;

	debug_fenter();

	/* Check input param */
	if(duration < -1) {
		debug_error("number is invalid %d\n", duration);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(num < MM_SOUND_TONE_DTMF_0 || num >= MM_SOUND_TONE_NUM) {
		debug_error("TONE Value is invalid %d\n", num);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(vol_type < VOLUME_TYPE_SYSTEM || vol_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume Type is invalid %d\n", vol_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(volume < 0.0 || volume > 1.0) {
		debug_error("Volume Value is invalid %d\n", vol_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play tone */
	debug_msg("Call MMSoundClientPlayTone\n");
	err = MMSoundClientPlayTone (num, vol_type, volume, duration, &lhandle);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	/* Set handle to return */
	if (handle)
		*handle = lhandle;
	else
		debug_critical("The sound handle cannot be get [%d]\n", lhandle);

	debug_fleave();
	return MM_ERROR_NONE;
}

///////////////////////////////////
////     MMSOUND ROUTING APIs
///////////////////////////////////
EXPORT_API
int mm_sound_set_path(int gain, int output, int input, int option)
{
	int err;

	debug_fenter();

	debug_msg("gain: 0x%02X, output: %d, input: %d, option: 0x%x\n", gain, output, input, option);

	err = avsys_audio_set_path_ex( gain, output, input, option);
	if (err < 0) {
		debug_error("avsys_audio_set_path() failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();
	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_get_path(int *gain, int *output, int *input, int *option)
{
	int err;

	debug_fenter();

	err = avsys_audio_get_path_ex( gain, output, input, option);
	if (err < 0) {
		debug_error("SoundCtlPathGet() failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_msg("gain: 0x%02X, output: %d, input: %d, option: 0x%x\n", *gain, *output, *input, *option);

	debug_fleave();
	return MM_ERROR_NONE;
}

#ifdef PULSE_CLIENT
enum {
	USE_PA_SINK_ALSA = 0,
	USE_PA_SINK_A2DP,
};
EXPORT_API
int mm_sound_route_set_system_policy (system_audio_route_t route)
{
	int ret = MM_ERROR_NONE;
	int pa_sink = USE_PA_SINK_ALSA;
	int codec_option = AVSYS_AUDIO_PATH_OPTION_JACK_AUTO;
	int current_route;
	int gain, out, in, option; 

	debug_fenter();

	debug_log ("route = %d\n", route);
	switch(route)
	{
	case SYSTEM_AUDIO_ROUTE_POLICY_IGNORE_A2DP:
		codec_option = AVSYS_AUDIO_PATH_OPTION_JACK_AUTO;
		pa_sink = USE_PA_SINK_ALSA;
		break;
	case SYSTEM_AUDIO_ROUTE_POLICY_HANDSET_ONLY:
		codec_option = AVSYS_AUDIO_PATH_OPTION_NONE;
		pa_sink = USE_PA_SINK_ALSA;
		break;
	case SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT:
		codec_option = AVSYS_AUDIO_PATH_OPTION_JACK_AUTO;
		pa_sink = USE_PA_SINK_A2DP;
		break;
	default:
		debug_error("Unknown route %d\n", route);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	/* Vconf check */
	ret = vconf_get_int(ROUTE_VCONF_KEY, &current_route);
	if(ret < 0) {
		debug_error("Can not get current route policy\n");
		current_route = SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT;
		if(0 > vconf_set_int(ROUTE_VCONF_KEY, current_route)) {
			debug_error("Can not save current audio route policy to %s\n", ROUTE_VCONF_KEY);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}
			return MM_ERROR_SOUND_INTERNAL;
		}
	}

	/* If same as before, do nothing */
	if(current_route == route)
	{
		debug_warning("Same route policy with current. Skip setting\n");
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return MM_ERROR_NONE;
	}

	/* Get Current gain */
	avsys_audio_get_path_ex(&gain, &out, &in, &option);                                 
	debug_msg ("gain = %x, out = %x, in = %x, option = %x\n", gain, out, in, option);   
	if (gain == AVSYS_AUDIO_GAIN_EX_FMRADIO) {
		int output_path = 0 ,res = 0;
		
		debug_msg ("This is FM radio gain mode.....\n");   

		/* select output path from policy */
		if (route == SYSTEM_AUDIO_ROUTE_POLICY_HANDSET_ONLY)
			output_path = AVSYS_AUDIO_PATH_EX_SPK;
		else if (route == SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT || route == SYSTEM_AUDIO_ROUTE_POLICY_IGNORE_A2DP)
			output_path = AVSYS_AUDIO_PATH_EX_HEADSET;

		/* Do set path */
		res = avsys_audio_set_path_ex( AVSYS_AUDIO_GAIN_EX_FMRADIO,            
		                                output_path,                           
		                                AVSYS_AUDIO_PATH_EX_FMINPUT,                            
		                                AVSYS_AUDIO_PATH_OPTION_NONE );        
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set playback sound path, error=0x%x\n", ret);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}
			return ret;
		}
	} else {

		/* Try to change default sink */
		ret = MMSoundClientSetAudioRoute(pa_sink);
		if (ret < 0) {
			debug_error("MMSoundClientsetAudioRoute() Failed for sink [%d]\n", pa_sink);
			if(pa_sink == USE_PA_SINK_ALSA) {
				/* PA_A2DP_SINK can be return error; */
				if(MM_ERROR_NONE != __mm_sound_unlock()) {
					debug_error("Unlock failed\n");
					return MM_ERROR_SOUND_INTERNAL;
				}
				return ret;
			}
		}

		/* Do Set path if (IGNORE A2DP or HANDSET) or (DEFAULT with no BT) */
		if(pa_sink == USE_PA_SINK_ALSA || (pa_sink == USE_PA_SINK_A2DP && ret != MM_ERROR_NONE)) 
		{
			ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_KEYTONE, AVSYS_AUDIO_PATH_EX_SPK, AVSYS_AUDIO_PATH_EX_NONE, codec_option);
			if(AVSYS_FAIL(ret)) {
				debug_error("Can not set playback sound path 0x%x\n", ret);
				if(MM_ERROR_NONE != __mm_sound_unlock()) {
					debug_error("Unlock failed\n");
					return MM_ERROR_SOUND_INTERNAL;
				}
				return ret;
			}
			ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_VOICEREC, AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_EX_MIC, codec_option);
			if(AVSYS_FAIL(ret)) {
				debug_error("Can not set capture sound path 0x%x\n", ret);
				if(MM_ERROR_NONE != __mm_sound_unlock()) {
					debug_error("Unlock failed\n");
					return MM_ERROR_SOUND_INTERNAL;
				}
				return ret;
			}
		}

	} /* if (gain == AVSYS_AUDIO_GAIN_EX_FMRADIO) {} else {} */
	
	/* Set route policy */
	ret = avsys_audio_set_route_policy((avsys_audio_route_policy_t)route);
	if(AVSYS_FAIL(ret)) {
		debug_error("Can not set route policy to avsystem 0x%x\n", ret);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return ret;
	}

	/* update vconf */
	ret = vconf_set_int(ROUTE_VCONF_KEY, (int)route);
	if(ret < 0) {
		debug_error("Can not set route policy to vconf %s\n", ROUTE_VCONF_KEY);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return MM_ERROR_SOUND_INTERNAL;
	}

	/* clean up */
	if(MM_ERROR_NONE != __mm_sound_unlock()) {
		debug_error("Unlock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();
	return MM_ERROR_NONE;
}


EXPORT_API
int mm_sound_route_get_system_policy (system_audio_route_t *route)
{
	int ret = MM_ERROR_NONE;
	int lv_route = 0;
	if(route == NULL) {
		debug_error("Null pointer\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	ret = vconf_get_int(ROUTE_VCONF_KEY, &lv_route);
	if(ret < 0 ) {
		debug_error("Can not get route policy from vconf. set default\n");
		if(0> vconf_set_int(ROUTE_VCONF_KEY, SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT))
		{
			debug_error("Set audio route policy to default failed\n");
			ret = MM_ERROR_SOUND_INTERNAL;
		}
		else {
			*route = SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT;
		}
	}
	else {
		*route = lv_route;
	}

	if(ret == MM_ERROR_NONE) {
		avsys_audio_route_policy_t av_route;
		ret = avsys_audio_get_route_policy((avsys_audio_route_policy_t*)&av_route);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not get route policy to avsystem 0x%x\n", ret);
			av_route = -1;
		}
		if(av_route != *route) {
			/* match vconf & shared mem info */
			ret = avsys_audio_set_route_policy(*route);
			if(AVSYS_FAIL(ret)) {
				debug_error("avsys_audio_set_route_policy failed 0x%x\n", ret);
			}
		}
	}

	if(MM_ERROR_NONE != __mm_sound_unlock()) {
		debug_error("Unlock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return ret;
}


EXPORT_API
int mm_sound_route_get_a2dp_status (int *connected, char **bt_name)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (connected == NULL || bt_name == NULL) {
		debug_error ("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT; 
	} 

	ret = MMSoundClientIsBtA2dpOn (connected, bt_name);
	debug_msg ("connected=[%d] bt_name[%s]\n", *connected, *bt_name);
	if (ret < 0) {
		debug_error("MMSoundClientIsBtA2dpOn() Failed\n");
		return ret;
	}
		
	debug_fleave();
	return ret;
}

EXPORT_API
int mm_sound_route_get_playing_device(system_audio_route_device_t *dev)
{
	avsys_audio_playing_devcie_t status;

	if(!dev)
		return MM_ERROR_INVALID_ARGUMENT;

	if(AVSYS_FAIL(avsys_audio_get_playing_device_info(&status)))
	{
		debug_error("Can not get playing device info\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	switch(status)
	{
	case AVSYS_AUDIO_ROUTE_DEVICE_HANDSET:
		*dev = SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_HANDSET;
		break;
	case AVSYS_AUDIO_ROUTE_DEVICE_BLUETOOTH:
		*dev = SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_BLUETOOTH;
		break;
	case AVSYS_AUDIO_ROUTE_DEVICE_EARPHONE:
		*dev = SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_EARPHONE;
		break;
	default:
		*dev = SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_NONE;
		break;
	}

	return MM_ERROR_NONE;
}


typedef struct {
	audio_route_policy_changed_callback_fn func;
	void* data;
}route_change_cb_param;

route_change_cb_param g_route_param;


void route_change_vconf_cb(keynode_t *node, void *data)
{
	int ret = MM_ERROR_NONE;
	int lv_route = 0;
	route_change_cb_param *param = (route_change_cb_param *) data;

	debug_msg("%s changed callback called\n", vconf_keynode_get_name(node));
	ret = vconf_get_int(ROUTE_VCONF_KEY, &lv_route);
	if(ret < 0) {
		debug_error("Can not get route info from vconf..(in cb func)\n");
		return;
	}

	if(param && (param->func != NULL)) {
		((audio_route_policy_changed_callback_fn)param->func)(param->data, (system_audio_route_t)lv_route);
	}
	return;
}

EXPORT_API
int mm_sound_route_add_change_callback(audio_route_policy_changed_callback_fn func, void *user_data)
{
	int ret = MM_ERROR_NONE;

	g_route_param.func = func;
	g_route_param.data = user_data;

	ret = vconf_notify_key_changed(ROUTE_VCONF_KEY, route_change_vconf_cb, (void *)&g_route_param);
	if(ret < 0) {
		debug_error("Can not add callback - vconf error\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	}
	return ret;
}

EXPORT_API
int mm_sound_route_remove_change_callback()
{
	int ret = MM_ERROR_NONE;

	g_route_param.func = NULL;
	g_route_param.data = NULL;

	ret = vconf_ignore_key_changed(ROUTE_VCONF_KEY, route_change_vconf_cb);
	if(ret < 0) {
		debug_error("Can not add callback - vconf error\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	}
	return ret;
}

#endif /* PULSE_CLIENT */

EXPORT_API
int mm_sound_system_get_capture_status(system_audio_capture_status_t *status)
{
	int err = AVSYS_STATE_SUCCESS;
	int on_capture = 0;

	if(!status)
		return MM_ERROR_INVALID_ARGUMENT;

	err = avsys_audio_get_capture_status(&on_capture);
	if(err < 0) {
		debug_error("Can not get capture status with 0x%x\n", err);
		return MM_ERROR_SOUND_INTERNAL;
	}

	if(on_capture)
		*status = SYSTEM_AUDIO_CAPTURE_ACTIVE;
	else
		*status = SYSTEM_AUDIO_CAPTURE_NONE;

	return MM_ERROR_NONE;
}

__attribute__ ((destructor))
void __mmfsnd_finalize(void)
{
	debug_fenter();

	MMSoundClientCallbackFini();

	debug_fleave();
}

__attribute__ ((constructor))
void __mmfsnd_initialize(void)
{
	/* Will be Fixed */
}
