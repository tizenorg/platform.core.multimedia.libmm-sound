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
#include <poll.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

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

#define FOCUS_HANDLE_MAX 512
#define FOCUS_HANDLE_INIT_VAL -1
#define CONFIG_ENABLE_RETCB

#define VOLUME_TYPE_LEN 64

struct sigaction FOCUS_int_old_action;
struct sigaction FOCUS_abrt_old_action;
struct sigaction FOCUS_segv_old_action;
struct sigaction FOCUS_term_old_action;
struct sigaction FOCUS_sys_old_action;
struct sigaction FOCUS_xcpu_old_action;

typedef struct {
	void *user_cb;
	void *user_data;
	int mask;
	guint subs_id;
} client_cb_data_t;

typedef struct {
	int focus_tid;
	int handle;
	int focus_fd;
	GSourceFuncs* g_src_funcs;
	GPollFD* g_poll_fd;
	GSource* focus_src;
	bool is_used;
	bool auto_reacquire;
	GMutex focus_lock;
	mm_sound_focus_changed_cb focus_callback;
	mm_sound_focus_changed_watch_cb watch_callback;
	void* user_data;

	bool is_for_session;	/* will be removed when the session concept is completely left out*/
} focus_sound_info_t;

typedef struct {
	int pid;
	int handle;
	int type;
	int state;
	char stream_type [MAX_STREAM_TYPE_LEN];
	char name [MM_SOUND_NAME_NUM];
} focus_cb_data_lib;

typedef struct {
	mm_sound_focus_session_interrupt_cb user_cb;
	void* user_data;
} focus_session_interrupt_info_t;

typedef gboolean (*focus_gLoopPollHandler_t)(gpointer d);

GThread *g_focus_thread;
GMainLoop *g_focus_loop;
focus_sound_info_t g_focus_sound_handle[FOCUS_HANDLE_MAX];
focus_session_interrupt_info_t g_focus_session_interrupt_info = {NULL, NULL};

/* global variables for device list */
//static GList *g_device_list = NULL;
static mm_sound_device_list_t g_device_list_t;
static pthread_mutex_t g_device_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_id_mutex = PTHREAD_MUTEX_INITIALIZER;

guint g_focus_signal_handle = 0;

void _focus_signal_handler(int signo)
{
	int ret = MM_ERROR_NONE;
	int exit_pid = 0;
	int index = 0;
	sigset_t old_mask, all_mask;

	debug_error("Got signal : signo(%d)", signo);

	/* signal block */

	sigfillset(&all_mask);
	sigprocmask(SIG_BLOCK, &all_mask, &old_mask);

	exit_pid = getpid();

	/* need implementation */
	//send exit pid to focus server and focus server will clear focus or watch if necessary.

	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == true && g_focus_sound_handle[index].focus_tid == exit_pid) {
			ret = mm_sound_client_dbus_emergent_exit_focus(exit_pid);
			break;
		}
	}

	if (ret == MM_ERROR_NONE)
		debug_msg("[Client] Success to emergnet_exit_focus\n");
	else
		debug_error("[Client] Error occurred : %d \n",ret);

	sigprocmask(SIG_SETMASK, &old_mask, NULL);
	/* signal unblock */

	switch (signo) {
	case SIGINT:
		sigaction(SIGINT, &FOCUS_int_old_action, NULL);
		raise( signo);
		break;
	case SIGABRT:
		sigaction(SIGABRT, &FOCUS_abrt_old_action, NULL);
		raise( signo);
		break;
	case SIGSEGV:
		sigaction(SIGSEGV, &FOCUS_segv_old_action, NULL);
		raise( signo);
		break;
	case SIGTERM:
		sigaction(SIGTERM, &FOCUS_term_old_action, NULL);
		raise( signo);
		break;
	case SIGSYS:
		sigaction(SIGSYS, &FOCUS_sys_old_action, NULL);
		raise( signo);
		break;
	case SIGXCPU:
		sigaction(SIGXCPU, &FOCUS_xcpu_old_action, NULL);
		raise( signo);
		break;
	default:
		break;
	}

	debug_error("signal handling end");
}

int mm_sound_client_initialize(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	mm_sound_client_dbus_initialize();

#ifdef USE_FOCUS

	struct sigaction FOCUS_action;
	FOCUS_action.sa_handler = _focus_signal_handler;
	FOCUS_action.sa_flags = SA_NOCLDSTOP;

	sigemptyset(&FOCUS_action.sa_mask);

	sigaction(SIGINT, &FOCUS_action, &FOCUS_int_old_action);
	sigaction(SIGABRT, &FOCUS_action, &FOCUS_abrt_old_action);
	sigaction(SIGSEGV, &FOCUS_action, &FOCUS_segv_old_action);
	sigaction(SIGTERM, &FOCUS_action, &FOCUS_term_old_action);
	sigaction(SIGSYS, &FOCUS_action, &FOCUS_sys_old_action);
	sigaction(SIGXCPU, &FOCUS_action, &FOCUS_xcpu_old_action);

#endif


	debug_fleave();
	return ret;
}

int mm_sound_client_finalize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_finalize();


