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
#include <glib.h>

#include <vconf.h>
#include <vconf-keys.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mm_sound_private.h"
#include "../include/mm_sound.h"
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_utils.h"

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
	VCONF_KEY_VOLUME_TYPE_SYSTEM,	/* VOLUME_TYPE_SYSTEM */
	VCONF_KEY_VOLUME_TYPE_NOTIFICATION,	/* VOLUME_TYPE_NOTIFICATION */
	VCONF_KEY_VOLUME_TYPE_ALARM,	/* VOLUME_TYPE_ALARM */
	VCONF_KEY_VOLUME_TYPE_RINGTONE,	/* VOLUME_TYPE_RINGTONE */
	VCONF_KEY_VOLUME_TYPE_MEDIA,	/* VOLUME_TYPE_MEDIA */
	VCONF_KEY_VOLUME_TYPE_CALL,	/* VOLUME_TYPE_CALL */
	VCONF_KEY_VOLUME_TYPE_VOIP,	/* VOLUME_TYPE_VOIP */
	VCONF_KEY_VOLUME_TYPE_VOICE,	/* VOLUME_TYPE_VOICE */
	VCONF_KEY_VOLUME_TYPE_ANDROID	/* VOLUME_TYPE_FIXED */
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

EXPORT_API int mm_sound_util_volume_get_value_by_type(volume_type_t type, unsigned int *value)
{
	int ret = MM_ERROR_NONE;
	int vconf_value = 0;

	/* Get volume value from VCONF */
	if (vconf_get_int(g_volume_vconf[type], &vconf_value)) {
		debug_error("vconf_get_int(%s) failed..\n", g_volume_vconf[type]);
		return MM_ERROR_SOUND_INTERNAL;
	}

	*value = vconf_value;
	if (ret == MM_ERROR_NONE)
		debug_log("volume_get_value %s %d", g_volume_str[type], *value);

	return ret;
}

EXPORT_API int mm_sound_util_volume_set_value_by_type(volume_type_t type, unsigned int value)
{
	int ret = MM_ERROR_NONE;
	int vconf_value = 0;

	vconf_value = value;
	debug_log("volume_set_value %s %d", g_volume_str[type], value);

	/* Set volume value to VCONF */
	if ((ret = vconf_set_int(g_volume_vconf[type], vconf_value)) != 0) {
		debug_error("vconf_set_int(%s) failed..ret[%d]\n", g_volume_vconf[type], ret);
		return MM_ERROR_SOUND_INTERNAL;
	}
	return ret;
}

EXPORT_API bool mm_sound_util_is_recording(void)
{
	/* FIXME : is this function needs anymore ??? */
	return false;
}

EXPORT_API bool mm_sound_util_is_process_alive(pid_t pid)
{
	gchar *tmp = NULL;
	int ret = -1;

	if (pid > 999999 || pid < 2)
		return false;

	if ((tmp = g_strdup_printf("/proc/%d", pid))) {
		ret = access(tmp, F_OK);
		g_free(tmp);
	}

	if (ret == -1) {
		if (errno == ENOENT) {
			debug_warning("/proc/%d not exist", pid);
			return false;
		} else {
			debug_error("/proc/%d access errno[%d]", pid, errno);

			/* FIXME: error occured but file exists */
			return true;
		}
	}

	return true;
}
