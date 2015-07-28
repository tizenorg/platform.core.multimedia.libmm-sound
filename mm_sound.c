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

#include <vconf.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_debug.h>
#include "include/mm_sound_private.h"
#include "include/mm_sound.h"
#include "include/mm_sound_utils.h"
#include "include/mm_sound_client.h"
#include "include/mm_sound_pa_client.h"
#include "include/mm_ipc.h"
#include "include/mm_sound_common.h"


#define VOLUME_MAX_MULTIMEDIA	16
#define VOLUME_MAX_BASIC		8
#define VOLUME_MAX_SINGLE		1


#define MASTER_VOLUME_MAX 100
#define MASTER_VOLUME_MIN 0


typedef struct {
	volume_callback_fn	func;
	void*				data;
	volume_type_t		type;
} volume_cb_param;

volume_cb_param g_volume_param[VOLUME_TYPE_MAX];

static pthread_mutex_t g_volume_mutex = PTHREAD_MUTEX_INITIALIZER;

#include <gio/gio.h>

static GList *g_subscribe_cb_list = NULL;
static pthread_mutex_t g_subscribe_cb_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MM_SOUND_DBUS_BUS_NAME_PREPIX  "org.tizen.MMSound"
#define MM_SOUND_DBUS_OBJECT_PATH  "/org/tizen/MMSound"
#define MM_SOUND_DBUS_INTERFACE    "org.tizen.mmsound"

GDBusConnection *g_dbus_conn_mmsound;

int g_dbus_signal_values[MM_SOUND_SIGNAL_MAX] = {0,};

const char* dbus_signal_name_str[] = {
	"ReleaseInternalFocus",
};

typedef struct _subscribe_cb {
	mm_sound_signal_name_t signal_type;
	mm_sound_signal_callback callback;
	void *user_data;
	unsigned int id;
} subscribe_cb_t;

static char* _get_volume_str (volume_type_t type)
{
	static const char *volume_type_str[VOLUME_TYPE_MAX] =
		{ "SYSTEM", "NOTIFICATION", "ALARM", "RINGTONE", "MEDIA", "CALL", "VOIP", "VOICE", "FIXED"};

	return (type >= VOLUME_TYPE_SYSTEM && type < VOLUME_TYPE_MAX)? volume_type_str[type] : "Unknown";
}

