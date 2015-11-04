/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Sangchul Lee <sc11.lee@samsung.com>
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
#include <unistd.h>
#include <stdio.h>
#include <vconf.h>

#include <mm_debug.h>

#include "include/mm_sound.h"
#include "include/mm_sound_client.h"
#include "include/mm_sound_focus.h"
#include "focus_server/include/mm_sound_mgr_focus.h"

#define RETURN_ERROR_IF_FOCUS_CB_THREAD(x_thread) \
{ \
	int ret = MM_ERROR_NONE; \
	bool result = false; \
	ret = mm_sound_client_is_focus_cb_thread(x_thread, &result); \
	if (ret) \
		return ret; \
	else (result) { \
		debug_error("it might be called in the thread of focus callback, it is not allowed\n"); \
		return MM_ERROR_SOUND_INVALID_OPERATION; \
	} \
} \

EXPORT_API
int mm_sound_focus_set_session_interrupt_callback(mm_sound_focus_session_interrupt_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (!callback)
		return MM_ERROR_INVALID_ARGUMENT;

	ret = mm_sound_client_set_session_interrupt_callback (callback, user_data);

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_focus_unset_session_interrupt_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	ret = mm_sound_client_unset_session_interrupt_callback ();
	if (ret) {
		debug_error("Failed to mm_sound_client_unset_session_interrupt_callback(), ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_focus_get_id(int *id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	ret = mm_sound_client_get_unique_id(id);
	if (ret) {
		debug_error("Failed to mm_sound_client_get_unique_id(), ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_focus_check_thread_self(bool *focus_cb_thread)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_is_focus_cb_thread(g_thread_self(), focus_cb_thread);
	if (!ret) {
		if (*focus_cb_thread)
			debug_error("it might be called in the thread of focus callback, it is not allowed\n");
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (id < 0 || callback == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_register_focus(id, getpid(), stream_type, callback, false, user_data);
	if (ret) {
		debug_error("Could not register focus, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_register_focus_for_session(int id, int pid, const char *stream_type, mm_sound_focus_changed_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (id < 0 || callback == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_register_focus(id, pid, stream_type, callback, true, user_data);
	if (ret) {
		debug_error("Could not register focus for session, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (id < 0) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_unregister_focus(id);
	if (ret) {
		debug_error("Could not unregister focus, ret = %x\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_acquire_focus(int id, mm_sound_focus_type_e focus_type, const char *additional_info)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (id < 0) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (focus_type < FOCUS_FOR_PLAYBACK || focus_type > FOCUS_FOR_BOTH) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_acquire_focus(id, focus_type, additional_info);
	if (ret) {
		debug_error("Could not acquire focus, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_release_focus(int id, mm_sound_focus_type_e focus_type, const char *additional_info)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (id < 0) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (focus_type < FOCUS_FOR_PLAYBACK || focus_type > FOCUS_FOR_BOTH) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_release_focus(id, focus_type, additional_info);
	if (ret) {
		debug_error("Could not release focus, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_set_focus_watch_callback(mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void *user_data, int *id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (callback == NULL || id == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	ret = mm_sound_client_set_focus_watch_callback(getpid(), focus_type, callback, false, user_data, id);
	if (ret) {
		debug_error("Could not set focus watch callback, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_set_focus_watch_callback_for_session(int pid, mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void *user_data, int *id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	if (callback == NULL || id == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	ret = mm_sound_client_set_focus_watch_callback(pid, focus_type, callback, true, user_data, id);
	if (ret) {
		debug_error("Could not set focus watch callback, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_unset_focus_watch_callback(int id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	RETURN_ERROR_IF_FOCUS_CB_THREAD(g_thread_self());

	ret = mm_sound_client_unset_focus_watch_callback(id);
	if (ret) {
		debug_error("Could not unset focus watch callback, id(%d), ret = %x\n", id, ret);
	}

	debug_fleave();

	return ret;
}
