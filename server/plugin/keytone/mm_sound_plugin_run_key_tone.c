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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <mm_error.h>
#include <mm_debug.h>
#include <mm_source.h>
#include <mm_sound.h>

#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>

#include <pulse/sample.h>

#include "../../include/mm_sound_plugin_run.h"
#include "../../include/mm_sound_plugin_codec.h"
#include "../../../include/mm_sound_utils.h"
#include "../../../include/mm_sound_common.h"
#include "../../../include/mm_sound_pa_client.h"

#define DEFAULT_TIMEOUT_MSEC_IN_USEC (600*1000)
#define ENV_KEYTONE_TIMEOUT "KEYTONE_TIMEOUT"

#define MAX_BUFFER_SIZE 1920
#define KEYTONE_PATH "/tmp/keytone"		/* Keytone pipe path */
#define KEYTONE_PATH_TMP "/tmp/keytone"		/* Keytone pipe path (it will be deprecated)*/
#define KEYTONE_GROUP	6526			/* Keytone group : assigned by security */
#define FILE_FULL_PATH 1024				/* File path lenth */
#define ROLE_NAME_LEN 64				/* Role name length */
#define VOLUME_GAIN_TYPE_LEN 64		/* Volume gain type length */
#define AUDIO_CHANNEL 1
#define AUDIO_SAMPLERATE 44100
#define DURATION_CRITERIA 11000          /* write once or not       */

#define SUPPORT_DBUS_KEYTONE
#ifdef SUPPORT_DBUS_KEYTONE
#include <gio/gio.h>

#include <vconf.h>

#define BUS_NAME       "org.tizen.system.deviced"
#define OBJECT_PATH    "/Org/Tizen/System/DeviceD/Key"
#define INTERFACE_NAME "org.tizen.system.deviced.Key"
#define SIGNAL_NAME    "ChangeHardkey"

#define DBUS_HW_KEYTONE "/usr/share/sounds/sound-server/Tizen_HW_Touch.ogg"

#endif /* SUPPORT_DBUS_KEYTONE */

typedef struct
{
	pthread_mutex_t sw_lock;
	pthread_cond_t sw_cond;
	int handle;

	int period;
	int volume_config;
	int state;
	void *src;
} keytone_info_t;

typedef struct
{
	char filename[FILE_FULL_PATH];
	int volume_config;
} ipc_type;

typedef struct
{
	mmsound_codec_info_t *info;
	MMSourceType *source;
} buf_param_t;

static int (*g_thread_pool_func)(void*, void (*)(void*)) = NULL;

static int _MMSoundKeytoneInit(void);
static int _MMSoundKeytoneFini(void);
static keytone_info_t g_keytone;
static int stop_flag = 0;

#ifdef SUPPORT_DBUS_KEYTONE
#define AUDIO_VOLUME_CONFIG_TYPE(vol) (vol & 0x00FF)
#define AUDIO_VOLUME_CONFIG_GAIN(vol) (vol & 0xFF00)
typedef struct ipc_data {
	char filename[FILE_FULL_PATH];
	char role[ROLE_NAME_LEN];
	char volume_gain_type[VOLUME_GAIN_TYPE_LEN];
}ipc_t;

GDBusConnection *conn;
guint sig_id;

static const char* _convert_volume_type_to_role(int volume_type)
{
	debug_warning ("volume_type(%d)", volume_type);
	switch(volume_type) {
	case VOLUME_TYPE_SYSTEM:
		return "system";
	case VOLUME_TYPE_NOTIFICATION:
		return "notification";
	case VOLUME_TYPE_ALARM:
		return "alarm";
	case VOLUME_TYPE_RINGTONE:
		return "ringtone";
	case VOLUME_TYPE_CALL:
		return "call";
	case VOLUME_TYPE_VOIP:
		return "voip";
	case VOLUME_TYPE_VOICE:
		return "voice";
	default:
		return NULL;
	}
}

static const char* _convert_volume_gain_type_to_string(int volume_gain_type)
{
	debug_warning ("volume_gain_type(0x%x)", volume_gain_type);
	switch(volume_gain_type) {
	case VOLUME_GAIN_DEFAULT:
		return NULL;
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
		return NULL;
	}
}

