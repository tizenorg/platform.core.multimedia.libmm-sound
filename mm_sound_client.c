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

/* global variables for device list */
//static GList *g_device_list = NULL;
static mm_sound_device_list_t g_device_list_t;
static pthread_mutex_t g_device_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int MMSoundClientInit(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	g_type_init();
	MMSoundClientDbusInit();

	debug_fleave();
	return ret;
}

int MMSoundClientCallbackFini(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	MMSoundClientDbusFini();

	debug_fleave();
	return MM_ERROR_NONE;
}

int _mm_sound_client_is_route_available(mm_sound_route route, bool *is_available)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_is_route_available(route, is_available);

	debug_fleave();
	return ret;

}

int _mm_sound_client_foreach_available_route_cb(mm_sound_available_route_cb available_route_cb, void *user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_foreach_available_route_cb(available_route_cb, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_set_active_route(mm_sound_route route, bool need_broadcast)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_set_active_route(route, need_broadcast);

	debug_fleave();
	return ret;

}

int _mm_sound_client_set_active_route_auto(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_set_active_route_auto();

	debug_fleave();
	return ret;

}

int _mm_sound_client_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{

	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_get_active_device(device_in, device_out);

	debug_fleave();
	return ret;
}

int _mm_sound_client_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_add_active_device_changed_callback(name, func, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_remove_active_device_changed_callback(const char *name)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_remove_active_device_changed_callback(name);

	debug_fleave();
	return ret;
}
int _mm_sound_client_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_add_available_route_changed_callback(func, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_remove_available_route_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_remove_available_route_changed_callback();

	debug_fleave();
	return ret;
}

#ifdef PULSE_CLIENT

int _mm_sound_client_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_set_sound_path_for_active_device(device_out, device_in);

	debug_fleave();
	return ret;
}

#endif // PULSE_CLIENT




/*****************************************************************************************
			    DBUS SUPPORTED FUNCTIONS
******************************************************************************************/

static int __mm_sound_device_check_flags_to_append (int device_flags, mm_sound_device_t *device_h, bool *is_good_to_append);
static int __mm_sound_device_check_flags_to_trigger (int device_flags, mm_sound_device_t *device_h, bool *is_good_to_go);

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/


int MMSoundClientPlayTone(int number, int volume_config, double volume, int time, int *handle, bool enable_session)
{
	int ret = MM_ERROR_NONE;
//	 int instance = -1; 	/* instance is unique to communicate with server : client message queue filter type */

	 debug_fenter();

	 /* read session information */
	 int session_type = MM_SESSION_TYPE_MEDIA;
	 int session_options = 0;
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

	ret = MMSoundClientDbusPlayTone(number, time, volume, volume_config,
					session_type, session_options, getpid(), enable_session, handle );

	debug_fleave();
	return ret;
}


int MMSoundClientPlaySound(MMSoundPlayParam *param, int tone, int keytone, int *handle)
{
	int ret = MM_ERROR_NONE;
	int session_type = MM_SESSION_TYPE_MEDIA;
	int session_options = 0;
//	int instance = -1; 	/* instance is unique to communicate with server : client message queue filter type */

	debug_fenter();

	/* read session information */

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

	ret = MMSoundClientDbusPlaySound(param->filename, tone, param->loop, param->volume, param->volume_config,
					 param->priority, session_type, session_options, getpid(), keytone, param->handle_route,
					 !param->skip_session, handle);
	if (ret != MM_ERROR_NONE) {
		debug_error("Play Sound Failed");
		goto failed;
	} 
	ret = _mm_sound_client_dbus_add_play_sound_end_callback(param->callback, param->data, *handle);
	if (ret != MM_ERROR_NONE) {
		debug_error("Add callback for play sound(%d) Failed", *handle);
	}

failed:

	debug_fleave();
	return ret;
}

int MMSoundClientStopSound(int handle)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = MMSoundClientDbusStopSound(handle);

	debug_fleave();
	return ret;
}

