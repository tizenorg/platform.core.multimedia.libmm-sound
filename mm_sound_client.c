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
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include <mm_error.h>
#include <mm_debug.h>
//#include <glib.h>

#include "include/mm_sound.h"
#include "include/mm_sound_msg.h"
#include "include/mm_sound_client.h"
#include "include/mm_sound_client_dbus.h"
#include "include/mm_sound_common.h"
#include "include/mm_sound_device.h"
#ifdef USE_FOCUS
#include "include/mm_sound_focus.h"
#endif

#include <mm_session.h>
#include <mm_session_private.h>

#define __DIRECT_CALLBACK__
//#define __GIDLE_CALLBACK__

#include <glib.h>
#if defined(__GSOURCE_CALLBACK__)
#include <sys/poll.h>
#endif

#define CLIENT_HANDLE_MAX 256

/* global variables for device list */
//static GList *g_device_list = NULL;
static mm_sound_device_list_t g_device_list_t;
static pthread_mutex_t g_device_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_id_mutex = PTHREAD_MUTEX_INITIALIZER;

guint g_focus_signal_handle = 0;

int mm_sound_client_initialize(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	mm_sound_client_dbus_initialize();

	debug_fleave();
	return ret;
}

int mm_sound_client_finalize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_finalize();

	debug_fleave();
	return ret;
}

int mm_sound_client_is_route_available(mm_sound_route route, bool *is_available)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_is_route_available(route, is_available);

	debug_fleave();
	return ret;

}

int mm_sound_client_foreach_available_route_cb(mm_sound_available_route_cb available_route_cb, void *user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_foreach_available_route_cb(available_route_cb, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_set_active_route(mm_sound_route route, bool need_broadcast)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_set_active_route(route, need_broadcast);

	debug_fleave();
	return ret;

}

int mm_sound_client_set_active_route_auto(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_set_active_route_auto();

	debug_fleave();
	return ret;

}

int mm_sound_client_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{

	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_get_active_device(device_in, device_out);

	debug_fleave();
	return ret;
}

int mm_sound_client_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_add_active_device_changed_callback(name, func, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_active_device_changed_callback(const char *name)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_active_device_changed_callback(name);

	debug_fleave();
	return ret;
}
int mm_sound_client_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_add_available_route_changed_callback(func, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_available_route_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_available_route_changed_callback();

	debug_fleave();
	return ret;
}

int mm_sound_client_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_set_sound_path_for_active_device(device_out, device_in);

	debug_fleave();
	return ret;
}