#ifdef USE_FOCUS

	int index = 0;
	int exit_pid = 0;

	exit_pid = getpid();
	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == true && g_focus_sound_handle[index].focus_tid == exit_pid) {
			mm_sound_client_dbus_emergent_exit_focus(exit_pid);
		}
	}

	if (g_focus_thread) {
		g_main_loop_quit(g_focus_loop);
		g_thread_join(g_focus_thread);
		debug_log("after thread join");
		g_main_loop_unref(g_focus_loop);
		g_focus_thread = NULL;
	}

	/* is it necessary? */
	sigaction(SIGINT, &FOCUS_int_old_action, NULL);
	sigaction(SIGABRT, &FOCUS_abrt_old_action, NULL);
	sigaction(SIGSEGV, &FOCUS_segv_old_action, NULL);
	sigaction(SIGTERM, &FOCUS_term_old_action, NULL);
	sigaction(SIGSYS, &FOCUS_sys_old_action, NULL);
	sigaction(SIGXCPU, &FOCUS_xcpu_old_action, NULL);

#endif

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

	if (enable_session) {
		if (MM_ERROR_NONE != _mm_session_util_read_information(-1, &session_type, &session_options)) {
			debug_warning("[Client] Read Session Information failed. use default \"media\" type\n");
			session_type = MM_SESSION_TYPE_MEDIA;

			if(MM_ERROR_NONE != mm_session_init(session_type)) {
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

void _mm_sound_stop_callback_wrapper_func(int ended_handle, void *userdata)
{
	client_cb_data_t *cb_data = (client_cb_data_t*) userdata;

	debug_log("[Wrapper CB][Play Stop] ended_handle : %d", ended_handle);

	if (cb_data == NULL) {
		debug_warning("stop callback data null");
		return;
	}
	if (ended_handle == cb_data->mask) {
		debug_log("Interested playing handle end : %d", ended_handle);
		((mm_sound_stop_callback_func)(cb_data->user_cb))(cb_data->user_data, ended_handle);
		if (mm_sound_client_dbus_remove_play_sound_end_callback(cb_data->subs_id) != MM_ERROR_NONE)
			debug_error("mm_sound_client_dbus_remove_play_file_end_callback failed");
	} else {
		debug_log("Not interested playing handle : %d", ended_handle);
	}
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
	client_cb_data_t *cb_data = NULL;

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
		if(MM_ERROR_NONE != _mm_session_util_read_information(-1, &session_type, &session_options)) {
			debug_warning("[Client] Read MMSession Type failed. use default \"media\" type\n");
			session_type = MM_SESSION_TYPE_MEDIA;

			if(MM_ERROR_NONE != mm_session_init(session_type)) {
				debug_critical("[Client] MMSessionInit() failed\n");
				return MM_ERROR_POLICY_INTERNAL;
			}
		}
	}

	/* Send msg */
	if ((param->mem_ptr && param->mem_size)) {
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
		cb_data = g_malloc0(sizeof(client_cb_data_t));
		cb_data->user_cb = param->callback;
		cb_data->user_data = param->data;
		cb_data->mask = *handle;

		ret = mm_sound_client_dbus_add_play_sound_end_callback(_mm_sound_stop_callback_wrapper_func, cb_data, &cb_data->subs_id);
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
	client_cb_data_t *cb_data = NULL;

	ret = mm_sound_client_dbus_play_sound_with_stream_info(param->filename, param->loop, param->volume,
					 param->priority, getpid(), param->handle_route, handle, stream_type, stream_id);
	if (ret != MM_ERROR_NONE) {
		debug_error("Play Sound Failed");
		goto failed;
	}
	if (param->callback) {
		cb_data = g_malloc0(sizeof(client_cb_data_t));
		cb_data->user_cb = param->callback;
		cb_data->user_data = param->data;
		cb_data->mask = *handle;

		ret = mm_sound_client_dbus_add_play_sound_end_callback(_mm_sound_stop_callback_wrapper_func, cb_data, &cb_data->subs_id);
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

void _mm_sound_device_connected_callback_wrapper_func(int device_id, const char *device_type, int io_direction, int state, const char *name, gboolean is_connected, void *userdata)
{
	mm_sound_device_t device_h;
	client_cb_data_t *cb_data = (client_cb_data_t*) userdata;

	debug_log("[Wrapper CB][Device Connnected] device_id : %d, device_type : %s, direction : %d, state : %d, name : %s, is_connected : %d",
			  device_id, device_type, io_direction, state, name, is_connected);

	if (cb_data == NULL) {
		debug_warning("device connected changed callback data null");
		return;
	}

	device_h.id = device_id;
	device_h.io_direction = io_direction;
	device_h.state = state;
	MMSOUND_STRNCPY(device_h.name, name, MAX_DEVICE_NAME_NUM);
	MMSOUND_STRNCPY(device_h.type, device_type, MAX_DEVICE_TYPE_STR_LEN);

	((mm_sound_device_connected_cb)(cb_data->user_cb))(&device_h, is_connected, cb_data->user_data);
}

int mm_sound_client_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;
	client_cb_data_t *cb_data = NULL;

	debug_fenter();

	cb_data = g_malloc0(sizeof(client_cb_data_t));
	cb_data->user_cb = func;
	cb_data->user_data = user_data;

	ret = mm_sound_client_dbus_add_device_connected_callback(device_flags, _mm_sound_device_connected_callback_wrapper_func, cb_data, subs_id);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_device_connected_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_device_connected_callback(subs_id);

	debug_fleave();
	return ret;
}

static void _mm_sound_device_info_changed_callback_wrapper_func(int device_id, const char *device_type, int io_direction, int state, const char *name, int changed_device_info_type, void *userdata)
{
	mm_sound_device_t device_h;
	client_cb_data_t *cb_data = (client_cb_data_t*) userdata;

	debug_log("[Wrapper CB][Device Info Changed] device_id : %d, device_type : %s, direction : %d, state : %d, name : %s, changed_info_type : %d",
			  device_id, device_type, io_direction, state, name, changed_device_info_type);

	if (cb_data == NULL) {
		debug_warning("device info changed callback data null");
		return;
	}

	device_h.id = device_id;
	device_h.io_direction = io_direction;
	device_h.state = state;
	MMSOUND_STRNCPY(device_h.name, name, MAX_DEVICE_NAME_NUM);
	MMSOUND_STRNCPY(device_h.type, device_type, MAX_DEVICE_TYPE_STR_LEN);

	((mm_sound_device_info_changed_cb)(cb_data->user_cb))(&device_h, changed_device_info_type, cb_data->user_data);
}

int mm_sound_client_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void *userdata, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;
	client_cb_data_t *cb_data = (client_cb_data_t*) userdata;

	debug_fenter();

	cb_data = g_malloc0(sizeof(client_cb_data_t));
	cb_data->user_cb = func;
	cb_data->user_data = userdata;

	ret = mm_sound_client_dbus_add_device_info_changed_callback(device_flags, _mm_sound_device_info_changed_callback_wrapper_func, cb_data, subs_id);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_device_info_changed_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret =  mm_sound_client_dbus_remove_device_info_changed_callback(subs_id);

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

int __convert_volume_type_to_str(int volume_type, char **volume_type_str)
{
	int ret = MM_ERROR_NONE;

	if (!volume_type_str) {
		return MM_ERROR_COMMON_INVALID_ARGUMENT;
	}

	switch (volume_type) {
	case VOLUME_TYPE_SYSTEM:
		*volume_type_str = "system";
		break;
	case VOLUME_TYPE_NOTIFICATION:
		*volume_type_str = "notification";
		break;
	case VOLUME_TYPE_ALARM:
		*volume_type_str = "alarm";
		break;
	case VOLUME_TYPE_RINGTONE:
		*volume_type_str = "ringtone";
		break;
	case VOLUME_TYPE_MEDIA:
		*volume_type_str = "media";
		break;
	case VOLUME_TYPE_CALL:
		*volume_type_str = "call";
		break;
	case VOLUME_TYPE_VOIP:
		*volume_type_str = "voip";
		break;
	case VOLUME_TYPE_VOICE:
		*volume_type_str = "voice";
		break;
	}
	if (!strncmp(*volume_type_str,"", VOLUME_TYPE_LEN)) {
		debug_error("could not find the volume_type[%d] in this switch case statement", volume_type);
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		debug_log("volume_type[%s]", *volume_type_str);
	}
	return ret;
}

static int __convert_volume_type_to_int(const char *volume_type_str, volume_type_t *volume_type)
{
	int ret = MM_ERROR_NONE;

	if (!volume_type || !volume_type_str) {
		return MM_ERROR_COMMON_INVALID_ARGUMENT;
	}

	if (!strncmp(volume_type_str, "system", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_SYSTEM;
	} else if (!strncmp(volume_type_str, "notification", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_NOTIFICATION;
	} else if (!strncmp(volume_type_str, "alarm", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_ALARM;
	} else if (!strncmp(volume_type_str, "ringtone", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_RINGTONE;
	} else if (!strncmp(volume_type_str, "media", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_MEDIA;
	} else if (!strncmp(volume_type_str, "call", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_CALL;
	} else if (!strncmp(volume_type_str, "voip", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_VOIP;
	} else if (!strncmp(volume_type_str, "voice", VOLUME_TYPE_LEN)) {
		*volume_type = VOLUME_TYPE_VOICE;
	} else {
		debug_log("Invalid volume type : [%s]", volume_type_str);
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	return ret;
}

int mm_sound_client_set_volume_by_type(const int volume_type, const unsigned int volume_level)
{
	int ret = MM_ERROR_NONE;
	char *type_str = NULL;
	debug_fenter();

	if ((ret = __convert_volume_type_to_str(volume_type, &type_str)) != MM_ERROR_NONE) {
		debug_error("volume type convert failed");
		goto failed;
	}

	ret = mm_sound_client_dbus_set_volume_by_type(type_str, volume_level);

failed:
	debug_fleave();
	return ret;
}

static void _mm_sound_volume_changed_callback_wrapper_func(const char *direction, const char *volume_type_str, int volume_level, void *userdata)
{
	volume_type_t volume_type = 0;
	client_cb_data_t *cb_data = (client_cb_data_t*) userdata;

	debug_log("[Wrapper CB][Volume Changed] direction : %s, volume_type : %s, volume_level : %d", direction, volume_type_str, volume_level);

	if (cb_data == NULL) {
		debug_warning("volume changed callback data null");
		return;
	}

	if (__convert_volume_type_to_int(volume_type_str, &volume_type) != MM_ERROR_NONE) {
		debug_error("volume type convert failed");
		return;
	}
	debug_log("Call volume changed user cb, direction : %s, vol_type : %s(%d), level : %u", direction, volume_type_str, volume_type, volume_level);
	((mm_sound_volume_changed_cb)(cb_data->user_cb))(volume_type, volume_level, cb_data->user_data);
}

int mm_sound_client_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;
	client_cb_data_t *cb_data = NULL;

	debug_fenter();

	cb_data = g_malloc0(sizeof(client_cb_data_t));
	cb_data->user_cb = func;
	cb_data->user_data = user_data;

	ret = mm_sound_client_dbus_add_volume_changed_callback(_mm_sound_volume_changed_callback_wrapper_func, cb_data, subs_id);

	debug_fleave();

	return ret;
}

int mm_sound_client_remove_volume_changed_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_volume_changed_callback(subs_id);

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
int mm_sound_client_set_session_interrupt_callback(mm_sound_focus_session_interrupt_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (!callback)
		return MM_ERROR_INVALID_ARGUMENT;

	g_focus_session_interrupt_info.user_cb = callback;
	g_focus_session_interrupt_info.user_data = user_data;

	debug_fleave();
	return ret;
}

int mm_sound_client_unset_session_interrupt_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (!g_focus_session_interrupt_info.user_cb) {
		debug_error("no callback to unset");
		return MM_ERROR_SOUND_INTERNAL;
	}

	g_focus_session_interrupt_info.user_cb = NULL;
	g_focus_session_interrupt_info.user_data = NULL;

	debug_fleave();
	return ret;
}

static gpointer _focus_thread_func(gpointer data)
{
	debug_log(">>> thread func..ID of this thread(%u)\n", (unsigned int)pthread_self());
	g_main_loop_run(g_focus_loop);
	debug_log("<<< quit thread func..\n");
	return NULL;
}

static gboolean _focus_fd_check(GSource * source)
{
	GSList *fd_list;
	GPollFD *temp;

	if (!source) {
		debug_error("GSource is null");
		return FALSE;
	}
	fd_list = source->poll_fds;
	if (!fd_list) {
		debug_error("fd_list is null");
		return FALSE;
	}
	do {
		temp = (GPollFD*)fd_list->data;
		if (!temp) {
			debug_error("fd_list->data is null");
			return FALSE;
		}
		if (temp->revents & (POLLIN | POLLPRI)) {
			return TRUE;
		}
		fd_list = fd_list->next;
	} while (fd_list);

	return FALSE; /* there is no change in any fd state */
}

static gboolean _focus_fd_prepare(GSource *source, gint *timeout)
{
	return FALSE;
}

static gboolean _focus_fd_dispatch(GSource *source,	GSourceFunc callback, gpointer user_data)
{
	callback(user_data);
	return TRUE;
}


static int _focus_find_index_by_handle(int handle)
{
	int i = 0;
	for(i = 0; i< FOCUS_HANDLE_MAX; i++) {
		if (handle == g_focus_sound_handle[i].handle) {
			//debug_msg("found index(%d) for handle(%d)", i, handle);
			if (handle == FOCUS_HANDLE_INIT_VAL) {
				return -1;
			}
			return i;
		}
	}
	return -1;
}

static gboolean _focus_callback_handler(gpointer d)
{
	GPollFD *data = (GPollFD*)d;
	int count;
	int tid = 0;
	int focus_index = 0;
	focus_cb_data_lib cb_data;
	debug_log(">>> focus_callback_handler()..ID of this thread(%u)\n", (unsigned int)pthread_self());

	memset(&cb_data, 0, sizeof(focus_cb_data_lib));

	if (!data) {
		debug_error("GPollFd is null");
		return FALSE;
	}
	if (data->revents & (POLLIN | POLLPRI)) {
		int changed_state = -1;

		count = read(data->fd, &cb_data, sizeof(cb_data));
		if (count < 0){
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("GpollFD read fail, errno=%d(%s)",errno, str_error);
			return FALSE;
		}
		changed_state = cb_data.state;
		focus_index = _focus_find_index_by_handle(cb_data.handle);
		if (focus_index == -1) {
			debug_error("Could not find index");
			return FALSE;
		}

		g_mutex_lock(&g_focus_sound_handle[focus_index].focus_lock);

		tid = g_focus_sound_handle[focus_index].focus_tid;

		if (changed_state != -1) {
			debug_error("Got and start CB : TID(%d), handle(%d), type(%d), state(%d,(DEACTIVATED(0)/ACTIVATED(1)), trigger(%s)", tid, cb_data.handle, cb_data.type, cb_data.state, cb_data.stream_type);
			if (g_focus_sound_handle[focus_index].focus_callback== NULL) {
					debug_error("callback is null..");
					g_mutex_unlock(&g_focus_sound_handle[focus_index].focus_lock);
					return FALSE;
			}
			debug_error("[CALLBACK(%p) START]",g_focus_sound_handle[focus_index].focus_callback);
			(g_focus_sound_handle[focus_index].focus_callback)(cb_data.handle, cb_data.type, cb_data.state, cb_data.stream_type, cb_data.name, g_focus_sound_handle[focus_index].user_data);
			debug_error("[CALLBACK END]");
			if (g_focus_session_interrupt_info.user_cb) {
				debug_error("sending session interrupt callback(%p)", g_focus_session_interrupt_info.user_cb);
				(g_focus_session_interrupt_info.user_cb)(cb_data.state, cb_data.stream_type, false, g_focus_session_interrupt_info.user_data);
			}
		}
#ifdef CONFIG_ENABLE_RETCB

				int rett = 0;
				int tmpfd = -1;
				unsigned int buf = 0;
				char *filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[focus_index].focus_tid, cb_data.handle);
				tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
				if (tmpfd < 0) {
					char str_error[256];
					strerror_r(errno, str_error, sizeof(str_error));
					debug_error("[RETCB][Failed(May Server Close First)]tid(%d) fd(%d) %s errno=%d(%s)\n", tid, tmpfd, filename2, errno, str_error);
					g_free(filename2);
					g_mutex_unlock(&g_focus_sound_handle[focus_index].focus_lock);
					return FALSE;
				}
				buf = (unsigned int)((0x0000ffff & cb_data.handle) |(g_focus_sound_handle[focus_index].auto_reacquire << 16));
				rett = write(tmpfd, &buf, sizeof(buf));
				close(tmpfd);
				g_free(filename2);
				debug_msg("[RETCB] tid(%d) finishing CB (write=%d)\n", tid, rett);
#endif
	}

	g_mutex_unlock(&g_focus_sound_handle[focus_index].focus_lock);

	return TRUE;
}

static gboolean _focus_watch_callback_handler( gpointer d)
{
	GPollFD *data = (GPollFD*)d;
	int count;
	int tid = 0;
	int focus_index = 0;
	focus_cb_data_lib cb_data;

	debug_fenter();

	memset(&cb_data, 0, sizeof(focus_cb_data_lib));

	if (!data) {
		debug_error("GPollFd is null");
		return FALSE;
	}
	if (data->revents & (POLLIN | POLLPRI)) {
		count = read(data->fd, &cb_data, sizeof(cb_data));
		if (count < 0){
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("GpollFD read fail, errno=%d(%s)",errno, str_error);
			return FALSE;
		}

		focus_index = _focus_find_index_by_handle(cb_data.handle);
		if (focus_index == -1) {
			debug_error("Could not find index");
			return FALSE;
		}

		debug_error("lock focus_lock = %p", &g_focus_sound_handle[focus_index].focus_lock);
		g_mutex_lock(&g_focus_sound_handle[focus_index].focus_lock);

		tid = g_focus_sound_handle[focus_index].focus_tid;

		debug_error("Got and start CB : TID(%d), handle(%d), type(%d), state(%d,(DEACTIVATED(0)/ACTIVATED(1)), trigger(%s)", tid, cb_data.handle,  cb_data.type, cb_data.state, cb_data.stream_type);

		if (g_focus_sound_handle[focus_index].watch_callback == NULL) {
			debug_msg("callback is null..");
		} else {
			debug_msg("[CALLBACK(%p) START]",g_focus_sound_handle[focus_index].watch_callback);
			(g_focus_sound_handle[focus_index].watch_callback)(cb_data.handle, cb_data.type, cb_data.state, cb_data.stream_type, cb_data.name, g_focus_sound_handle[focus_index].user_data);
			debug_msg("[CALLBACK END]");
			if (g_focus_session_interrupt_info.user_cb) {
				debug_error("sending session interrupt callback(%p)", g_focus_session_interrupt_info.user_cb);
				(g_focus_session_interrupt_info.user_cb)(cb_data.state, cb_data.stream_type, true, g_focus_session_interrupt_info.user_data);
			}
		}

#ifdef CONFIG_ENABLE_RETCB

			int rett = 0;
			int tmpfd = -1;
			int buf = -1;
			char *filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[focus_index].focus_tid);
			tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
			if (tmpfd < 0) {
				char str_error[256];
				strerror_r(errno, str_error, sizeof(str_error));
				debug_error("[RETCB][Failed(May Server Close First)]tid(%d) fd(%d) %s errno=%d(%s)\n", tid, tmpfd, filename2, errno, str_error);
				g_free(filename2);
				g_mutex_unlock(&g_focus_sound_handle[focus_index].focus_lock);
				return FALSE;
			}
			buf = cb_data.handle;
			rett = write(tmpfd, &buf, sizeof(buf));
			close(tmpfd);
			g_free(filename2);
			debug_msg("[RETCB] tid(%d) finishing CB (write=%d)\n", tid, rett);

#endif

	}

	debug_error("unlock focus_lock = %p", &g_focus_sound_handle[focus_index].focus_lock);
	g_mutex_unlock(&g_focus_sound_handle[focus_index].focus_lock);

	debug_fleave();


	return TRUE;
}

static void _focus_open_callback(int index, bool is_for_watching)
{
	mode_t pre_mask;
	char *filename;

	debug_fenter();

	if (is_for_watching) {
		filename = g_strdup_printf("/tmp/FOCUS.%d.wch", g_focus_sound_handle[index].focus_tid);
	} else {
		filename = g_strdup_printf("/tmp/FOCUS.%d.%d", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}
	pre_mask = umask(0);
	if (mknod(filename, S_IFIFO|0666, 0)) {
		debug_error("mknod() failure, errno(%d)", errno);
	}
	umask(pre_mask);
	g_focus_sound_handle[index].focus_fd = open( filename, O_RDWR|O_NONBLOCK);
	if (g_focus_sound_handle[index].focus_fd == -1) {
		debug_error("Open fail : index(%d), file open error(%d)", index, errno);
	} else {
		debug_log("Open sucess : index(%d), filename(%s), fd(%d)", index, filename, g_focus_sound_handle[index].focus_fd);
	}
	g_free(filename);
	filename = NULL;

#ifdef CONFIG_ENABLE_RETCB
	char *filename2;

	if (is_for_watching) {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[index].focus_tid);
	} else {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}
	pre_mask = umask(0);
	if (mknod(filename2, S_IFIFO | 0666, 0)) {
		debug_error("mknod() failure, errno(%d)", errno);
	}
	umask(pre_mask);
	g_free(filename2);
	filename2 = NULL;
#endif
	debug_fleave();

}

void _focus_close_callback(int index, bool is_for_watching)
{
	debug_fenter();

	if (g_focus_sound_handle[index].focus_fd < 0) {
		debug_error("Close fail : fd error.");
	} else {
		char *filename;
		if (is_for_watching) {
			filename = g_strdup_printf("/tmp/FOCUS.%d.wch", g_focus_sound_handle[index].focus_tid);
		} else {
			filename = g_strdup_printf("/tmp/FOCUS.%d.%d", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
		}
		close(g_focus_sound_handle[index].focus_fd);
		if (remove(filename)) {
			debug_error("remove() failure, filename(%s), errno(%d)", filename, errno);
		}
		debug_log("Close Sucess : index(%d), filename(%s)", index, filename);
		g_free(filename);
		filename = NULL;
	}

#ifdef CONFIG_ENABLE_RETCB
	char *filename2;
	int written;

	if (is_for_watching) {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[index].focus_tid);
	} else {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}

	/* Defensive code - wait until callback timeout although callback is removed */
	int buf = MM_ERROR_NONE; //no need to specify cb result to server, just notice if the client got the callback properly or not
	int tmpfd = -1;

	tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
	if (tmpfd < 0) {
		char str_error[256];
		strerror_r(errno, str_error, sizeof(str_error));
		debug_warning("could not open file(%s) (may server close it first), tid(%d) fd(%d) %s errno=%d(%s)",
			filename2, g_focus_sound_handle[index].focus_tid, tmpfd, filename2, errno, str_error);
	} else {
		debug_msg("write MM_ERROR_NONE(tid:%d) for waiting server", g_focus_sound_handle[index].focus_tid);
		written = write(tmpfd, &buf, sizeof(buf));
		close(tmpfd);
	}

	if (remove(filename2)) {
		debug_error("remove() failure, filename(%s), errno(%d)", filename2, errno);
	}
	g_free(filename2);
	filename2 = NULL;
#endif

}

static bool _focus_add_sound_callback(int index, int fd, gushort events, focus_gLoopPollHandler_t p_gloop_poll_handler )
{
	GSource* g_src = NULL;
	GSourceFuncs *g_src_funcs = NULL;		/* handler function */
	guint gsource_handle;
	GPollFD *g_poll_fd = NULL;			/* file descriptor */

	debug_fenter();

	g_mutex_init(&g_focus_sound_handle[index].focus_lock);

	/* 1. make GSource Object */
	g_src_funcs = (GSourceFuncs *)g_malloc(sizeof(GSourceFuncs));
	if (!g_src_funcs) {
		debug_error("g_malloc failed on g_src_funcs");
		return false;
	}

	g_src_funcs->prepare = _focus_fd_prepare;
	g_src_funcs->check = _focus_fd_check;
	g_src_funcs->dispatch = _focus_fd_dispatch;
	g_src_funcs->finalize = NULL;
	g_src = g_source_new(g_src_funcs, sizeof(GSource));
	if (!g_src) {
		debug_error("g_malloc failed on m_readfd");
		return false;
	}
	g_focus_sound_handle[index].focus_src = g_src;
	g_focus_sound_handle[index].g_src_funcs = g_src_funcs;

	/* 2. add file description which used in g_loop() */
	g_poll_fd = (GPollFD *)g_malloc(sizeof(GPollFD));
	if (!g_poll_fd) {
		debug_error("g_malloc failed on g_poll_fd");
		return false;
	}
	g_poll_fd->fd = fd;
	g_poll_fd->events = events;
	g_focus_sound_handle[index].g_poll_fd = g_poll_fd;

	/* 3. combine g_source object and file descriptor */
	g_source_add_poll(g_src, g_poll_fd);
	gsource_handle = g_source_attach(g_src, g_main_loop_get_context(g_focus_loop));
	if (!gsource_handle) {
		debug_error(" Failed to attach the source to context");
		return false;
	}
	//g_source_unref(g_src);

	/* 4. set callback */
	g_source_set_callback(g_src, p_gloop_poll_handler,(gpointer)g_poll_fd, NULL);

	debug_log(" g_malloc:g_src_funcs(%#X),g_poll_fd(%#X)  g_source_add_poll:g_src_id(%d)  g_source_set_callback:errno(%d)",
				g_src_funcs, g_poll_fd, gsource_handle, errno);

	debug_fleave();
	return true;


}

static bool _focus_remove_sound_callback(int index, gushort events)
{
	bool ret = true;

	debug_fenter();

	g_mutex_clear(&g_focus_sound_handle[index].focus_lock);

	GSourceFuncs *g_src_funcs = g_focus_sound_handle[index].g_src_funcs;
	GPollFD *g_poll_fd = g_focus_sound_handle[index].g_poll_fd;	/* store file descriptor */
	if (!g_poll_fd) {
		debug_error("g_poll_fd is null..");
		ret = false;
		goto init_handle;
	}
	g_poll_fd->fd = g_focus_sound_handle[index].focus_fd;
	g_poll_fd->events = events;

	if (!g_focus_sound_handle[index].focus_src) {
		debug_error("FOCUS_sound_handle[%d].focus_src is null..", index);
		goto init_handle;
	}
	debug_log(" g_source_remove_poll : fd(%d), event(%x), errno(%d)", g_poll_fd->fd, g_poll_fd->events, errno);
	g_source_remove_poll(g_focus_sound_handle[index].focus_src, g_poll_fd);

init_handle:

	if (g_focus_sound_handle[index].focus_src) {
		g_source_destroy(g_focus_sound_handle[index].focus_src);
		if (!g_source_is_destroyed (g_focus_sound_handle[index].focus_src)) {
			debug_warning(" failed to g_source_destroy(), focus_src(0x%p)", g_focus_sound_handle[index].focus_src);
		}
	}
	debug_log(" g_free : g_src_funcs(%#X), g_poll_fd(%#X)", g_src_funcs, g_poll_fd);

	if (g_src_funcs) {
		g_free(g_src_funcs);
		g_src_funcs = NULL;
	}
	if (g_poll_fd) {
		g_free(g_poll_fd);
		g_poll_fd = NULL;
	}

	g_focus_sound_handle[index].g_src_funcs = NULL;
	g_focus_sound_handle[index].g_poll_fd = NULL;
	g_focus_sound_handle[index].focus_src = NULL;
	g_focus_sound_handle[index].focus_callback = NULL;
	g_focus_sound_handle[index].watch_callback = NULL;

	debug_fleave();
	return ret;
}


static void _focus_add_callback(int index, bool is_for_watching)
{
	debug_fenter();
	if (!is_for_watching) {
		if (!_focus_add_sound_callback(index, g_focus_sound_handle[index].focus_fd, (gushort)POLLIN | POLLPRI, _focus_callback_handler)) {
			debug_error("failed to _focus_add_sound_callback()");
			//return false;
		}
	} else { // need to check if it's necessary
		if (!_focus_add_sound_callback(index, g_focus_sound_handle[index].focus_fd, (gushort)POLLIN | POLLPRI, _focus_watch_callback_handler)) {
			debug_error("failed to _focus_add_sound_callback()");
			//return false;
		}
	}
	debug_fleave();
}

static void _focus_remove_callback(int index)
{
	debug_fenter();
	if (!_focus_remove_sound_callback(index, (gushort)POLLIN | POLLPRI)) {
		debug_error("failed to __focus_remove_sound_callback()");
		//return false;
	}
	debug_fleave();
}

static void _focus_init_callback(int index, bool is_for_watching)
{
	debug_fenter();
	_focus_open_callback(index, is_for_watching);
	_focus_add_callback(index, is_for_watching);
	debug_fleave();
}

static void _focus_destroy_callback(int index, bool is_for_watching)
{
	debug_fenter();
	_focus_remove_callback(index);
	_focus_close_callback(index, is_for_watching);
	debug_fleave();
}

int mm_sound_client_get_unique_id(int *id)
{
	int ret = MM_ERROR_NONE;

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_id_mutex, MM_ERROR_SOUND_INTERNAL);
	debug_fenter();

	if (!id)
		ret = MM_ERROR_INVALID_ARGUMENT;
	else
		ret = mm_sound_client_dbus_get_unique_id(id);

	debug_fleave();
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_id_mutex);

	return ret;
}

int mm_sound_client_register_focus(int id, int pid, const char *stream_type, mm_sound_focus_changed_cb callback, bool is_for_session, void* user_data)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = 0;
	debug_fenter();

	instance = pid;

	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == false) {
			g_focus_sound_handle[index].is_used = true;
			break;
		}
	}

	g_focus_sound_handle[index].focus_tid = instance;
	g_focus_sound_handle[index].handle = id;
	g_focus_sound_handle[index].focus_callback = callback;
	g_focus_sound_handle[index].user_data = user_data;
	g_focus_sound_handle[index].is_for_session = is_for_session;

	ret = mm_sound_client_dbus_register_focus(id, pid, stream_type, callback, is_for_session, user_data);

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to register focus\n");
		if (!g_focus_thread) {
			GMainContext* focus_context = g_main_context_new ();
			g_focus_loop = g_main_loop_new (focus_context, FALSE);
			g_main_context_unref(focus_context);
			g_focus_thread = g_thread_new("focus-callback-thread", _focus_thread_func, NULL);
			if (g_focus_thread == NULL) {
				debug_error ("could not create thread..");
				g_main_loop_unref(g_focus_loop);
				g_focus_sound_handle[index].is_used = false;
				ret = MM_ERROR_SOUND_INTERNAL;
				goto cleanup;
			}
		}
	} else {
		debug_error("[Client] Error occurred : %d \n",ret);
		g_focus_sound_handle[index].is_used = false;
		goto cleanup;
	}

	_focus_init_callback(index, false);

cleanup:

	debug_fleave();
	return ret;
}

int mm_sound_client_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = -1;
	debug_fenter();

	index = _focus_find_index_by_handle(id);
	if (index == -1) {
		debug_error("Could not find index");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	instance = g_focus_sound_handle[index].focus_tid;

	if (!g_mutex_trylock(&g_focus_sound_handle[index].focus_lock)) {
		debug_warning("maybe focus_callback is being called, try one more time..");
		usleep(2500000); // 2.5 sec
		if (g_mutex_trylock(&g_focus_sound_handle[index].focus_lock)) {
			debug_msg("finally got focus_lock");
		}
	}

	ret = mm_sound_client_dbus_unregister_focus(instance, id, g_focus_sound_handle[index].is_for_session);

	if (ret == MM_ERROR_NONE)
		debug_msg("[Client] Success to unregister focus\n");
	else
		debug_error("[Client] Error occurred : %d \n",ret);

	g_mutex_unlock(&g_focus_sound_handle[index].focus_lock);

	_focus_destroy_callback(index, false);
	g_focus_sound_handle[index].focus_fd = 0;
	g_focus_sound_handle[index].focus_tid = 0;
	g_focus_sound_handle[index].handle = 0;
	g_focus_sound_handle[index].is_used = false;

	debug_fleave();
	return ret;
}

int mm_sound_client_disalbe_focus_reacquirement(int id, bool no_reacquirement)
{
	int ret = MM_ERROR_NONE;
	int index = -1;

	debug_fenter();

	index = _focus_find_index_by_handle(id);
	if (index == -1) {
		debug_error("Could not find index");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	g_focus_sound_handle[index].auto_reacquire = (no_reacquirement)? false : true;

	debug_fleave();
	return ret;
}

int mm_sound_client_acquire_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = -1;
	debug_fenter();

	index = _focus_find_index_by_handle(id);
	if (index == -1) {
		debug_error("Could not find index");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	instance = g_focus_sound_handle[index].focus_tid;

	ret = mm_sound_client_dbus_acquire_focus(instance, id, type, option, g_focus_sound_handle[index].is_for_session);

	if (ret == MM_ERROR_NONE)
		debug_msg("[Client] Success to acquire focus\n");
	else
		debug_error("[Client] Error occurred : %d \n",ret);

	debug_fleave();
	return ret;
}

int mm_sound_client_release_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = -1;
	debug_fenter();

	index = _focus_find_index_by_handle(id);
	if (index == -1) {
		debug_error("Could not find index");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	instance = g_focus_sound_handle[index].focus_tid;

	ret = mm_sound_client_dbus_release_focus(instance, id, type, option, g_focus_sound_handle[index].is_for_session);

	if (ret == MM_ERROR_NONE)
		debug_msg("[Client] Success to release focus\n");
	else
		debug_error("[Client] Error occurred : %d \n",ret);


	debug_fleave();
	return ret;
}

int mm_sound_client_set_focus_watch_callback(int pid, mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, bool is_for_session, void* user_data, int *id)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = 0;
	debug_fenter();

	if (!id)
		return MM_ERROR_INVALID_ARGUMENT;

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = pid;

	ret = mm_sound_client_dbus_get_unique_id(id);
	if (ret)
		return ret;

	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == false) {
			g_focus_sound_handle[index].is_used = true;
			break;
		}
	}

	g_focus_sound_handle[index].focus_tid = instance;
	g_focus_sound_handle[index].handle = *id;
	g_focus_sound_handle[index].watch_callback = callback;
	g_focus_sound_handle[index].user_data = user_data;
	g_focus_sound_handle[index].is_for_session = is_for_session;

	ret = mm_sound_client_dbus_set_focus_watch_callback(pid, g_focus_sound_handle[index].handle, focus_type, callback, is_for_session, user_data);

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to watch focus");
		if (!g_focus_thread) {
			GMainContext* focus_context = g_main_context_new ();
			g_focus_loop = g_main_loop_new (focus_context, FALSE);
			g_main_context_unref(focus_context);
			g_focus_thread = g_thread_new("focus-callback-thread", _focus_thread_func, NULL);
			if (g_focus_thread == NULL) {
				debug_error ("could not create thread..");
				g_main_loop_unref(g_focus_loop);
				ret = MM_ERROR_SOUND_INTERNAL;
				goto cleanup;
			}
		}
	} else {
		debug_error("[Client] Error occurred : %d",ret);
		goto cleanup;
	}

	_focus_init_callback(index, true);