static int __mm_sound_device_check_flags_to_append (int device_flags, mm_sound_device_t *device_h, bool *is_good_to_append)
{
	bool need_to_append = false;
	int need_to_check_for_io_direction = device_flags & DEVICE_IO_DIRECTION_FLAGS;
	int need_to_check_for_state = device_flags & DEVICE_STATE_FLAGS;
	int need_to_check_for_type = device_flags & DEVICE_TYPE_FLAGS;

	debug_warning("device_h[0x%x], device_flags[0x%x], need_to_check(io_direction[0x%x],state[0x%x],type[0x%x])\n",
			device_h, device_flags, need_to_check_for_io_direction, need_to_check_for_state, need_to_check_for_type);

	if(!device_h) {
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (device_flags == DEVICE_ALL_FLAG) {
		*is_good_to_append = true;
		return MM_ERROR_NONE;
	}

	if (need_to_check_for_io_direction) {
		if ((device_h->io_direction == DEVICE_IO_DIRECTION_IN) && (device_flags & DEVICE_IO_DIRECTION_IN_FLAG)) {
			need_to_append = true;
		} else if ((device_h->io_direction == DEVICE_IO_DIRECTION_OUT) && (device_flags & DEVICE_IO_DIRECTION_OUT_FLAG)) {
			need_to_append = true;
		} else if ((device_h->io_direction == DEVICE_IO_DIRECTION_BOTH) && (device_flags & DEVICE_IO_DIRECTION_BOTH_FLAG)) {
			need_to_append = true;
		}
		if (need_to_append) {
			if (!need_to_check_for_state && !need_to_check_for_type) {
				*is_good_to_append = true;
				return MM_ERROR_NONE;
			}
		} else {
			*is_good_to_append = false;
			return MM_ERROR_NONE;
		}
	}
	if (need_to_check_for_state) {
		need_to_append = false;
		if ((device_h->state == DEVICE_STATE_DEACTIVATED) && (device_flags & DEVICE_STATE_DEACTIVATED_FLAG)) {
			need_to_append = true;
		} else if ((device_h->state == DEVICE_STATE_ACTIVATED) && (device_flags & DEVICE_STATE_ACTIVATED_FLAG)) {
			need_to_append = true;
		}
		if (need_to_append) {
			if (!need_to_check_for_type) {
				*is_good_to_append = true;
				return MM_ERROR_NONE;
			}
		} else {
			*is_good_to_append = false;
			return MM_ERROR_NONE;
		}
	}
	if (need_to_check_for_type) {
		need_to_append = false;
		bool is_internal_device = IS_INTERNAL_DEVICE(device_h->type);
		if (is_internal_device && (device_flags & DEVICE_TYPE_INTERNAL_FLAG)) {
			need_to_append = true;
		} else if (!is_internal_device && (device_flags & DEVICE_TYPE_EXTERNAL_FLAG)) {
			need_to_append = true;
		}
		if (need_to_append) {
			*is_good_to_append = true;
			return MM_ERROR_NONE;
		} else {
			*is_good_to_append = false;
			return MM_ERROR_NONE;
		}
	}
	return MM_ERROR_NONE;
}


static int __mm_sound_device_check_flags_to_trigger (int device_flags, mm_sound_device_t *device_h, bool *is_good_to_go)
{
	bool need_to_go = false;
	int need_to_check_for_io_direction = device_flags & DEVICE_IO_DIRECTION_FLAGS;
	int need_to_check_for_state = device_flags & DEVICE_STATE_FLAGS;
	int need_to_check_for_type = device_flags & DEVICE_TYPE_FLAGS;

	debug_warning("device_h[0x%x], device_flags[0x%x], need_to_check(io_direction[0x%x],state[0x%x],type[0x%x])\n",
			device_h, device_flags, need_to_check_for_io_direction, need_to_check_for_state, need_to_check_for_type);

	if(!device_h) {
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (device_flags == DEVICE_ALL_FLAG) {
		*is_good_to_go = true;
		return MM_ERROR_NONE;
	}

	if (need_to_check_for_io_direction) {
		if ((device_h->io_direction == DEVICE_IO_DIRECTION_IN) && (device_flags & DEVICE_IO_DIRECTION_IN_FLAG)) {
			need_to_go = true;
		} else if ((device_h->io_direction == DEVICE_IO_DIRECTION_OUT) && (device_flags & DEVICE_IO_DIRECTION_OUT_FLAG)) {
			need_to_go = true;
		} else if ((device_h->io_direction == DEVICE_IO_DIRECTION_BOTH) && (device_flags & DEVICE_IO_DIRECTION_BOTH_FLAG)) {
			need_to_go = true;
		}
		if (need_to_go) {
			if (!need_to_check_for_state && !need_to_check_for_type) {
				*is_good_to_go = true;
				return MM_ERROR_NONE;
			}
		} else {
			*is_good_to_go = false;
			return MM_ERROR_NONE;
		}
	}
	if (need_to_check_for_state) {
		need_to_go = false;
		if ((device_h->state == DEVICE_STATE_DEACTIVATED) && (device_flags & DEVICE_STATE_DEACTIVATED_FLAG)) {
			need_to_go = true;
		} else if ((device_h->state == DEVICE_STATE_ACTIVATED) && (device_flags & DEVICE_STATE_ACTIVATED_FLAG)) {
			need_to_go = true;
		}
		if (need_to_go) {
			if (!need_to_check_for_type) {
				*is_good_to_go = true;
				return MM_ERROR_NONE;
			}
		} else {
			*is_good_to_go = false;
			return MM_ERROR_NONE;
		}
	}
	if (need_to_check_for_type) {
		need_to_go = false;
		bool is_internal_device = IS_INTERNAL_DEVICE(device_h->type);
		if (is_internal_device && (device_flags & DEVICE_TYPE_INTERNAL_FLAG)) {
			need_to_go = true;
		} else if (!is_internal_device && (device_flags & DEVICE_TYPE_EXTERNAL_FLAG)) {
			need_to_go = true;
		}
		if (need_to_go) {
			*is_good_to_go = true;
			return MM_ERROR_NONE;
		} else {
			*is_good_to_go = false;
			return MM_ERROR_NONE;
		}
	}
	return MM_ERROR_NONE;
}

#if 0
static int __mm_sound_client_device_list_clear ()
{
	int ret = MM_ERROR_NONE;

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_device_list_mutex, MM_ERROR_SOUND_INTERNAL);

	if (g_device_list) {
		g_list_free_full(g_device_list, g_free);
		g_device_list = NULL;
	}

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_device_list_mutex);

	return ret;
}
#endif


