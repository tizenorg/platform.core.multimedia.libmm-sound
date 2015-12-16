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
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <mm_source.h>
#include <mm_error.h>
#include <mm_types.h>
#include <mm_debug.h>
#include <mm_ipc.h>
#include <mm_session.h>

#include "include/mm_sound_mgr_common.h"
#include "include/mm_sound_mgr_codec.h"
#include "include/mm_sound_mgr_ipc.h"
#include "include/mm_sound_plugin_codec.h"
#include "include/mm_sound_thread_pool.h"
#include "include/mm_sound_pa_client.h"
#include "../include/mm_sound_focus.h"
#include "../include/mm_sound_device.h"



#define _ENABLE_KEYTONE	/* Temporal test code */

typedef struct {
	int (*callback)(int, void *, void *, int);	/* msg_type(pid) client callback & client data info */
	void *param;
	int pid;
	void *msgcallback;
	void *msgdata;
	MMHandleType plughandle;

	int pluginid;
	int status;
	int session_type;
	int session_options;
	int focus_handle;
	int focus_wcb_id;
	unsigned int subs_id;
	mm_sound_focus_type_e current_focus_type;

	bool enable_session;
 } __mmsound_mgr_codec_handle_t;

static MMSoundPluginType *g_codec_plugins = NULL;
static __mmsound_mgr_codec_handle_t g_slots[MANAGER_HANDLE_MAX];
static mmsound_codec_interface_t g_plugins[MM_SOUND_SUPPORTED_CODEC_NUM];
static pthread_mutex_t g_slot_mutex;
static pthread_mutex_t codec_wave_mutex;
static int _MMSoundMgrCodecStopCallback(int param);
static int _MMSoundMgrCodecGetEmptySlot(int *slotid);
static int _MMSoundMgrCodecRegisterInterface(MMSoundPluginType *plugin);

#define STATUS_IDLE 0
#define STATUS_SOUND 3

#define SOUND_SLOT_START 0

void sound_codec_focus_callback(int id, mm_sound_focus_type_e focus_type, mm_sound_focus_state_e focus_state, const char *reason_for_change, const char *additional_info, void *user_data)
{

	int slotid = (int)user_data;
	int result = MM_ERROR_NONE;

	debug_warning ("focus callback called -> focus_stae(%d), reasoun_for_change(%s)", focus_state, reason_for_change ? reason_for_change : "N/A");

	if(g_slots[slotid].session_options & MM_SESSION_OPTION_UNINTERRUPTIBLE){
		debug_warning ("session option is UNINTERRUPTIBLE, nothing to do with focus");
		return;
	}
	if (focus_state == FOCUS_IS_RELEASED) {
		g_slots[slotid].current_focus_type = FOCUS_FOR_BOTH&(~focus_type);
		debug_warning ("focus is released -> stop playing");
		result = MMSoundMgrCodecStop(slotid);
		if (result != MM_ERROR_NONE) {
			debug_log("result error %d\n", result);
		}

	}
	return;
}

void sound_codec_focus_watch_callback(int id, mm_sound_focus_type_e focus_type, mm_sound_focus_state_e focus_state, const char *reason_for_change, const char* additional_info, void *user_data)
{
	int slotid = (int)user_data;
	int result = MM_ERROR_NONE;

	debug_warning ("focus callback called -> focus_stae(%d), reasoun_for_change(%s)", focus_state, reason_for_change ? reason_for_change : "N/A");

	if(g_slots[slotid].session_options & MM_SESSION_OPTION_UNINTERRUPTIBLE){
		debug_warning ("session option is UNINTERRUPTIBLE, nothing to do with focus");
		return;
	}
	if (focus_state == FOCUS_IS_ACQUIRED) {
		debug_warning ("focus is released -> stop playing");
		result = MMSoundMgrCodecStop(slotid);
		if (result != MM_ERROR_NONE) {
			debug_log("result error %d\n", result);
		}

	}
	return;
}

void sound_codec_device_connected_callback(MMSoundDevice_t device, bool is_connected, void *user_data)
{
	int slotid = (int)user_data;
	int result = MM_ERROR_NONE;
	mm_sound_device_type_e type;

	debug_warning ("device_connected_callback called");

	if (mm_sound_get_device_type (device, &type) != MM_ERROR_NONE) {
		debug_error("getting device type failed");
	} else {
		switch (type) {
			case MM_SOUND_DEVICE_TYPE_AUDIOJACK:
			case MM_SOUND_DEVICE_TYPE_BLUETOOTH:
			case MM_SOUND_DEVICE_TYPE_HDMI:
			case MM_SOUND_DEVICE_TYPE_MIRRORING:
			case MM_SOUND_DEVICE_TYPE_USB_AUDIO:
				if (!is_connected) {
					debug_warning("sound device unplugged");
					result = MMSoundMgrCodecStop(slotid);
					if (result != MM_ERROR_NONE) {
						debug_error("MMSoundMgrCodecStop error %d\n", result);
					}
					result = mm_sound_remove_device_connected_callback(g_slots[slotid].subs_id);
					if (result != MM_ERROR_NONE) {
						debug_error("mm_sound_remove_device_connected_callback error %d\n", result);
					}
				}
				break;
			default:
				break;
		}
	}
}