cleanup:

	if (ret) {
		g_focus_sound_handle[index].is_used = false;
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_unset_focus_watch_callback(int id)
{
	int ret = MM_ERROR_NONE;
	int index = -1;
	debug_fenter();

	index = _focus_find_index_by_handle(id);
	if (index == -1) {
		debug_error("Could not find index");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	g_mutex_lock(&g_focus_sound_handle[index].focus_lock);

	ret = mm_sound_client_dbus_unset_focus_watch_callback(g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle, g_focus_sound_handle[index].is_for_session);

	if (ret == MM_ERROR_NONE)
		debug_msg("[Client] Success to unwatch focus\n");
	else
		debug_error("[Client] Error occurred : %d \n",ret);


	g_mutex_unlock(&g_focus_sound_handle[index].focus_lock);

	_focus_destroy_callback(index, true);
	g_focus_sound_handle[index].focus_fd = 0;
	g_focus_sound_handle[index].focus_tid = 0;
	g_focus_sound_handle[index].handle = 0;
	g_focus_sound_handle[index].is_used = false;

	debug_fleave();
	return ret;
}
#endif


int mm_sound_client_add_test_callback(mm_sound_test_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_dbus_add_test_callback(func, user_data, subs_id);

	debug_fleave();
	return ret;
}

int mm_sound_client_remove_test_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = mm_sound_client_dbus_remove_test_callback(subs_id);

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