static int __mm_sound_client_device_list_clear ()
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

#if 0
static int __mm_sound_client_device_list_append_item (mm_sound_device_t *device_h)
{
	int ret = MM_ERROR_NONE;
	mm_sound_device_t *device_node = g_malloc0(sizeof(mm_sound_device_t));
	memcpy(device_node, device_h, sizeof(mm_sound_device_t));


	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_device_list_mutex, MM_ERROR_SOUND_INTERNAL);

	g_device_list = g_list_append(g_device_list, device_node);
	debug_log("[Client] g_device_list[0x%x], new device_node[0x%x] is appended, type[%d], id[%d]\n", g_device_list, device_node, device_node->type, device_node->id);

	MMSOUND_LEAVE_CRITICAL_SECTION(&g_device_list_mutex);

	return ret;
}
#endif

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
			debug_log(" list idx[%d]: type[%02d], id[%02d], io_direction[%d], state[%d], name[%s]\n",
						count++, device_node->type, device_node->id, device_node->io_direction, device_node->state, device_node->name);
		}
	}
	debug_log("======================== device list : end ============================\n");

	return ret;
}

int _mm_sound_client_get_current_connected_device_list(int device_flags, mm_sound_device_list_t **device_list)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = __mm_sound_client_device_list_clear();
	if (ret) {
		debug_error("[Client] failed to __mm_sound_client_device_list_clear(), ret[0x%x]\n", ret);
		return ret;
	}

	if((ret = _mm_sound_client_dbus_get_current_connected_device_list(device_flags, &g_device_list_t))!=MM_ERROR_NONE){
		debug_error("[Client] failed to get current connected device list with dbus, ret[0x%x]", ret);
		goto failed;
	}
	if (!g_device_list_t.list) {
		debug_error("Got device list null");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto failed;
	}
//		g_device_list_t.list = g_device_list;
	_mm_sound_client_device_list_dump(g_device_list_t.list);
	*device_list = &g_device_list_t;

failed:
	debug_fleave();
	return ret;
}

int _mm_sound_client_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_add_device_connected_callback(device_flags, func, user_data);

	debug_fleave();
	return ret;

}

int _mm_sound_client_remove_device_connected_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_remove_device_connected_callback();

	debug_fleave();
	return ret;
}

int _mm_sound_client_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_add_device_info_changed_callback(device_flags, func, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_remove_device_info_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret =  _mm_sound_client_dbus_remove_device_info_changed_callback();

	debug_fleave();
	return ret;

}
int MMSoundClientIsBtA2dpOn (bool *connected, char** bt_name)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = MMSoundClientDbusIsBtA2dpOn(connected, bt_name);

	debug_fleave();
	return ret;
}


int _mm_sound_client_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_add_volume_changed_callback(func, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_remove_volume_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_remove_volume_changed_callback();

	debug_fleave();
	return ret;
}


int _mm_sound_client_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_get_audio_path(device_in, device_out);

	debug_fleave();

	return ret;
}


#ifdef USE_FOCUS
int _mm_sound_client_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_register_focus(id, stream_type, callback,  user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_unregister_focus(id);

	debug_fleave();
	return ret;
}

int _mm_sound_client_acquire_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_acquire_focus(id, type, option);

	debug_fleave();
	return ret;
}

int _mm_sound_client_release_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	_mm_sound_client_dbus_release_focus(id, type, option);

	debug_fleave();
	return ret;
}

int _mm_sound_client_set_focus_watch_callback(mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	_mm_sound_client_dbus_set_focus_watch_callback(focus_type, callback, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_unset_focus_watch_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_unset_focus_watch_callback();

	debug_fleave();
	return ret;
}
#endif


int _mm_sound_client_add_test_callback(mm_sound_test_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_add_test_callback(func, user_data);

	debug_fleave();
	return ret;
}

int _mm_sound_client_remove_test_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = _mm_sound_client_dbus_remove_test_callback();

	debug_fleave();
	return ret;
}


int _mm_sound_client_test(int a, int b, int* getv)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_dbus_test(a, b, getv);
	debug_log("%d * %d -> result : %d", a, b, *getv);

	debug_fleave();

	return ret;
}


