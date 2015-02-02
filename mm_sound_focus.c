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
#include "include/mm_sound_focus.h"
#include "server/include/mm_sound_mgr_focus.h"

EXPORT_API
int mm_sound_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (id < 0 || callback == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = _mm_sound_client_register_focus(id, stream_type, callback, user_data);
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

	ret = _mm_sound_client_unregister_focus(id);
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

	ret = _mm_sound_client_acquire_focus(id, focus_type, additional_info);
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

	ret = _mm_sound_client_release_focus(id, focus_type, additional_info);
	if (ret) {
		debug_error("Could not release focus, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_set_focus_watch_callback(mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (callback == NULL) {
		debug_error("argument is not valid\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = _mm_sound_client_set_focus_watch_callback(focus_type, callback, user_data);
	if (ret) {
		debug_error("Could not set focus watch callback, ret[0x%x]\n", ret);
	}

	debug_fleave();

	return ret;
}

EXPORT_API
int mm_sound_unset_focus_watch_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	ret = _mm_sound_client_unset_focus_watch_callback();
	if (ret) {
		debug_error("Could not unset focus watch callback, ret = %x\n", ret);
	}

	debug_fleave();

	return ret;
}