int MMSoundMgrCodecInit(const char *targetdir)
{
	int loop = 0;
	int count = 0;

	debug_enter("\n");

	memset (g_slots, 0, sizeof(g_slots));

	if(pthread_mutex_init(&g_slot_mutex, NULL)) {
		debug_error("pthread_mutex_init failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if(pthread_mutex_init(&codec_wave_mutex, NULL)) {
		debug_error("pthread_mutex_init failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	for (count = 0; count < MANAGER_HANDLE_MAX; count++) {
		g_slots[count].status = STATUS_IDLE;
		g_slots[count].plughandle = 0;
	}

	if (g_codec_plugins) {
		debug_warning("Please Check Init twice\n");
		MMSoundPluginRelease(g_codec_plugins);
	}

	MMSoundPluginScan(targetdir, MM_SOUND_PLUGIN_TYPE_CODEC, &g_codec_plugins);

	while (g_codec_plugins[loop].type != MM_SOUND_PLUGIN_TYPE_NONE) {
		_MMSoundMgrCodecRegisterInterface(&g_codec_plugins[loop++]);
	}

	debug_leave("\n");
	return MM_ERROR_NONE;
}

int MMSoundMgrCodecFini(void)
{
	debug_enter("\n");

	memset(g_plugins, 0, sizeof(mmsound_codec_interface_t) * MM_SOUND_SUPPORTED_CODEC_NUM);
	MMSoundPluginRelease(g_codec_plugins);
	g_codec_plugins = NULL;
	pthread_mutex_destroy(&g_slot_mutex);
	pthread_mutex_destroy(&codec_wave_mutex);
	debug_leave("\n");
	return MM_ERROR_NONE;
}


int MMSoundMgrCodecPlay(int *slotid, const mmsound_mgr_codec_param_t *param)
{
	int count = 0;
	mmsound_codec_info_t info;
	mmsound_codec_param_t codec_param;
	int err = MM_ERROR_NONE;
	int need_focus_unregister = 0;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	for (count = 0; g_plugins[count].GetSupportTypes; count++) {
		/* Find codec */
		if (g_plugins[count].Parse(param->source, &info) == MM_ERROR_NONE)
			break;
	}

	/*The count num means codec type WAV, MP3 */
	debug_msg("DTMF[%d] Repeat[%d] Volume[%f] plugin_codec[%d]\n", param->tone, param->repeat_count, param->volume, count);

	if (g_plugins[count].GetSupportTypes == NULL) {	/* Codec not found */
		debug_error("unsupported file type %d\n", count);
		err = MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE;
		goto cleanup;
	}

#ifdef DEBUG_DETAIL
	debug_msg("Get New handle\n");
#endif

	err = _MMSoundMgrCodecGetEmptySlot(slotid);
	if (err != MM_ERROR_NONE || *slotid < 0) {
		debug_error("Empty g_slot is not found\n");
		goto cleanup;
	}

	codec_param.tone = param->tone;
	codec_param.volume_config = param->volume_config;
	codec_param.repeat_count = param->repeat_count;
	codec_param.volume = param->volume;
	codec_param.source = param->source;
	codec_param.priority = param->priority;
	codec_param.stop_cb = _MMSoundMgrCodecStopCallback;
	codec_param.param = *slotid;
	codec_param.pid = (int)param->param;
	codec_param.handle_route = param->handle_route;
	codec_param.codec_wave_mutex = &codec_wave_mutex;
	codec_param.stream_index = param->stream_index;
	strncpy(codec_param.stream_type, param->stream_type, MM_SOUND_STREAM_TYPE_LEN);
	pthread_mutex_lock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif

	/*
	 * Register FOCUS here
	 */

	if (param->session_type != MM_SESSION_TYPE_CALL &&
		param->session_type != MM_SESSION_TYPE_VIDEOCALL &&
		param->session_type != MM_SESSION_TYPE_VOIP &&
		param->session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
		param->priority != HANDLE_PRIORITY_SOLO &&
		param->enable_session) {

		unsigned int subs_id = 0;

		err = mm_sound_add_device_connected_callback(MM_SOUND_DEVICE_STATE_ACTIVATED_FLAG, (mm_sound_device_connected_cb)sound_codec_device_connected_callback, *slotid, &subs_id);
		if (err) {
			debug_error("mm_sound_add_device_connected_callback failed [0x%x]", err);
			pthread_mutex_unlock(&g_slot_mutex);
			return MM_ERROR_POLICY_INTERNAL;
		}
		g_slots[*slotid].subs_id = subs_id;

		if ((param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS) || param->session_type == MM_SESSION_TYPE_ALARM || param->session_type == MM_SESSION_TYPE_NOTIFY || param->session_type == MM_SESSION_TYPE_EMERGENCY) {
			debug_warning("session option is PAUSE_OTHERS -> acquire focus");
			err = mm_sound_focus_get_id((int *)(&param->focus_handle));
			err = mm_sound_register_focus_for_session(param->focus_handle, (int)param->param, "media", sound_codec_focus_callback, (void*)*slotid);
			if (err) {
				debug_error("mm_sound_register_focus_for_session failed [0x%x]", err);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
			err = mm_sound_acquire_focus(param->focus_handle, FOCUS_FOR_BOTH, NULL);
			if (err) {
				debug_error("mm_sound_acquire_focus failed [0x%x]", err);
				err = mm_sound_unregister_focus(param->focus_handle);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
			g_slots[*slotid].current_focus_type = FOCUS_FOR_BOTH;
		} else if (param->session_options & MM_SESSION_OPTION_UNINTERRUPTIBLE) {
			/* do nothing */
			debug_warning("session option is UNINTERRUPTIBLE, nothing to do with focus");
		} else {
			debug_warning("need to set focus watch callback");
			err = mm_sound_set_focus_watch_callback_for_session((int)param->param, FOCUS_FOR_BOTH, sound_codec_focus_watch_callback, (void*)*slotid, (int *)(&param->focus_wcb_id));
			if (err) {
				debug_error("mm_sound_set_focus_watch_callback_for_session failed [0x%x]", err);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}
	//

	/* Codec id WAV or MP3 */
	g_slots[*slotid].pluginid = count;
	g_slots[*slotid].param    = param->param;		/* This arg is used callback data */
	g_slots[*slotid].session_type = param->session_type;
	g_slots[*slotid].session_options = param->session_options;
	g_slots[*slotid].focus_handle = param->focus_handle;
	g_slots[*slotid].focus_wcb_id = param->focus_wcb_id;
	g_slots[*slotid].enable_session = true;
	g_slots[*slotid].pid = (int)param->param;


	debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);

	err = g_plugins[g_slots[*slotid].pluginid].Create(&codec_param, &info, &(g_slots[*slotid].plughandle));
	debug_msg("Created audio handle : [%d]\n", g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Plugin create fail : 0x%08X\n", err);
		g_slots[*slotid].status = STATUS_IDLE;
		pthread_mutex_unlock(&g_slot_mutex);
		debug_warning("After Slot_mutex UNLOCK\n");
		if (param->focus_handle) {
			need_focus_unregister = 1;
		}
		goto cleanup;
	}

	err = g_plugins[g_slots[*slotid].pluginid].Play(g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Fail to play : 0x%08X\n", err);
		g_plugins[g_slots[*slotid].pluginid].Destroy(g_slots[*slotid].plughandle);
		if (param->focus_handle) {
			need_focus_unregister = 1;
		}
	}

	pthread_mutex_unlock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex UNLOCK\n");
#endif

cleanup:
	if(param->session_type != MM_SESSION_TYPE_CALL &&
		param->session_type != MM_SESSION_TYPE_VIDEOCALL &&
		param->session_type != MM_SESSION_TYPE_VOIP &&
		param->session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
		param->enable_session &&
		need_focus_unregister == 1) {

		if (param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS || param->session_type == MM_SESSION_TYPE_ALARM || param->session_type == MM_SESSION_TYPE_NOTIFY || param->session_type == MM_SESSION_TYPE_EMERGENCY) {
			err = mm_sound_release_focus(param->focus_handle, FOCUS_FOR_BOTH, NULL);
			if(mm_sound_unregister_focus(param->focus_handle) || err) {
				debug_error("focus cleaning up failed[0x%x]", err);
				return MM_ERROR_POLICY_INTERNAL;
			}
		} else if (~(param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS)) {
			err = mm_sound_unset_focus_watch_callback(param->focus_wcb_id);
			if (err) {
				debug_error("focus watch cleaning up failed[0x%x]", err);
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

#ifdef DEBUG_DETAIL
	debug_leave("\n");
#endif

	return err;
}

int MMSoundMgrCodecPlayWithStreamInfo(int *slotid, const mmsound_mgr_codec_param_t *param)
{
	int count = 0;
	mmsound_codec_info_t info;
	mmsound_codec_param_t codec_param;
	int err = MM_ERROR_NONE;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	for (count = 0; g_plugins[count].GetSupportTypes; count++) {
		/* Find codec */
		if (g_plugins[count].Parse(param->source, &info) == MM_ERROR_NONE)
			break;
	}

	/*The count num means codec type WAV, MP3 */
	debug_msg("Repeat[%d] Volume[%f] plugin_codec[%d]\n", param->repeat_count, param->volume, count);

	if (g_plugins[count].GetSupportTypes == NULL) {	/* Codec not found */
		debug_error("unsupported file type %d\n", count);
		err = MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE;
		goto cleanup;
	}

	err = _MMSoundMgrCodecGetEmptySlot(slotid);
	if (err != MM_ERROR_NONE || *slotid < 0) {
		debug_error("Empty g_slot is not found\n");
		goto cleanup;
	}

	codec_param.volume_config = -1; //setting volume config to -1 since using stream info instead of volume type
	codec_param.repeat_count = param->repeat_count;
	codec_param.volume = param->volume;
	codec_param.source = param->source;
	codec_param.priority = param->priority;
	codec_param.stop_cb = _MMSoundMgrCodecStopCallback;
	codec_param.param = *slotid;
	codec_param.pid = (int)param->param;
	codec_param.handle_route = param->handle_route;
	codec_param.codec_wave_mutex = &codec_wave_mutex;
	codec_param.stream_index = param->stream_index;
	strncpy(codec_param.stream_type, param->stream_type, MM_SOUND_STREAM_TYPE_LEN);
	pthread_mutex_lock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif

	/* Codec id WAV or MP3 */
	g_slots[*slotid].pluginid = count;
	g_slots[*slotid].param    = param->param;		/* This arg is used callback data */

	debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);

	err = g_plugins[g_slots[*slotid].pluginid].Create(&codec_param, &info, &(g_slots[*slotid].plughandle));
	debug_msg("Created audio handle : [%d]\n", g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Plugin create fail : 0x%08X\n", err);
		g_slots[*slotid].status = STATUS_IDLE;
		pthread_mutex_unlock(&g_slot_mutex);
		debug_warning("After Slot_mutex UNLOCK\n");
		goto cleanup;
	}

	err = g_plugins[g_slots[*slotid].pluginid].Play(g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Fail to play : 0x%08X\n", err);
		g_plugins[g_slots[*slotid].pluginid].Destroy(g_slots[*slotid].plughandle);
	}

	pthread_mutex_unlock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex UNLOCK\n");
#endif

cleanup:

#ifdef DEBUG_DETAIL
	debug_leave("\n");
#endif

	return err;

}

#define DTMF_PLUGIN_COUNT 2
int MMSoundMgrCodecPlayDtmf(int *slotid, const mmsound_mgr_codec_param_t *param)
{
	int count = 0;
	int *codec_type;
	mmsound_codec_info_t info;
	mmsound_codec_param_t codec_param;
	int err = MM_ERROR_NONE;
	int need_focus_unregister = 0;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	for (count = 0; g_plugins[count].GetSupportTypes; count++) {
		/* Find codec */
		codec_type = g_plugins[count].GetSupportTypes();
		if(codec_type && (MM_SOUND_SUPPORTED_CODEC_DTMF == codec_type[0]))
			break;
	}

	/*The count num means codec type DTMF */
	debug_msg("DTMF[%d] Repeat[%d] Volume[%f] plugin_codec[%d]\n", param->tone, param->repeat_count, param->volume, count);

	if (g_plugins[count].GetSupportTypes == NULL) {	/* Codec not found */
		debug_error("unsupported file type %d\n", count);
		printf("unsupported file type %d\n", count);
		err = MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE;
		goto cleanup;
	}

#ifdef DEBUG_DETAIL
	debug_msg("Get New handle\n");
#endif

	err = _MMSoundMgrCodecGetEmptySlot(slotid);
	if(err != MM_ERROR_NONE || *slotid < 0)
	{
		debug_error("Empty g_slot is not found\n");
		goto cleanup;
	}

	codec_param.tone = param->tone;
	codec_param.priority = 0;
	codec_param.volume_config = param->volume_config;
	codec_param.repeat_count = param->repeat_count;
	codec_param.volume = param->volume;
	codec_param.stop_cb = _MMSoundMgrCodecStopCallback;
	codec_param.param = *slotid;
	codec_param.pid = (int)param->param;
	codec_param.stream_index = param->stream_index;
	strncpy(codec_param.stream_type, param->stream_type, MM_SOUND_STREAM_TYPE_LEN);

	pthread_mutex_lock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif

	//
	/*
	 * Register FOCUS here
	 */

	if (param->session_type != MM_SESSION_TYPE_CALL &&
		param->session_type != MM_SESSION_TYPE_VIDEOCALL &&
		param->session_type != MM_SESSION_TYPE_VOIP &&
		param->session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
		param->enable_session) {

		unsigned int subs_id = 0;

		err = mm_sound_add_device_connected_callback(MM_SOUND_DEVICE_STATE_ACTIVATED_FLAG, (mm_sound_device_connected_cb)sound_codec_device_connected_callback, *slotid, &subs_id);
		if (err) {
			debug_error("mm_sound_add_device_connected_callback failed [0x%x]", err);
			pthread_mutex_unlock(&g_slot_mutex);
			return MM_ERROR_POLICY_INTERNAL;
		}
		g_slots[*slotid].subs_id = subs_id;

		if ((param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS) || param->session_type == MM_SESSION_TYPE_ALARM || param->session_type == MM_SESSION_TYPE_NOTIFY || param->session_type == MM_SESSION_TYPE_EMERGENCY) {
			debug_warning("session option is PAUSE_OTHERS -> acquire focus");
			err = mm_sound_focus_get_id((int *)(&param->focus_handle));
			err = mm_sound_register_focus_for_session(param->focus_handle, (int)param->param, "media", sound_codec_focus_callback, (void*)*slotid);
			if (err) {
				debug_error("mm_sound_register_focus failed [0x%x]", err);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
			err = mm_sound_acquire_focus(param->focus_handle, FOCUS_FOR_BOTH, NULL);
			if (err) {
				debug_error("mm_sound_acquire_focus failed [0x%x]", err);
				err = mm_sound_unregister_focus(param->focus_handle);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
			g_slots[*slotid].current_focus_type = FOCUS_FOR_BOTH;
		} else if (param->session_options & MM_SESSION_OPTION_UNINTERRUPTIBLE) {
			/* do nothing */
			debug_warning("session option is UNINTERRUPTIBLE, nothing to do with focus");
		} else {
			debug_warning("need to set focus watch callback");
			err = mm_sound_set_focus_watch_callback_for_session((int)param->param, FOCUS_FOR_BOTH, sound_codec_focus_watch_callback, (void*)*slotid, (int *)(&param->focus_wcb_id));
			if (err) {
				debug_error("mm_sound_set_focus_watch_callback failed [0x%x]", err);
				pthread_mutex_unlock(&g_slot_mutex);
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

	g_slots[*slotid].pluginid = count;
	g_slots[*slotid].param    = param->param;		/* This arg is used callback data */
	g_slots[*slotid].session_type = param->session_type;
	g_slots[*slotid].session_options = param->session_options;
	g_slots[*slotid].focus_handle= param->focus_handle;
	g_slots[*slotid].focus_wcb_id= param->focus_wcb_id;
	g_slots[*slotid].enable_session = param->enable_session;
	g_slots[*slotid].pid = (int)param->param;

#ifdef DEBUG_DETAIL
	debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);
#endif

	err = g_plugins[g_slots[*slotid].pluginid].Create(&codec_param, &info, &(g_slots[*slotid].plughandle));
	debug_msg("Created audio handle : [%d]\n", g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Plugin create fail : 0x%08X\n", err);
		g_slots[*slotid].status = STATUS_IDLE;
		pthread_mutex_unlock(&g_slot_mutex);
		debug_warning("After Slot_mutex UNLOCK\n");
		need_focus_unregister = 1;
		goto cleanup;
	}

	err = g_plugins[g_slots[*slotid].pluginid].Play(g_slots[*slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Fail to play : 0x%08X\n", err);
		g_plugins[g_slots[*slotid].pluginid].Destroy(g_slots[*slotid].plughandle);
		need_focus_unregister = 1;
	}

	pthread_mutex_unlock(&g_slot_mutex);

	debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex UNLOCK\n")
#endif

cleanup:
	if (param->session_type != MM_SESSION_TYPE_CALL &&
		param->session_type != MM_SESSION_TYPE_VIDEOCALL &&
		param->session_type != MM_SESSION_TYPE_VOIP &&
		param->session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
		param->enable_session &&
		need_focus_unregister == 1) {

		if (param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS || param->session_type == MM_SESSION_TYPE_ALARM || param->session_type == MM_SESSION_TYPE_NOTIFY || param->session_type == MM_SESSION_TYPE_EMERGENCY) {
			err = mm_sound_release_focus(param->focus_handle, FOCUS_FOR_BOTH, NULL);
			if(mm_sound_unregister_focus(param->focus_handle) || err) {
				debug_error("focus cleaning up failed[0x%x]", err);
				return MM_ERROR_POLICY_INTERNAL;
			}
		} else if (~(param->session_options & MM_SESSION_OPTION_PAUSE_OTHERS)) {
			err = mm_sound_unset_focus_watch_callback(param->focus_wcb_id);
			if (err) {
				debug_error("focus watch cleaning up failed[0x%x]", err);
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

#ifdef DEBUG_DETAIL
	debug_leave("\n");
#endif

	return err;
}

int MMSoundMgrCodecPlayDtmfWithStreamInfo(int *slotid, const mmsound_mgr_codec_param_t *param)
{
	int count = 0;
	int *codec_type;
	mmsound_codec_info_t info;
	mmsound_codec_param_t codec_param;
	int err = MM_ERROR_NONE;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	for (count = 0; g_plugins[count].GetSupportTypes; count++) {
		/* Find codec */
		codec_type = g_plugins[count].GetSupportTypes();
		if(codec_type && (MM_SOUND_SUPPORTED_CODEC_DTMF == codec_type[0]))
			break;
	}

	/*The count num means codec type DTMF */
	debug_msg("DTMF[%d] Repeat[%d] Volume[%f] plugin_codec[%d]\n", param->tone, param->repeat_count, param->volume, count);

	if (g_plugins[count].GetSupportTypes == NULL) { /* Codec not found */
		debug_error("unsupported file type %d\n", count);
		printf("unsupported file type %d\n", count);
		err = MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE;
		goto cleanup;
	}

#ifdef DEBUG_DETAIL
	debug_msg("Get New handle\n");
#endif

	err = _MMSoundMgrCodecGetEmptySlot(slotid);
	if(err != MM_ERROR_NONE || *slotid < 0)
	{
		debug_error("Empty g_slot is not found\n");
		goto cleanup;
	}

	codec_param.tone = param->tone;
	codec_param.priority = 0;
	codec_param.repeat_count = param->repeat_count;
	codec_param.volume = param->volume;
	codec_param.stop_cb = _MMSoundMgrCodecStopCallback;
	codec_param.param = *slotid;
	codec_param.pid = (int)param->param;
	codec_param.volume_config = -1; //setting volume config to -1 since using stream info instead of volume type
	codec_param.stream_index = param->stream_index;
	strncpy(codec_param.stream_type, param->stream_type, MM_SOUND_STREAM_TYPE_LEN);

	pthread_mutex_lock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif
		g_slots[*slotid].pluginid = count;
		g_slots[*slotid].param	  = param->param;		/* This arg is used callback data */
		g_slots[*slotid].enable_session = param->enable_session;

#ifdef DEBUG_DETAIL
		debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);
#endif

		err = g_plugins[g_slots[*slotid].pluginid].Create(&codec_param, &info, &(g_slots[*slotid].plughandle));
		debug_msg("Created audio handle : [%d]\n", g_slots[*slotid].plughandle);
		if (err != MM_ERROR_NONE) {
			debug_error("Plugin create fail : 0x%08X\n", err);
			g_slots[*slotid].status = STATUS_IDLE;
			pthread_mutex_unlock(&g_slot_mutex);
			debug_warning("After Slot_mutex UNLOCK\n");
			goto cleanup;
		}

		err = g_plugins[g_slots[*slotid].pluginid].Play(g_slots[*slotid].plughandle);
		if (err != MM_ERROR_NONE) {
			debug_error("Fail to play : 0x%08X\n", err);
			g_plugins[g_slots[*slotid].pluginid].Destroy(g_slots[*slotid].plughandle);
		}

		pthread_mutex_unlock(&g_slot_mutex);

		debug_msg("Using Slotid : [%d] Slot Status : [%d]\n", *slotid, g_slots[*slotid].status);
#ifdef DEBUG_DETAIL
		debug_msg("After Slot_mutex UNLOCK\n");
#endif

	cleanup:
#ifdef DEBUG_DETAIL
		debug_leave("\n");
#endif

		return err;

}

int MMSoundMgrCodecStop(const int slotid)
{
	int err = MM_ERROR_NONE;

	debug_enter("(Slotid : [%d])\n", slotid);

	if (slotid < 0 || MANAGER_HANDLE_MAX <= slotid) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	pthread_mutex_lock (&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif
	if (g_slots[slotid].status == STATUS_IDLE) {
		err = MM_ERROR_SOUND_INVALID_STATE;
		debug_warning("The playing slots is not found, Slot ID : [%d]\n", slotid);
		goto cleanup;
	}
#ifdef DEBUG_DETAIL
	debug_msg("Found slot, Slotid [%d] State [%d]\n", slotid, g_slots[slotid].status);
#endif

	err = g_plugins[g_slots[slotid].pluginid].Stop(g_slots[slotid].plughandle);
	if (err != MM_ERROR_NONE) {
		debug_error("Fail to STOP Code : 0x%08X\n", err);
	}
	debug_msg("Found slot, Slotid [%d] State [%d]\n", slotid, g_slots[slotid].status);
cleanup:
	pthread_mutex_unlock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex UNLOCK\n");
#endif
	debug_leave("(err : 0x%08X)\n", err);

	return err;
}

int MMSoundMgrCodecClearFocus(int pid)
{
	int err = MM_ERROR_NONE;
	int slotid;

	debug_enter("(pid : [%d])\n", pid);

	pthread_mutex_lock (&g_slot_mutex);

	for (slotid = 0 ; slotid < MANAGER_HANDLE_MAX ; slotid++) {
		if (g_slots[slotid].pid == pid) {
			if (g_slots[slotid].focus_handle || g_slots[slotid].focus_wcb_id) {
				if(g_slots[slotid].session_type != MM_SESSION_TYPE_CALL &&
					g_slots[slotid].session_type != MM_SESSION_TYPE_VIDEOCALL &&
					g_slots[slotid].session_type != MM_SESSION_TYPE_VOIP &&
					g_slots[slotid].session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
					g_slots[slotid].enable_session ) {
					if ((g_slots[slotid].session_options & MM_SESSION_OPTION_PAUSE_OTHERS) || g_slots[slotid].session_type == MM_SESSION_TYPE_ALARM || g_slots[slotid].session_type == MM_SESSION_TYPE_NOTIFY || g_slots[slotid].session_type == MM_SESSION_TYPE_EMERGENCY) {
						err = mm_sound_release_focus(g_slots[slotid].focus_handle, FOCUS_FOR_BOTH, NULL);
						if (err) {
							debug_error("mm_sound_release_focus failed [0x%x]", err);
						}
						if(mm_sound_unregister_focus(g_slots[slotid].focus_handle) || err) {
							debug_error("Focus clean up failed [0x%x]", err);
							err = MM_ERROR_POLICY_INTERNAL;
							goto cleanup;
						}
					} else if (~(g_slots[slotid].session_options & MM_SESSION_OPTION_PAUSE_OTHERS)) {
						err = mm_sound_unset_focus_watch_callback(g_slots[slotid].focus_wcb_id);
						if (err) {
							debug_error("mm_sound_unset_focus_watch_callback failed [0x%x]", err);
							err = MM_ERROR_POLICY_INTERNAL;
							goto cleanup;
						}
					}
				}
				if(mm_sound_remove_device_connected_callback(g_slots[slotid].subs_id) != MM_ERROR_NONE)
					debug_error("mm_sound_remove_device_connected_callback() failed");
				g_slots[slotid].focus_handle = 0;
				g_slots[slotid].focus_wcb_id = 0;
				g_slots[slotid].subs_id = 0;
			}
		}
	}

cleanup:
	pthread_mutex_unlock(&g_slot_mutex);
	debug_leave("(err : 0x%08X)\n", err);

	return err;
}


static int _MMSoundMgrCodecStopCallback(int param)
{
	int err = MM_ERROR_NONE;

	debug_enter("(Slot : %d)\n", param);

	pthread_mutex_lock(&g_slot_mutex);
	debug_msg("[CODEC MGR] Slot_mutex lock done\n");


	/*
	 * Unregister FOCUS here
	 */
	debug_msg("[CODEC MGR] enable_session %d ",g_slots[param].enable_session);

	if (g_slots[param].focus_handle || g_slots[param].focus_wcb_id) {
		if(g_slots[param].session_type != MM_SESSION_TYPE_CALL &&
			g_slots[param].session_type != MM_SESSION_TYPE_VIDEOCALL &&
			g_slots[param].session_type != MM_SESSION_TYPE_VOIP &&
			g_slots[param].session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
			g_slots[param].enable_session ) {
			if ((g_slots[param].session_options & MM_SESSION_OPTION_PAUSE_OTHERS) || g_slots[param].session_type == MM_SESSION_TYPE_ALARM || g_slots[param].session_type == MM_SESSION_TYPE_NOTIFY || g_slots[param].session_type == MM_SESSION_TYPE_EMERGENCY) {
				if(g_slots[param].current_focus_type != FOCUS_NONE) {
					err = mm_sound_release_focus(g_slots[param].focus_handle, g_slots[param].current_focus_type, NULL);
					if (err) {
						debug_error("mm_sound_release_focus failed [0x%x]", err);
					}
				}
				if(mm_sound_unregister_focus(g_slots[param].focus_handle) || err) {
					debug_error("Focus clean up failed [0x%x]", err);
					pthread_mutex_unlock(&g_slot_mutex);
					return MM_ERROR_POLICY_INTERNAL;
				}
			} else if (~(g_slots[param].session_options & MM_SESSION_OPTION_PAUSE_OTHERS)) {
				err = mm_sound_unset_focus_watch_callback(g_slots[param].focus_wcb_id);
				if (err) {
					debug_error("mm_sound_unset_focus_watch_callback failed [0x%x]", err);
					pthread_mutex_unlock(&g_slot_mutex);
					return MM_ERROR_POLICY_INTERNAL;
				}
			}
		}
		if(mm_sound_remove_device_connected_callback(g_slots[param].subs_id) != MM_ERROR_NONE)
			debug_error("mm_sound_remove_device_connected_callback() failed");
	}

	__mm_sound_mgr_ipc_notify_play_file_end(param);

	debug_msg("Client callback msg_type (instance) : [%d]\n", (int)g_slots[param].param);
	debug_msg("Handle allocated handle : [0x%08X]\n", g_slots[param].plughandle);
	err = g_plugins[g_slots[param].pluginid].Destroy(g_slots[param].plughandle);
	if (err < 0 ) {
		debug_critical("[CODEC MGR] Fail to destroy slot number : [%d] err [0x%x]\n", param, err);
	}
	memset(&g_slots[param], 0, sizeof(__mmsound_mgr_codec_handle_t));
	g_slots[param].status = STATUS_IDLE;
	pthread_mutex_unlock(&g_slot_mutex);
	debug_msg("[CODEC MGR] Slot_mutex done\n");

	return err;
}

static int _MMSoundMgrCodecGetEmptySlot(int *slot)
{
	int count = 0;
	int err = MM_ERROR_NONE;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif
	debug_msg("Codec slot ID : [%d]\n", *slot);
	pthread_mutex_lock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex LOCK\n");
#endif

	for (count = SOUND_SLOT_START; count < MANAGER_HANDLE_MAX ; count++) {
		if (g_slots[count].status == STATUS_IDLE) {
			g_slots[count].status = STATUS_SOUND;
			break;
		}
	}
	pthread_mutex_unlock(&g_slot_mutex);
#ifdef DEBUG_DETAIL
	debug_msg("After Slot_mutex UNLOCK\n");
#endif

	if (count < MANAGER_HANDLE_MAX)	{
		debug_msg("New handle allocated (codec slot ID : [%d])\n", count);
		*slot = count;
		err =  MM_ERROR_NONE;
	} else {
		debug_warning("Handle is full handle : [%d]\n", count);
		*slot = -1;
		/* Temporal code for reset */
		while(count--) {
			g_slots[count].status = STATUS_IDLE;
		}
		err =  MM_ERROR_SOUND_INTERNAL;
	}

#ifdef DEBUG_DETAIL
	debug_leave("\n");
#endif

	return err;
}

static int _MMSoundMgrCodecRegisterInterface(MMSoundPluginType *plugin)
{
	int err = MM_ERROR_NONE;
	int count = 0;
	void *getinterface = NULL;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	/* find emptry slot */
	for (count = 0; count < MM_SOUND_SUPPORTED_CODEC_NUM; count++) {
		if (g_plugins[count].GetSupportTypes == NULL)
			break;
	}

	if (count == MM_SOUND_SUPPORTED_CODEC_NUM) {
		debug_critical("The plugin support type is not valid\n");
		return MM_ERROR_COMMON_OUT_OF_RANGE;
	}

	err = MMSoundPluginGetSymbol(plugin, CODEC_GET_INTERFACE_FUNC_NAME, &getinterface);
	if (err != MM_ERROR_NONE) {
		debug_error("Get Symbol CODEC_GET_INTERFACE_FUNC_NAME is fail : %x\n", err);
		goto cleanup;
	}
	debug_msg("interface[%p] empty_slot[%d]\n", getinterface, count);

	err = MMSoundPlugCodecCastGetInterface(getinterface)(&g_plugins[count]);
	if (err != MM_ERROR_NONE) {
		debug_error("Get interface fail : %x\n", err);

cleanup:
		/* If error occur, clean interface */
		memset(&g_plugins[count], 0, sizeof(mmsound_codec_interface_t));
	} else {
		if (g_plugins[count].SetThreadPool)
			g_plugins[count].SetThreadPool(MMSoundThreadPoolRun);
	}

#ifdef DEBUG_DETAIL
	debug_leave("\n");
#endif

	return err;
}

