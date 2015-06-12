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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <vconf.h>
#include <vconf-keys.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mm_sound_private.h"
#include "../include/mm_sound.h"
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_utils.h"

static mm_sound_route g_valid_route[] = {
		MM_SOUND_ROUTE_OUT_SPEAKER, MM_SOUND_ROUTE_OUT_RECEIVER, MM_SOUND_ROUTE_OUT_WIRED_ACCESSORY, MM_SOUND_ROUTE_OUT_BLUETOOTH_SCO, MM_SOUND_ROUTE_OUT_BLUETOOTH_A2DP,
		MM_SOUND_ROUTE_OUT_DOCK, MM_SOUND_ROUTE_OUT_HDMI, MM_SOUND_ROUTE_OUT_MIRRORING, MM_SOUND_ROUTE_OUT_USB_AUDIO, MM_SOUND_ROUTE_OUT_MULTIMEDIA_DOCK,
		MM_SOUND_ROUTE_IN_MIC, MM_SOUND_ROUTE_IN_WIRED_ACCESSORY, MM_SOUND_ROUTE_IN_MIC_OUT_RECEIVER,
		MM_SOUND_ROUTE_IN_MIC_OUT_SPEAKER, MM_SOUND_ROUTE_IN_MIC_OUT_HEADPHONE,
		MM_SOUND_ROUTE_INOUT_HEADSET, MM_SOUND_ROUTE_INOUT_BLUETOOTH
};

#define MM_SOUND_DEFAULT_VOLUME_SYSTEM			9
#define MM_SOUND_DEFAULT_VOLUME_NOTIFICATION	11
#define MM_SOUND_DEFAULT_VOLUME_ALARAM			7
#define MM_SOUND_DEFAULT_VOLUME_RINGTONE		11
#define MM_SOUND_DEFAULT_VOLUME_MEDIA			7
#define MM_SOUND_DEFAULT_VOLUME_CALL			4
#define MM_SOUND_DEFAULT_VOLUME_VOIP			4
#define MM_SOUND_DEFAULT_VOLUME_VOICE			7
#define MM_SOUND_DEFAULT_VOLUME_ANDROID			0

static char *g_volume_vconf[VOLUME_TYPE_MAX] = {
	VCONF_KEY_VOLUME_TYPE_SYSTEM,		/* VOLUME_TYPE_SYSTEM */
	VCONF_KEY_VOLUME_TYPE_NOTIFICATION,	/* VOLUME_TYPE_NOTIFICATION */
	VCONF_KEY_VOLUME_TYPE_ALARM,		/* VOLUME_TYPE_ALARM */
	VCONF_KEY_VOLUME_TYPE_RINGTONE,		/* VOLUME_TYPE_RINGTONE */
	VCONF_KEY_VOLUME_TYPE_MEDIA,		/* VOLUME_TYPE_MEDIA */
	VCONF_KEY_VOLUME_TYPE_CALL,			/* VOLUME_TYPE_CALL */
	VCONF_KEY_VOLUME_TYPE_VOIP,			/* VOLUME_TYPE_VOIP */
	VCONF_KEY_VOLUME_TYPE_VOICE,		/* VOLUME_TYPE_VOICE */
	VCONF_KEY_VOLUME_TYPE_ANDROID		/* VOLUME_TYPE_FIXED */
};
static char *g_volume_str[VOLUME_TYPE_MAX] = {
	"SYSTEM",
	"NOTIFICATION",
	"ALARM",
	"RINGTONE",
	"MEDIA",
	"CALL",
	"VOIP",
	"VOICE",
	"FIXED",
};

EXPORT_API
int mm_sound_util_get_valid_route_list(mm_sound_route **route_list)
{
	*route_list = g_valid_route;

	return (int)(sizeof(g_valid_route) / sizeof(mm_sound_route));
}

EXPORT_API
bool mm_sound_util_is_route_valid(mm_sound_route route)
{
	mm_sound_route *route_list = 0;
	int route_index = 0;
	int route_list_count = 0;

	route_list_count = mm_sound_util_get_valid_route_list(&route_list);
	for (route_index = 0; route_index < route_list_count; route_index++) {
		if (route_list[route_index] == route)
			return 1;
	}

	return 0;
}

EXPORT_API
void mm_sound_util_get_devices_from_route(mm_sound_route route, mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	if (device_in && device_out) {
		*device_in = route & 0x00FF;
		*device_out = route & 0xFFF00;
	}
}

EXPORT_API
int mm_sound_util_volume_add_callback(volume_type_t type, void *func, void* user_data)
{
	if (vconf_notify_key_changed(g_volume_vconf[type], func, user_data)) {
		debug_error ("vconf_notify_key_changed failed..\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_util_volume_remove_callback(volume_type_t type, void *func)
{
	if (vconf_ignore_key_changed(g_volume_vconf[type], func)) {
		debug_error ("vconf_ignore_key_changed failed..\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_util_volume_get_value_by_type(volume_type_t type, unsigned int *value)
{
	int ret = MM_ERROR_NONE;
	int vconf_value = 0;

	/* Get volume value from VCONF */
	if (vconf_get_int(g_volume_vconf[type], &vconf_value)) {
		debug_error ("vconf_get_int(%s) failed..\n", g_volume_vconf[type]);
		return MM_ERROR_SOUND_INTERNAL;
	}

	*value = vconf_value;
	if (ret == MM_ERROR_NONE)
		debug_log("volume_get_value %s %d",  g_volume_str[type], *value);

	return ret;
}

EXPORT_API
int mm_sound_util_volume_set_value_by_type(volume_type_t type, unsigned int value)
{
	int ret = MM_ERROR_NONE;
	int vconf_value = 0;

	vconf_value = value;
	debug_log("volume_set_value %s %d",  g_volume_str[type], value);

	/* Set volume value to VCONF */
	if ((ret = vconf_set_int(g_volume_vconf[type], vconf_value)) != 0) {
		debug_error ("vconf_set_int(%s) failed..ret[%d]\n", g_volume_vconf[type], ret);
		if (ret == -EPERM || ret == -EACCES)
			return MM_ERROR_SOUND_PERMISSION_DENIED;
		else
			return MM_ERROR_SOUND_INTERNAL;
	}
	return ret;
}

EXPORT_API
int mm_sound_util_get_earjack_type (int *type)
{
	int earjack_status = 0;

	if (type == NULL) {
		debug_error ("invalid parameter!!!");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Get actual vconf value */
	vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &earjack_status);
	debug_msg ("[%s] get status=[%d]\n", VCONFKEY_SYSMAN_EARJACK, earjack_status);

	*type = (earjack_status >= 0)? earjack_status : VCONFKEY_SYSMAN_EARJACK_REMOVED;

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_util_get_dock_type (int *type)
{
	int dock_status = 0;

	if (type == NULL) {
		debug_error ("invalid parameter!!!");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Get actual vconf value */
	vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &dock_status);
	debug_msg ("[%s] get dock status=[%d]\n", VCONFKEY_SYSMAN_CRADLE_STATUS, dock_status);

	*type = dock_status;

	return MM_ERROR_NONE;
}

EXPORT_API
bool mm_sound_util_is_recording (void)
{
	int capture_status = 0;
	bool result = false;

    /* FIXME */
	return result;
}

EXPORT_API
bool mm_sound_util_is_mute_policy (void)
{
	int setting_sound_status = true;

	/* If sound is mute mode, force ringtone/notification path to headset */
	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &setting_sound_status);
	debug_log ("[%s] setting_sound_status=%d\n", VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, setting_sound_status);

	return !setting_sound_status;
}