static int _play_keytone(const char *filename, int volume_config)
{
	int err = -1;
	int fd = -1;
	ipc_t data = {{0,},{0,},{0,}};
	int ret = MM_ERROR_NONE;
	char *role = NULL;
	char *vol_gain_type = NULL;

	debug_msg("filepath=[%s], volume_config=[0x%x]\n", filename, volume_config);

	if (!filename)
		return MM_ERROR_SOUND_INVALID_FILE;

	/* Open PIPE */
	if ((fd = open(KEYTONE_PATH, O_WRONLY | O_NONBLOCK)) != -1) {
		/* convert volume type to role */
		role = _convert_volume_type_to_role(AUDIO_VOLUME_CONFIG_TYPE(volume_config));
		if (role) {
			MMSOUND_STRNCPY(data.role, role, ROLE_NAME_LEN);
			vol_gain_type = _convert_volume_gain_type_to_string(AUDIO_VOLUME_CONFIG_GAIN(volume_config));
			if (vol_gain_type)
				MMSOUND_STRNCPY(data.volume_gain_type, vol_gain_type, VOLUME_GAIN_TYPE_LEN);
		}
		MMSOUND_STRNCPY(data.filename, filename, FILE_FULL_PATH);

		/* Write to PIPE */
		if ((err = write(fd, &data, sizeof(ipc_t))) < 0) {
			debug_error("Fail to write data: %s\n", strerror(errno));
			ret = MM_ERROR_SOUND_INTERNAL;
		}
	} else {
		debug_error("Fail to open pipe\n");
		ret = MM_ERROR_SOUND_FILE_NOT_FOUND;
	}

	/* Close PIPE */
	if (fd != -1)
		close(fd);

	return ret;
}

static bool _is_mute_sound ()
{
	int setting_sound_status = true;
	int setting_touch_sound = true;

	/* 1. Check if recording is in progress */
	if (mm_sound_util_is_recording()) {
		debug_log ("During Recording....MUTE!!!");
		return true;
	}

	/* 2. Check both SoundStatus & TouchSound vconf key for mute case */
	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &setting_sound_status);
	vconf_get_bool(VCONFKEY_SETAPPL_TOUCH_SOUNDS_BOOL, &setting_touch_sound);

	return !(setting_sound_status & setting_touch_sound);
}

static void _on_changed_receive(GDBusConnection *conn,
							   const gchar *sender_name,
							   const gchar *object_path,
							   const gchar *interface_name,
							   const gchar *signal_name,
							   GVariant *parameters,
							   gpointer user_data)
{
	debug_msg ("sender : %s, object : %s, interface : %s, signal : %s",
			sender_name, object_path, interface_name, signal_name);

	if (_is_mute_sound ()) {
		debug_log ("Skip playing keytone due to mute sound mode");
	} else {
		_play_keytone (DBUS_HW_KEYTONE, VOLUME_TYPE_SYSTEM | VOLUME_GAIN_TOUCH);
	}
}

static int _init_dbus_keytone ()
{
	GError *err = NULL;

	debug_fenter ();

	g_type_init();

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!conn && err) {
		debug_error ("g_bus_get_sync() error (%s) ", err->message);
		g_error_free (err);
		goto error;
	}

	sig_id = g_dbus_connection_signal_subscribe(conn,
			NULL, INTERFACE_NAME, SIGNAL_NAME, OBJECT_PATH, NULL, 0,
			_on_changed_receive, NULL, NULL);
	if (sig_id == 0) {
		debug_error ("g_dbus_connection_signal_subscribe() error (%d)", sig_id);
		goto sig_error;
	}

	debug_fleave ();
	return 0;

sig_error:
	g_dbus_connection_signal_unsubscribe(conn, sig_id);
	g_object_unref(conn);

error:
	return -1;
}

static void _deinit_dbus_keytone ()
{
	debug_fenter ();
	g_dbus_connection_signal_unsubscribe(conn, sig_id);
	g_object_unref(conn);
	debug_fleave ();
}
#endif /* SUPPORT_DBUS_KEYTONE */