static int _validate_volume(volume_type_t type, int value)
{
	if (value < 0)
		return -1;

	switch (type)
	{
	case VOLUME_TYPE_CALL:
	case VOLUME_TYPE_VOIP:
		if (value >= VOLUME_MAX_BASIC) {
			return -1;
		}
		break;
	case VOLUME_TYPE_SYSTEM:
	case VOLUME_TYPE_MEDIA:
	case VOLUME_TYPE_ALARM:
	case VOLUME_TYPE_NOTIFICATION:
	case VOLUME_TYPE_RINGTONE:
	case VOLUME_TYPE_VOICE:
		if (value >= VOLUME_MAX_MULTIMEDIA) {
			return -1;
		}
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

static void _volume_changed_cb(keynode_t* node, void* data)
{
	volume_cb_param* param = (volume_cb_param*) data;

	debug_msg("%s changed callback called\n",vconf_keynode_get_name(node));

	MMSOUND_ENTER_CRITICAL_SECTION( &g_volume_mutex )

	if(param && (param->func != NULL)) {
		debug_log("function 0x%x\n", param->func);
		((volume_callback_fn)param->func)(param->data);
	}

	MMSOUND_LEAVE_CRITICAL_SECTION( &g_volume_mutex )
}

EXPORT_API
int mm_sound_volume_add_callback(volume_type_t type, volume_callback_fn func, void* user_data)
{
	debug_msg("type = (%d)%15s, func = %p, user_data = %p", type, _get_volume_str(type), func, user_data);

	/* Check input param */
	if (type < 0 || type >= VOLUME_TYPE_MAX) {
		debug_error("invalid argument\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (!func) {
		debug_warning("callback function is null\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN( &g_volume_mutex, MM_ERROR_SOUND_INTERNAL );

	g_volume_param[type].func = func;
	g_volume_param[type].data = user_data;
	g_volume_param[type].type = type;

	MMSOUND_LEAVE_CRITICAL_SECTION( &g_volume_mutex );

	return mm_sound_util_volume_add_callback(type, _volume_changed_cb, (void*)&g_volume_param[type]);
}

EXPORT_API
int mm_sound_volume_remove_callback(volume_type_t type)
{
	debug_msg("type = (%d)%s", type, _get_volume_str(type));

	if(type < 0 || type >=VOLUME_TYPE_MAX) {
		debug_error("invalid argument\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN( &g_volume_mutex, MM_ERROR_SOUND_INTERNAL );

	g_volume_param[type].func = NULL;
	g_volume_param[type].data = NULL;
	g_volume_param[type].type = type;

	MMSOUND_LEAVE_CRITICAL_SECTION( &g_volume_mutex );

	return mm_sound_util_volume_remove_callback(type, _volume_changed_cb);
}

EXPORT_API
int mm_sound_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	if (func == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_add_volume_changed_callback(func, user_data);
	if (ret < 0) {
		debug_error("Can not add volume changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_remove_volume_changed_callback(void)
{
	int ret = MM_ERROR_NONE;

	ret = mm_sound_client_remove_volume_changed_callback();
	if (ret < 0) {
		debug_error("Can not remove volume changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_volume_get_step(volume_type_t type, int *step)
{
	return MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
}

EXPORT_API
int mm_sound_volume_set_value(volume_type_t volume_type, const unsigned int volume_level)
{
	int ret = MM_ERROR_NONE;

	debug_msg("type = (%d)%s, value = %d", volume_type, _get_volume_str(volume_type), volume_level);

	/* Check input param */
	if (0 > _validate_volume(volume_type, (int)volume_level)) {
		debug_error("invalid volume type %d, value %u\n", volume_type, volume_level);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_util_volume_set_value_by_type(volume_type, volume_level);
	if (ret == MM_ERROR_NONE) {
		/* update shared memory value */
		if(MM_ERROR_NONE != mm_sound_client_set_volume_by_type(volume_type, volume_level)) {
			debug_error("Can not set volume to shared memory 0x%x\n", ret);
		}
	}

	return ret;
}

EXPORT_API
int mm_sound_volume_get_value(volume_type_t type, unsigned int *value)
{
	int ret = MM_ERROR_NONE;

	/* Check input param */
	if (value == NULL) {
		debug_error("invalid argument\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (type < 0 || type >= VOLUME_TYPE_MAX) {
		debug_error("invalid volume type value %d\n", type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_util_volume_get_value_by_type(type, value);

	debug_msg("returned %s = %d", _get_volume_str(type), *value);
	return ret;
}

EXPORT_API
int mm_sound_volume_primary_type_set(volume_type_t type)
{
	pid_t mypid;
	int ret = MM_ERROR_NONE;

	/* Check input param */
	if(type < VOLUME_TYPE_UNKNOWN || type >= VOLUME_TYPE_MAX) {
		debug_error("invalid argument\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (vconf_set_int(VCONFKEY_SOUND_PRIMARY_VOLUME_TYPE, type)) {
		debug_error("could not set vconf for RIMARY_VOLUME_TYPE\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		debug_msg("set primary volume type forcibly %d(%s)", type, _get_volume_str(type));
	}

	return ret;
}

EXPORT_API
int mm_sound_volume_primary_type_get(volume_type_t *type)
{
	int ret = MM_ERROR_NONE;
	int voltype = VOLUME_TYPE_RINGTONE;

	/* Check input param */
	if(type == NULL) {
		debug_error("invalid argument\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* check force set */
	if (vconf_get_int(VCONFKEY_SOUND_PRIMARY_VOLUME_TYPE, &voltype)) {
		debug_error("could not get vconf for PRIMARY_VOLUME_TYPE\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		debug_msg("get primary volume type %d(%s)", voltype, _get_volume_str(voltype));
		*type = voltype;
	}

	return ret;
}

/* it will be removed */
EXPORT_API
int mm_sound_volume_primary_type_clear(void)
{
	pid_t mypid;
	int ret = MM_ERROR_NONE;

	if (vconf_set_int(VCONFKEY_SOUND_PRIMARY_VOLUME_TYPE, -1)) {
		debug_error("could not reset vconf for PRIMARY_VOLUME_TYPE\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		debug_msg("clear primary volume type forcibly %d(%s)", -1, "none");
	}

	return ret;
}

///////////////////////////////////
////     MMSOUND PLAY APIs
///////////////////////////////////
static inline void _mm_sound_fill_play_param(MMSoundPlayParam *param, const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int priority, int handle_route)
{
	param->filename = filename;
	param->volume = 0; //volume value dose not effect anymore
	param->callback = callback;
	param->data = data;
	param->loop = 1;
	param->volume_config = volume_config;
	param->priority = priority;
	param->handle_route = handle_route;
}

EXPORT_API
int mm_sound_play_loud_solo_sound_no_restore(const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };

	_mm_sound_fill_play_param(&param, filename, volume_config, callback, data, HANDLE_PRIORITY_SOLO, MM_SOUND_HANDLE_ROUTE_SPEAKER_NO_RESTORE);
	return mm_sound_play_sound_ex(&param, handle);
}

EXPORT_API
int mm_sound_play_loud_solo_sound(const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };

	_mm_sound_fill_play_param(&param, filename, volume_config, callback, data, HANDLE_PRIORITY_SOLO, MM_SOUND_HANDLE_ROUTE_SPEAKER);
	return mm_sound_play_sound_ex(&param, handle);
}

EXPORT_API
int mm_sound_play_solo_sound(const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };

	_mm_sound_fill_play_param(&param, filename, volume_config, callback, data, HANDLE_PRIORITY_SOLO, MM_SOUND_HANDLE_ROUTE_USING_CURRENT);
	return mm_sound_play_sound_ex(&param, handle);
}

EXPORT_API
int mm_sound_play_sound_without_session(const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };

	_mm_sound_fill_play_param(&param, filename, volume_config, callback, data, HANDLE_PRIORITY_SOLO, MM_SOUND_HANDLE_ROUTE_USING_CURRENT);
	param.skip_session = true;
	return mm_sound_play_sound_ex(&param, handle);
}

EXPORT_API
int mm_sound_play_sound(const char *filename, int volume_config, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };

	_mm_sound_fill_play_param(&param, filename, volume_config, callback, data, HANDLE_PRIORITY_NORMAL, MM_SOUND_HANDLE_ROUTE_USING_CURRENT);
	return mm_sound_play_sound_ex(&param, handle);
}

EXPORT_API
int mm_sound_play_sound_ex(MMSoundPlayParam *param, int *handle)
{
	int err;
	int lhandle = -1;
	int volume_type = 0;
	/* Check input param */
	if (param == NULL) {
		debug_error("param is null\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	volume_type = MM_SOUND_VOLUME_CONFIG_TYPE(param->volume_config);

	if (param->filename == NULL) {
		debug_error("filename is NULL\n");
		return MM_ERROR_SOUND_FILE_NOT_FOUND;
	}
	if (volume_type < 0 || volume_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type is invalid %d\n", volume_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_warning ("play sound : priority=[%d], handle_route=[%d]\n", param->priority, param->handle_route);

	/* Play sound */
	err = mm_sound_client_play_sound(param, 0, &lhandle);
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

	debug_warning ("success : handle=[%p]\n", handle);

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_play_sound_with_stream_info(const char *filename, char *stream_type, int stream_id, mm_sound_stop_callback_func callback, void *data, int *handle)
{
	MMSoundPlayParam param = { 0, };
	int err;

	param.filename = filename;
	param.volume = 0; //volume value dose not effect anymore
	param.callback = callback;
	param.data = data;
	param.loop = 1;
	param.priority = HANDLE_PRIORITY_NORMAL;
	param.handle_route = MM_SOUND_HANDLE_ROUTE_USING_CURRENT;

	err = mm_sound_client_play_sound_with_stream_info(&param, handle, stream_type, stream_id);
	if (err < 0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	debug_warning ("success : handle=[%p]\n", handle);

	return MM_ERROR_NONE;

}


EXPORT_API
int mm_sound_stop_sound(int handle)
{
	int err;

	debug_warning ("enter : handle=[%p]\n", handle);
	/* Stop sound */
	err = mm_sound_client_stop_sound(handle);
	if (err < 0) {
		debug_error("Fail to stop sound\n");
		return err;
	}
	debug_warning ("success : handle=[%p]\n", handle);

	return MM_ERROR_NONE;
}

///////////////////////////////////
////     MMSOUND TONE APIs
///////////////////////////////////
EXPORT_API
int mm_sound_play_tone_ex (MMSoundTone_t num, int volume_config, const double volume, const int duration, int *handle, bool enable_session)
{
	int lhandle = -1;
	int err = MM_ERROR_NONE;
	int volume_type = MM_SOUND_VOLUME_CONFIG_TYPE(volume_config);

	debug_fenter();

	/* Check input param */
	if (duration < -1) {
		debug_error("number is invalid %d\n", duration);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (num < MM_SOUND_TONE_DTMF_0 || num >= MM_SOUND_TONE_NUM) {
		debug_error("TONE Value is invalid %d\n", num);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (volume_type < 0 || volume_type >= VOLUME_TYPE_MAX) {
		debug_error("Volume type is invalid %d\n", volume_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (volume < 0.0 || volume > 1.0) {
		debug_error("Volume Value is invalid %d\n", volume);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Play tone */
	debug_msg("Call MMSoundClientPlayTone\n");
	err = mm_sound_client_play_tone(num, volume_config, volume, duration, &lhandle, enable_session);
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

EXPORT_API
int mm_sound_play_tone_with_stream_info(MMSoundTone_t tone, char *stream_type, int stream_id, const double volume, const int duration, int *handle)
{

	int err = MM_ERROR_NONE;

	err = mm_sound_client_play_tone_with_stream_info(tone, stream_type, stream_id, volume, duration, handle);
	if (err <0) {
		debug_error("Failed to play sound\n");
		return err;
	}

	return err;

}


EXPORT_API
int mm_sound_play_tone (MMSoundTone_t num, int volume_config, const double volume, const int duration, int *handle)
{
	return mm_sound_play_tone_ex (num, volume_config, volume, duration, handle, true);
}

///////////////////////////////////
////     MMSOUND ROUTING APIs
///////////////////////////////////

EXPORT_API
int mm_sound_route_get_a2dp_status (bool *connected, char **bt_name)
{
	int ret = MM_ERROR_NONE;

	if (connected == NULL || bt_name == NULL) {
		debug_error ("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_is_bt_a2dp_on (connected, bt_name);
	debug_msg ("connected=[%d] bt_name[%s]\n", *connected, *bt_name);
	if (ret < 0) {
		debug_error("MMSoundClientIsBtA2dpOn() Failed\n");
		return ret;
	}

	return ret;
}

EXPORT_API
int mm_sound_is_route_available(mm_sound_route route, bool *is_available)
{
	int ret = MM_ERROR_NONE;

	debug_warning ("enter : route=[%x], is_available=[%p]\n", route, is_available);

	if (!mm_sound_util_is_route_valid(route)) {
		debug_error("route is invalid %d\n", route);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (!is_available) {
		debug_error("is_available is invalid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_is_route_available(route, is_available);
	if (ret < 0) {
		debug_error("Can not check given route is available, ret = %x\n", ret);
	} else {
		debug_warning ("success : route=[%x], available=[%d]\n", route, *is_available);
	}

	return ret;
}

EXPORT_API
int mm_sound_foreach_available_route_cb(mm_sound_available_route_cb available_route_cb, void *user_data)
{
	int ret = MM_ERROR_NONE;

	if (!available_route_cb) {
		debug_error("available_route_cb is invalid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_foreach_available_route_cb(available_route_cb, user_data);
	if (ret < 0) {
		debug_error("Can not set foreach available route callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_set_active_route(mm_sound_route route)
{
	int ret = MM_ERROR_NONE;

	debug_warning ("enter : route=[%x]\n", route);
	if (!mm_sound_util_is_route_valid(route)) {
		debug_error("route is invalid %d\n", route);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_set_active_route(route, true);
	if (ret < 0) {
		debug_error("Can not set active route, ret = %x\n", ret);
	} else {
		debug_warning ("success : route=[%x]\n", route);
	}

	return ret;
}

EXPORT_API
int mm_sound_set_active_route_auto(void)
{
	int ret = MM_ERROR_NONE;

	ret = mm_sound_client_set_active_route_auto();
	if (ret < 0) {
		debug_error("fail to set active route auto, ret = %x\n", ret);
	} else {
		debug_msg ("success !!\n");
	}

	return ret;
}

EXPORT_API
int mm_sound_set_active_route_without_broadcast(mm_sound_route route)
{
	int ret = MM_ERROR_NONE;

	debug_warning ("enter : route=[%x]\n", route);
	if (!mm_sound_util_is_route_valid(route)) {
		debug_error("route is invalid %d\n", route);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_set_active_route(route, false);
	if (ret < 0) {
		debug_error("Can not set active route, ret = %x\n", ret);
	} else {
		debug_warning ("success : route=[%x]\n", route);
	}

	return ret;
}

EXPORT_API
int mm_sound_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	if (device_in == NULL || device_out == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_get_active_device(device_in, device_out);
	if (ret < 0) {
		debug_error("Can not add active device callback, ret = %x\n", ret);
	} else {
		debug_msg ("success : in=[%x], out=[%x]\n", *device_in, *device_out);
	}

	return ret;
}

EXPORT_API
int mm_sound_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	if (device_in == NULL || device_out == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_get_audio_path(device_in, device_out);
	if (ret < 0) {
		debug_error("Can not add active device callback, ret = %x\n", ret);
	} else {
		debug_msg ("success : in=[%x], out=[%x]\n", *device_in, *device_out);
	}

	return ret;
}

EXPORT_API
int mm_sound_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_warning ("enter %s\n", name);
	if (func == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_add_active_device_changed_callback(name, func, user_data);
	if (ret < 0) {
		debug_error("Can not add active device changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_remove_active_device_changed_callback(const char *name)
{
	int ret = MM_ERROR_NONE;

	debug_warning ("enter name %s \n", name);
	ret = mm_sound_client_remove_active_device_changed_callback(name);
	if (ret < 0) {
		debug_error("Can not remove active device changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void *user_data)
{
	int ret = MM_ERROR_NONE;

	if (func == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_add_available_route_changed_callback(func, user_data);
	if (ret < 0) {
		debug_error("Can not add available route changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_remove_available_route_changed_callback(void)
{
	int ret = MM_ERROR_NONE;

	ret = mm_sound_client_remove_available_route_changed_callback();
	if (ret < 0) {
		debug_error("Can not remove available route changed callback, ret = %x\n", ret);
	}

	return ret;
}

EXPORT_API
int mm_sound_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	int ret = MM_ERROR_NONE;

	ret = mm_sound_client_set_sound_path_for_active_device(device_out, device_in);
	if (ret < 0) {
		debug_error("Can not mm sound set sound path for active device, ret = %x\n", ret);
	}

	return ret;
}


EXPORT_API
int mm_sound_test(int a, int b, int* getv)
{
	int ret = MM_ERROR_NONE;

	debug_log("mm_sound_test enter");
	if (!getv) {
		debug_error("argu null");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	ret = mm_sound_client_test(a, b, getv);
	if (ret < 0) {
		debug_error("Can not mm sound test, ret = %x\n", ret);
	}
	debug_log("mm_sound_test leave");

	return ret;
}

EXPORT_API
int mm_sound_add_test_callback(unsigned int *id, mm_sound_test_cb func, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_log("mm_sound_add_test_callback enter");
	if (!func) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_add_test_callback(id, func, user_data);
	if (ret < 0) {
		debug_error("Can not add test callback, ret = %x\n", ret);
	}
	debug_log("mm_sound_add_test_callback leave");

	return ret;
}

EXPORT_API
int mm_sound_remove_test_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_log("mm_sound_remove_test_callback enter");
	ret = mm_sound_client_remove_test_callback();
	if (ret < 0) {
		debug_error("Can not remove test callback, ret = %x\n", ret);
	}
	debug_log("mm_sound_remove_test_callback leave");

	return ret;
}

static void signal_callback(GDBusConnection *conn,
							   const gchar *sender_name,
							   const gchar *object_path,
							   const gchar *interface_name,
							   const gchar *signal_name,
							   GVariant *parameters,
							   gpointer user_data)
{
	int value=0;
	const GVariantType* value_type;

	debug_msg ("sender : %s, object : %s, interface : %s, signal : %s",
			sender_name, object_path, interface_name, signal_name);
	if(g_variant_is_of_type(parameters, G_VARIANT_TYPE("(i)"))) {
		g_variant_get(parameters, "(i)",&value);
		debug_msg(" - value : %d\n", value);
		_dbus_signal_callback (signal_name, value, user_data);
	} else	{
		value_type = g_variant_get_type(parameters);
		debug_warning("signal type is %s", value_type);
	}
}

int _convert_signal_name_str_to_enum (const char *name_str, mm_sound_signal_name_t *name_enum) {
	int ret = MM_ERROR_NONE;

	if (!name_str || !name_enum)
		return MM_ERROR_INVALID_ARGUMENT;

	if (!strncmp(name_str, "ReleaseInternalFocus", strlen("ReleaseInternalFocus"))) {
		*name_enum = MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS;
	} else {
		ret = MM_ERROR_INVALID_ARGUMENT;
		LOGE("not supported signal name(%s), err(0x%08x)", name_str, ret);
	}
	return ret;
}

void _dbus_signal_callback (const char *signal_name, int value, void *user_data)
{
	int ret = MM_ERROR_NONE;
	mm_sound_signal_name_t signal;
	subscribe_cb_t *subscribe_cb = (subscribe_cb_t*)user_data;

	debug_fenter();

	if (!subscribe_cb)
		return;

	ret = _convert_signal_name_str_to_enum(signal_name, &signal);
	if (ret)
		return;

	debug_msg ("signal_name[%s], value[%d], user_data[0x%x]\n", signal_name, value, user_data);

	if (subscribe_cb->signal_type == MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS) {
		/* trigger the signal callback when it comes from the same process */
		if (getpid() == ((value & 0xFFFF0000) >> 16)) {
			subscribe_cb->callback(signal, (value & 0x0000FFFF), subscribe_cb->user_data);
		}
	} else {
		subscribe_cb->callback(signal, value, subscribe_cb->user_data);
	}

	debug_fleave();

	return;
}

EXPORT_API
int mm_sound_subscribe_signal(mm_sound_signal_name_t signal, unsigned int *subscribe_id, mm_sound_signal_callback callback, void *user_data)
{
	int ret = MM_ERROR_NONE;
	GError *err = NULL;

	subscribe_cb_t *subscribe_cb = NULL;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_subscribe_cb_list_mutex, MM_ERROR_SOUND_INTERNAL);

	if (signal < 0 || signal >= MM_SOUND_SIGNAL_MAX || !subscribe_id) {
		debug_error ("invalid argument, signal(%d), subscribe_id(0x%p)", signal, subscribe_id);
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto error;
	}

	subscribe_cb = malloc(sizeof(subscribe_cb_t));
	if (!subscribe_cb) {
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}
	memset(subscribe_cb, 0, sizeof(subscribe_cb_t));

	g_type_init();

	g_dbus_conn_mmsound = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!g_dbus_conn_mmsound && err) {
		debug_error ("g_bus_get_sync() error (%s) ", err->message);
		g_error_free (err);
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}

	subscribe_cb->signal_type = signal;
	subscribe_cb->callback = callback;
	subscribe_cb->user_data = user_data;

	*subscribe_id = g_dbus_connection_signal_subscribe(g_dbus_conn_mmsound,
			NULL, MM_SOUND_DBUS_INTERFACE, dbus_signal_name_str[signal], MM_SOUND_DBUS_OBJECT_PATH, NULL, 0,
			signal_callback, subscribe_cb, NULL);
	if (*subscribe_id == 0) {
		debug_error ("g_dbus_connection_signal_subscribe() error (%d)", *subscribe_id);
		ret = MM_ERROR_SOUND_INTERNAL;
		goto sig_error;
	}

	subscribe_cb->id = *subscribe_id;

	g_subscribe_cb_list = g_list_append(g_subscribe_cb_list, subscribe_cb);
	if (g_subscribe_cb_list) {
		debug_log("new subscribe_cb(0x%x)[user_callback(0x%x), subscribe_id(%u)] is added\n", subscribe_cb, subscribe_cb->callback, subscribe_cb->id);
	} else {
		debug_error("g_list_append failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	debug_fleave();

	return ret;

sig_error:
	g_dbus_connection_signal_unsubscribe(g_dbus_conn_mmsound, *subscribe_id);
	g_object_unref(g_dbus_conn_mmsound);

error:
	if (subscribe_cb)
		free (subscribe_cb);

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	return ret;
}

EXPORT_API
void mm_sound_unsubscribe_signal(unsigned int subscribe_id)
{
	GList *list = NULL;
	subscribe_cb_t *subscribe_cb = NULL;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_subscribe_cb_list_mutex, MM_ERROR_SOUND_INTERNAL);

	if (g_dbus_conn_mmsound && subscribe_id) {
		g_dbus_connection_signal_unsubscribe(g_dbus_conn_mmsound, subscribe_id);
		g_object_unref(g_dbus_conn_mmsound);
		for (list = g_subscribe_cb_list; list != NULL; list = list->next) {
			subscribe_cb = (subscribe_cb_t *)list->data;
			if (subscribe_cb && (subscribe_cb->id == subscribe_id)) {
				g_subscribe_cb_list = g_list_remove(g_subscribe_cb_list, subscribe_cb);
				debug_log("subscribe_cb(0x%x) is removed\n", subscribe_cb);
				free (subscribe_cb);
			}
		}
	}

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	debug_fleave();
}

EXPORT_API
int mm_sound_send_signal(mm_sound_signal_name_t signal, int value)
{
	int ret = MM_ERROR_NONE;
	GError *err = NULL;
	GDBusConnection *conn = NULL;
	gboolean dbus_ret = TRUE;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_subscribe_cb_list_mutex, MM_ERROR_SOUND_INTERNAL);

	if (signal < 0 || signal >= MM_SOUND_SIGNAL_MAX) {
		debug_error ("invalid argument, signal(%d)", signal);
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto error;
	}

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!conn && err) {
		debug_error ("g_bus_get_sync() error (%s)", err->message);
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}

	g_dbus_signal_values[signal] = value;
	if (signal == MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS) {
		/* trigger the signal callback when it comes from the same process */
		value |= ((int)getpid() << 16);
	}
	dbus_ret = g_dbus_connection_emit_signal (conn,
				NULL, MM_SOUND_DBUS_OBJECT_PATH, MM_SOUND_DBUS_INTERFACE, dbus_signal_name_str[signal],
				g_variant_new ("(i)", value),
				&err);
	if (!dbus_ret && err) {
		debug_error ("g_dbus_connection_emit_signal() error (%s)", err->message);
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}

	dbus_ret = g_dbus_connection_flush_sync(conn, NULL, &err);
	if (!dbus_ret && err) {
		debug_error ("g_dbus_connection_flush_sync() error (%s)", err->message);
		ret = MM_ERROR_SOUND_INTERNAL;
		goto error;
	}

	g_object_unref(conn);
	debug_msg ("sending signal[%s], value[%d] success", dbus_signal_name_str[signal], value);

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	debug_fleave();

	return ret;

error:
	if (err)
		g_error_free (err);
	if (conn)
		g_object_unref(conn);

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	return ret;
}

EXPORT_API
int mm_sound_get_signal_value(mm_sound_signal_name_t signal, int *value)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_subscribe_cb_list_mutex, MM_ERROR_SOUND_INTERNAL);

	*value = g_dbus_signal_values[signal];

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_subscribe_cb_list_mutex);

	debug_fleave();

	return ret;
}

__attribute__ ((constructor))
static void _mm_sound_initialize(void)
{
	mm_sound_client_initialize();
	/* Will be Fixed */
}

__attribute__ ((destructor))
static void _mm_sound_finalize(void)
{
	mm_sound_client_finalize();
}


