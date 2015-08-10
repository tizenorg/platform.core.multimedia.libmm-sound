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

struct sigaction FOCUS_int_old_action;
struct sigaction FOCUS_abrt_old_action;
struct sigaction FOCUS_segv_old_action;
struct sigaction FOCUS_term_old_action;
struct sigaction FOCUS_sys_old_action;
struct sigaction FOCUS_xcpu_old_action;

EXPORT_API
int mm_sound_focus_get_id(int *id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = mm_sound_client_get_uniq_id(id);
	if (ret) {
		debug_error("Failed to mm_sound_client_get_uniq_id(), ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (id < 0 || callback == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = mm_sound_client_register_focus(id, stream_type, callback, user_data);
	if (ret) {
		debug_error("Could not register focus, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

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

	if (callback == NULL || id == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	ret = mm_sound_client_set_focus_watch_callback(focus_type, callback, user_data, id);
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

	ret = mm_sound_client_unset_focus_watch_callback(id);
	if (ret) {
		debug_error("Could not unset focus watch callback, id(%d), ret = %x\n", id, ret);
	}

	debug_fleave();

	return ret;
}

void __FOCUS_signal_handler(int signo)
{
	int exit_pid = 0;
	sigset_t old_mask, all_mask;

	debug_error("Got signal : signo(%d)", signo);

	/* signal block */

	sigfillset(&all_mask);
	sigprocmask(SIG_BLOCK, &all_mask, &old_mask);

	exit_pid = getpid();

	/* need implementation */
	//send exit pid to focus server and focus server will clear focus or watch if necessary.
}

void __attribute__((constructor)) __FOCUS_init_module(void)
{
	struct sigaction FOCUS_action;
	FOCUS_action.sa_handler = __FOCUS_signal_handler;
	FOCUS_action.sa_flags = SA_NOCLDSTOP;

	debug_fenter();

	sigemptyset(&FOCUS_action.sa_mask);

	sigaction(SIGINT, &FOCUS_action, &FOCUS_int_old_action);
	sigaction(SIGABRT, &FOCUS_action, &FOCUS_abrt_old_action);
	sigaction(SIGSEGV, &FOCUS_action, &FOCUS_segv_old_action);
	sigaction(SIGTERM, &FOCUS_action, &FOCUS_term_old_action);
	sigaction(SIGSYS, &FOCUS_action, &FOCUS_sys_old_action);
	sigaction(SIGXCPU, &FOCUS_action, &FOCUS_xcpu_old_action);

	debug_fleave();

	
}

void __attribute__((destructor)) __FOCUS_fini_module(void)
{

	debug_fenter();

	/* is it necessary? */
	sigaction(SIGINT, &FOCUS_int_old_action, NULL);
	sigaction(SIGABRT, &FOCUS_abrt_old_action, NULL);
	sigaction(SIGSEGV, &FOCUS_segv_old_action, NULL);
	sigaction(SIGTERM, &FOCUS_term_old_action, NULL);
	sigaction(SIGSYS, &FOCUS_sys_old_action, NULL);
	sigaction(SIGXCPU, &FOCUS_xcpu_old_action, NULL);

	debug_fleaver();

}