static
int MMSoundPlugRunKeytoneControlRun(void)
{
	int pre_mask;
	int ret = MM_ERROR_NONE;
	int fd = -1;
	ipc_type data;
	int size = 0;
	mmsound_codec_info_t info = {0,};
	MMSourceType source = {0,};

	buf_param_t buf_param = {NULL, NULL};

	debug_enter("\n");

	/* INIT IPC */
	pre_mask = umask(0);
	if (mknod(KEYTONE_PATH_TMP,S_IFIFO|0660,0)<0) {
		debug_warning ("mknod failed. errno=[%d][%s]\n", errno, strerror(errno));
	}
	umask(pre_mask);

	fd = open(KEYTONE_PATH_TMP, O_RDWR);
	if (fd == -1) {
		debug_warning("Check ipc node %s\n", KEYTONE_PATH);
		return MM_ERROR_SOUND_INTERNAL;
	}

	/* change access mode so group can use keytone pipe */
	if (fchmod (fd, 0666) == -1) {
		debug_warning("Changing keytone access mode is failed. errno=[%d][%s]\n", errno, strerror(errno));
	}

	/* change group due to security request */
	if (fchown (fd, -1, KEYTONE_GROUP) == -1) {
		debug_warning("Changing keytone group is failed. errno=[%d][%s]\n", errno, strerror(errno));
	}

	/* Init Audio Handle & internal buffer */
	ret = _MMSoundKeytoneInit();	/* Create two thread and open device */
	if (ret != MM_ERROR_NONE) {
		debug_critical("Cannot create keytone\n");

	}
	/* While loop is always on */
	stop_flag = MMSOUND_TRUE;
	source.ptr = NULL;

	debug_msg("Start IPC with pipe\n");
	size = sizeof(ipc_type);

#ifdef SUPPORT_DBUS_KEYTONE
	_init_dbus_keytone();
#endif /* SUPPORT_DBUS_KEYTONE */

	while (stop_flag) {
		memset(&data, 0, sizeof(ipc_type));
#if defined(_DEBUG_VERBOS_)
		debug_log("Start to read from pipe\n");
#endif
		ret = read(fd, (void *)&data, size);
		if (ret == -1) {
			debug_error("Fail to read file\n");
			continue;
		}
#if defined(_DEBUG_VERBOS_)
		debug_log("Read returns\n");
#endif
	}

	if (fd > -1)
		close(fd);

	_MMSoundKeytoneFini();
	debug_leave("\n");

	return MM_ERROR_NONE;
}

static
int MMSoundPlugRunKeytoneControlStop(void)
{
	stop_flag = MMSOUND_FALSE; /* No impl. Don`t stop */

#ifdef SUPPORT_DBUS_KEYTONE
	_deinit_dbus_keytone();
#endif /* SUPPORT_DBUS_KEYTONE */

	return MM_ERROR_NONE;
}

static
int MMSoundPlugRunKeytoneSetThreadPool(int (*func)(void*, void (*)(void*)))
{
	debug_enter("(func : %p)\n", func);
	g_thread_pool_func = func;
	debug_leave("\n");
	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundPlugRunGetInterface(mmsound_run_interface_t *intf)
{
	debug_enter("\n");
	intf->run = MMSoundPlugRunKeytoneControlRun;
	intf->stop = MMSoundPlugRunKeytoneControlStop;
	intf->SetThreadPool = MMSoundPlugRunKeytoneSetThreadPool;
	debug_leave("\n");

	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundGetPluginType(void)
{
	debug_enter("\n");
	debug_leave("\n");
	return MM_SOUND_PLUGIN_TYPE_RUN;
}

static int _MMSoundKeytoneInit(void)
{
	debug_enter("\n");

	/* Set audio FIXED param */
	memset(&g_keytone, 0x00, sizeof(g_keytone));
	if (pthread_mutex_init(&(g_keytone.sw_lock), NULL)) {
		debug_error("pthread_mutex_init() failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	if (pthread_cond_init(&g_keytone.sw_cond,NULL)) {
		debug_error("pthread_cond_init() failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

static int _MMSoundKeytoneFini(void)
{
	g_keytone.handle = -1;

	if (pthread_mutex_destroy(&(g_keytone.sw_lock))) {
		debug_error("Fail to destroy mutex\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	debug_msg("destroy\n");

	if (pthread_cond_destroy(&g_keytone.sw_cond)) {
		debug_error("Fail to destroy cond\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return MM_ERROR_NONE;
}
