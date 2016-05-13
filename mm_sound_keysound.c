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
#include <fcntl.h>
#include <vconf.h>

#include <gio/gio.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <mm_sound.h>

#include "include/mm_sound_common.h"

#define PA_BUS_NAME                              "org.pulseaudio.Server"
#define PA_SOUND_PLAYER_OBJECT_PATH              "/org/pulseaudio/SoundPlayer"
#define PA_SOUND_PLAYER_INTERFACE                "org.pulseaudio.SoundPlayer"
#define PA_SOUND_PLAYER_METHOD_NAME_SIMPLE_PLAY  "SimplePlay"

#define KEYTONE_PATH "/tmp/keytone"		/* Keytone pipe path */
#define FILE_FULL_PATH 1024				/* File path length */
#define ROLE_NAME_LEN 64				/* Role name length */
#define VOLUME_GAIN_TYPE_LEN 64		/* Volume gain type length */

#define AUDIO_VOLUME_CONFIG_TYPE(vol) (vol & 0x00FF)
#define AUDIO_VOLUME_CONFIG_GAIN(vol) (vol & 0xFF00)

typedef struct ipc_data {
    char filename[FILE_FULL_PATH];
    char role[ROLE_NAME_LEN];
    char volume_gain_type[VOLUME_GAIN_TYPE_LEN];
}ipc_t;

typedef enum {
	IPC_TYPE_PIPE,
	IPC_TYPE_DBUS,
}ipc_type_t;

static int _mm_sound_play_keysound(const char *filename, int volume_config, ipc_type_t ipc_type);

static const char* convert_volume_type_to_role(int volume_type)
{
	debug_warning ("volume_type(%d)", volume_type);
	switch(volume_type) {
	case VOLUME_TYPE_MEDIA:
		return "media";
	case VOLUME_TYPE_SYSTEM:
		return "system";
	case VOLUME_TYPE_NOTIFICATION:
		return "notification";
	case VOLUME_TYPE_ALARM:
		return "alarm";
	case VOLUME_TYPE_VOICE:
		return "voice";
	default:
		debug_warning ("not supported type(%d), we change it SYSTEM type forcibly" );
		return "system";
	}
}

static const char* convert_volume_gain_type_to_string(int volume_gain_type)
{
	debug_warning ("volume_gain_type(0x%x)", volume_gain_type);
	switch(volume_gain_type) {
	case VOLUME_GAIN_DEFAULT:
		return "";
	case VOLUME_GAIN_DIALER:
		return "dialer";
	case VOLUME_GAIN_TOUCH:
		return "touch";
	case VOLUME_GAIN_AF:
		return "af";
	case VOLUME_GAIN_SHUTTER1:
		return "shutter1";
	case VOLUME_GAIN_SHUTTER2:
		return "shutter2";
	case VOLUME_GAIN_CAMCORDING:
		return "camcording";
	case VOLUME_GAIN_MIDI:
		return "midi";
	case VOLUME_GAIN_BOOTING:
		return "booting";
	case VOLUME_GAIN_VIDEO:
		return "video";
	case VOLUME_GAIN_TTS:
		return "tts";
	default:
		return "";
	}
}

EXPORT_API
int mm_sound_play_keysound(const char *filename, int volume_config)
{
	return _mm_sound_play_keysound(filename, volume_config, IPC_TYPE_DBUS);
}

static int _mm_sound_play_keysound(const char *filename, int volume_config, ipc_type_t ipc_type)
{
	int ret = MM_ERROR_NONE;
	const char *role = NULL;
	const char *vol_gain_type = NULL;

	if (!filename)
		return MM_ERROR_SOUND_INVALID_FILE;

	/* convert volume type to role/volume gain */
	role = convert_volume_type_to_role(AUDIO_VOLUME_CONFIG_TYPE(volume_config));
	if (role) {
		vol_gain_type = convert_volume_gain_type_to_string(AUDIO_VOLUME_CONFIG_GAIN(volume_config));
	}

	if (ipc_type == IPC_TYPE_PIPE) {
		int res = 0;
		int fd = -1;
		int size = 0;
		ipc_t data = {{0,},{0,},{0,}};

		/* Check whether file exists */
		fd = open(filename, O_RDONLY);
		if (fd == -1) {
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("file open failed with [%s][%d]\n", str_error, errno);
			switch (errno) {
			case ENOENT:
				return MM_ERROR_SOUND_FILE_NOT_FOUND;
			default:
				return MM_ERROR_SOUND_INTERNAL;
			}
		}
		close(fd);
		fd = -1;

		/* Open PIPE */
		fd = open(KEYTONE_PATH, O_WRONLY | O_NONBLOCK);
		if (fd == -1) {
			debug_error("Fail to open pipe\n");
			return MM_ERROR_SOUND_FILE_NOT_FOUND;
		}

		/* convert volume type to role/volume gain */
		if (role) {
			MMSOUND_STRNCPY(data.role, role, ROLE_NAME_LEN);
		}
		if (vol_gain_type) {
			MMSOUND_STRNCPY(data.volume_gain_type, vol_gain_type, VOLUME_GAIN_TYPE_LEN);
		}

		MMSOUND_STRNCPY(data.filename, filename, FILE_FULL_PATH);

		debug_msg("filepath=[%s], role=[%s], volume_gain_type=[%s]\n", data.filename, data.role, data.volume_gain_type);
		size = sizeof(ipc_t);

		/* Write to PIPE */
		res = write(fd, &data, size);
		if (res < 0) {
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("Fail to write data: [%s][%d]\n", str_error, errno);
			ret = MM_ERROR_SOUND_INTERNAL;
		}
		/* Close PIPE */
		close(fd);

	} else if (ipc_type == IPC_TYPE_DBUS) {
		GVariant *result = NULL;
		GDBusConnection *conn = NULL;
		GError *err = NULL;
		int idx = 0;

		conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
		if (!conn && err) {
			debug_error("g_bus_get_sync() error (%s)", err->message);
			ret = MM_ERROR_SOUND_INTERNAL;
		} else {
			result = g_dbus_connection_call_sync (conn,
									PA_BUS_NAME,
									PA_SOUND_PLAYER_OBJECT_PATH,
									PA_SOUND_PLAYER_INTERFACE,
									PA_SOUND_PLAYER_METHOD_NAME_SIMPLE_PLAY,
									g_variant_new ("(sss)", filename, role, vol_gain_type),
									NULL,
									G_DBUS_CALL_FLAGS_NONE,
									2000,
									NULL,
									&err);
			if (!result && err) {
				debug_error("g_dbus_connection_call_sync() for SIMPLE_PLAY error (%s)", err->message);
				ret = MM_ERROR_SOUND_INTERNAL;
			} else {
				g_variant_get(result, "(i)", &idx);
				if (idx == -1) {
					debug_error("SIMPLE_PLAY failure, filename(%s)/role(%s)/gain(%s)/stream idx(%d)", filename, role, vol_gain_type, idx);
					ret = MM_ERROR_SOUND_INTERNAL;
				} else {
					debug_msg("SIMPLE_PLAY success, filename(%s)/role(%s)/gain(%s)/stream idx(%d)", filename, role, vol_gain_type, idx);
				}
				g_variant_unref(result);
			}
			g_object_unref(conn);
		}
		if (err) {
			g_error_free(err);
		}
	}

	return ret;
}