void mm_sound_convert_volume_type_to_stream_type(int volume_type, char *stream_type)
{
	switch (volume_type) {
	case VOLUME_TYPE_SYSTEM:
		strncpy(stream_type, "system", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_NOTIFICATION:
		strncpy(stream_type, "notification", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_ALARM:
		strncpy(stream_type, "alarm", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_RINGTONE:
		strncpy(stream_type, "ringtone-voip", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_MEDIA:
		strncpy(stream_type, "media", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_CALL:
		strncpy(stream_type, "system", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_VOIP:
		strncpy(stream_type, "voip", MM_SOUND_STREAM_TYPE_LEN);
		break;
	case VOLUME_TYPE_VOICE:
		strncpy(stream_type, "voice-recognition", MM_SOUND_STREAM_TYPE_LEN);
		break;
	default:
		strncpy(stream_type, "media", MM_SOUND_STREAM_TYPE_LEN);
		break;
	}

	debug_error("volume type (%d) converted to stream type (%s)", volume_type, stream_type);

}

/*****************************************************************************************
			    DBUS SUPPORTED FUNCTIONS
******************************************************************************************/

void _mm_sound_client_focus_signal_callback(mm_sound_signal_name_t signal, int value, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_error("focus signal received, value = %d", value);

	if (value == 1) {
		ret = mm_sound_client_dbus_clear_focus(getpid());
		if (ret)
			debug_error("clear focus failed ret = 0x%x", ret);
		mm_sound_unsubscribe_signal(g_focus_signal_handle);
		g_focus_signal_handle = 0;
	}
}

int mm_sound_client_play_tone(int number, int volume_config, double volume, int time, int *handle, bool enable_session)
{
	int ret = MM_ERROR_NONE;
//	 int instance = -1; 	/* instance is unique to communicate with server : client message queue filter type */
	int volume_type = MM_SOUND_VOLUME_CONFIG_TYPE(volume_config);
	char stream_type[MM_SOUND_STREAM_TYPE_LEN] = {0, };

	 debug_fenter();

	/* read session information */
	int session_type = MM_SESSION_TYPE_MEDIA;
	int session_options = 0;
	int is_focus_registered = 0;

	ret = mm_sound_get_signal_value(MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS, &is_focus_registered);
	if (ret) {
		debug_error("mm_sound_get_signal_value failed [0x%x]", ret);
		return MM_ERROR_POLICY_INTERNAL;
	}

	if (is_focus_registered)
		enable_session = false;

	if (enable_session)
	{
		if (MM_ERROR_NONE != _mm_session_util_read_information(-1, &session_type, &session_options))
		{
			debug_warning("[Client] Read Session Information failed. use default \"media\" type\n");
			session_type = MM_SESSION_TYPE_MEDIA;

			if(MM_ERROR_NONE != mm_session_init(session_type))
			{
				debug_critical("[Client] MMSessionInit() failed\n");
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

	 // instance = getpid();
	 //debug_log("[Client] pid for client ::: [%d]\n", instance);

	 /* Send msg */
	 debug_msg("[Client] Input number : %d\n", number);
	 /* Send req memory */

	mm_sound_convert_volume_type_to_stream_type(volume_type, stream_type);
	ret = mm_sound_client_dbus_play_tone(number, time, volume, volume_config,
					session_type, session_options, getpid(), enable_session, handle, stream_type, -1);

	if (enable_session && !g_focus_signal_handle) {
		ret = mm_sound_subscribe_signal(MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS, &g_focus_signal_handle, _mm_sound_client_focus_signal_callback, NULL);
		if (ret) {
			debug_error("mm_sound_subscribe_signal failed [0x%x]", ret);
			return MM_ERROR_POLICY_INTERNAL;
		}
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_play_tone_with_stream_info(int tone, char *stream_type, int stream_id, double volume, int duration, int *handle)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_play_tone_with_stream_info(getpid(), tone, stream_type, stream_id, volume, duration, handle);

	debug_fleave();
	return ret;
}

int mm_sound_client_play_sound(MMSoundPlayParam *param, int tone, int *handle)
{
	int ret = MM_ERROR_NONE;
	int session_type = MM_SESSION_TYPE_MEDIA;
	int session_options = 0;
	int is_focus_registered = 0;
//	int instance = -1; 	/* instance is unique to communicate with server : client message queue filter type */
	int volume_type = MM_SOUND_VOLUME_CONFIG_TYPE(param->volume_config);
	char stream_type[MM_SOUND_STREAM_TYPE_LEN] = {0, };

	debug_fenter();

	/* read session information */

	ret = mm_sound_get_signal_value(MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS, &is_focus_registered);
	if (ret) {
		debug_error("mm_sound_get_signal_value failed [0x%x]", ret);
		return MM_ERROR_POLICY_INTERNAL;
	}

	if (is_focus_registered)
		param->skip_session = true;

	if (param->skip_session == false) {
		if(MM_ERROR_NONE != _mm_session_util_read_information(-1, &session_type, &session_options))
		{
			debug_warning("[Client] Read MMSession Type failed. use default \"media\" type\n");
			session_type = MM_SESSION_TYPE_MEDIA;

			if(MM_ERROR_NONE != mm_session_init(session_type))
			{
				debug_critical("[Client] MMSessionInit() failed\n");
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

//	instance = getpid();
// 	debug_msg("[Client] pid for client ::: [%d]\n", instance);

	/* Send msg */
	if ((param->mem_ptr && param->mem_size))
	{
		// Play memory, deprecated
		return MM_ERROR_INVALID_ARGUMENT;
	}

	mm_sound_convert_volume_type_to_stream_type(volume_type, stream_type);
	ret = mm_sound_client_dbus_play_sound(param->filename, tone, param->loop, param->volume, param->volume_config,
					 param->priority, session_type, session_options, getpid(), param->handle_route,
					 !param->skip_session, handle, stream_type, -1);
	if (ret != MM_ERROR_NONE) {
		debug_error("Play Sound Failed");
		goto failed;
	}
	if (param->callback) {
		ret = mm_sound_client_dbus_add_play_sound_end_callback(*handle, param->callback, param->data);
		if (ret != MM_ERROR_NONE) {
			debug_error("Add callback for play sound(%d) Failed", *handle);
		}
	}
	if (!param->skip_session && !g_focus_signal_handle) {
		ret = mm_sound_subscribe_signal(MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS, &g_focus_signal_handle, _mm_sound_client_focus_signal_callback, NULL);
		if (ret) {
			debug_error("mm_sound_subscribe_signal failed [0x%x]", ret);
			return MM_ERROR_POLICY_INTERNAL;
		}
	}

failed:

	debug_fleave();
	return ret;
}

int mm_sound_client_play_sound_with_stream_info(MMSoundPlayParam *param, int *handle, char* stream_type, int stream_id)
{
	int ret = MM_ERROR_NONE;

	ret = mm_sound_client_dbus_play_sound_with_stream_info(param->filename, param->loop, param->volume,
					 param->priority, getpid(), param->handle_route, handle, stream_type, stream_id);
	if (ret != MM_ERROR_NONE) {
		debug_error("Play Sound Failed");
		goto failed;
	}
	if (param->callback) {
		ret = mm_sound_client_dbus_add_play_sound_end_callback(*handle, param->callback, param->data);
		if (ret != MM_ERROR_NONE) {
			debug_error("Add callback for play sound(%d) Failed", *handle);
		}
	}

failed:

	debug_fleave();
	return ret;

}

int mm_sound_client_stop_sound(int handle)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if (handle < 0 || handle > CLIENT_HANDLE_MAX) {
		ret = MM_ERROR_INVALID_ARGUMENT;
		return ret;
	}

	ret = mm_sound_client_dbus_stop_sound(handle);

	debug_fleave();
	return ret;
}

static int _mm_sound_client_device_list_clear ()
{
	int ret = MM_ERROR_NONE;

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_device_list_mutex, MM_ERROR_SOUND_INTERNAL);

	if (g_device_list_t.list) {
		g_list_free_full(g_device_list_t.list, g_free);
		g_device_list_t.list = NULL;
	}

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_device_list_mutex);

	return ret;
}

static int _mm_sound_client_device_list_dump (GList *device_list)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	mm_sound_device_t *device_node = NULL;
	int count = 0;
	if (!device_list) {
		debug_error("Device list NULL, cannot dump list");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_log("======================== device list : start ==========================\n");
	for (list = device_list; list != NULL; list = list->next) {
		device_node = (mm_sound_device_t *)list->data;
		if (device_node) {
			debug_log(" list idx[%d]: type[%17s], id[%02d], io_direction[%d], state[%d], name[%s]\n",
						count++, device_node->type, device_node->id, device_node->io_direction, device_node->state, device_node->name);
		}
	}
	debug_log("======================== device list : end ============================\n");

	return ret;
}

int mm_sound_client_get_current_connected_device_list(int device_flags, mm_sound_device_list_t **device_list)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_device_list_clear();
	if (ret) {
		debug_error("[Client] failed to __mm_sound_client_device_list_clear(), ret[0x%x]\n", ret);
		return ret;
	}

	if ((ret = mm_sound_client_dbus_get_current_connected_device_list(device_flags, &g_device_list_t.list)) != MM_ERROR_NONE) {
		debug_error("[Client] failed to get current connected device list with dbus, ret[0x%x]", ret);
		goto failed;
	}
	if (!g_device_list_t.list) {
		debug_error("Got device list null");
		ret = MM_ERROR_SOUND_NO_DATA;
		goto failed;
	}
//		g_device_list_t.list = g_device_list;
	_mm_sound_client_device_list_dump(g_device_list_t.list);
	*device_list = &g_device_list_t;

failed:
	debug_fleave();
	return ret;
}

int mm_sound_client_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_add_device_connected_callback(device_flags, func, user_data);

	debug_fleave();
	return ret;

}

int mm_sound_client_remove_device_connected_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_device_connected_callback();

	debug_fleave();
	return ret;
}

int mm_sound_client_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_add_device_info_changed_callback(device_flags, func, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_device_info_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret =  mm_sound_client_dbus_remove_device_info_changed_callback();

	debug_fleave();
	return ret;

}
int mm_sound_client_is_bt_a2dp_on (bool *connected, char** bt_name)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_is_bt_a2dp_on(connected, bt_name);

	debug_fleave();
	return ret;
}

int mm_sound_client_set_volume_by_type(const int volume_type, const unsigned int volume_level)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_set_volume_by_type(volume_type, volume_level);

	debug_fleave();
	return ret;
}

int mm_sound_client_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_add_volume_changed_callback(func, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_volume_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_volume_changed_callback();

	debug_fleave();
	return ret;
}


int mm_sound_client_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_get_audio_path(device_in, device_out);

	debug_fleave();

	return ret;
}


#ifdef USE_FOCUS
void mm_sound_client_set_session_interrupt_callback(mm_sound_focus_session_interrupt_cb callback)
{
	debug_fenter();

	mm_sound_client_dbus_set_session_interrupt_callback(callback);

	debug_fleave();
	return;
}

int mm_sound_client_unset_session_interrupt_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_unset_session_interrupt_callback();

	debug_fleave();
	return ret;
}

int mm_sound_client_get_uniq_id(int *id)
{
	static int uniq_id = 0;
	int ret = MM_ERROR_NONE;

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_id_mutex, MM_ERROR_SOUND_INTERNAL);
	debug_fenter();

	if (!id)
		ret = MM_ERROR_INVALID_ARGUMENT;
	else
		*id = ++uniq_id;

	debug_fleave();
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_id_mutex);

	return ret;
}

int mm_sound_client_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_register_focus(id, stream_type, callback, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_unregister_focus(id);

	debug_fleave();
	return ret;
}

int mm_sound_client_acquire_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_acquire_focus(id, type, option);

	debug_fleave();
	return ret;
}

int mm_sound_client_release_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	mm_sound_client_dbus_release_focus(id, type, option);

	debug_fleave();
	return ret;
}

int mm_sound_client_set_focus_watch_callback(mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void* user_data, int *id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	mm_sound_client_dbus_set_focus_watch_callback(focus_type, callback, user_data, id);

	debug_fleave();
	return ret;
}

int mm_sound_client_unset_focus_watch_callback(int id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_unset_focus_watch_callback(id);

	debug_fleave();
	return ret;
}
#endif


int mm_sound_client_add_test_callback(mm_sound_test_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_add_test_callback(func, user_data);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_test_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_test_callback();

	debug_fleave();
	return ret;
}


int mm_sound_client_test(int a, int b, int* getv)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_test(a, b, getv);
	debug_log("%d * %d -> result : %d", a, b, *getv);

	debug_fleave();

	return ret;
}
