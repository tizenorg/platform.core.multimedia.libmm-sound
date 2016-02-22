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

/**
 * @file		mm_sound_proxy.h
 * @brief		Client-Side proxy interface for audio module.
 * @date
 * @version		Release
 *
 * Client-Side proxy interface for audio module.
 */

#ifndef __MM_SOUND_PROXY_H__
#define __MM_SOUND_PROXY_H__

#include "mm_sound_private.h"
#include "mm_sound_device.h"
#ifdef USE_FOCUS
#include "mm_sound_focus.h"
#endif
#include "include/mm_sound_client.h"

typedef void (*mm_sound_proxy_userdata_free) (void *data);

int mm_sound_proxy_play_tone(int tone, int repeat, int volume, int volume_config,
			int session_type, int session_options, int client_pid,
			bool enable_session, int *codechandle, char *stream_type, int stream_index);
int mm_sound_proxy_play_tone_with_stream_info(int client_pid, int tone, char *stream_type, int stream_id, int volume, int repeat, int *codechandle);
int mm_sound_proxy_play_sound(const char* filename, int tone, int repeat, int volume, int volume_config,
			int priority, int session_type, int session_options, int client_pid, int handle_route,
			bool enable_session, int *codechandle, char *stream_type, int stream_index);
int mm_sound_proxy_play_sound_with_stream_info(const char* filename, int repeat, int volume,
			int priority, int client_pid, int handle_route, int *codechandle, char *stream_type, int stream_index);
int mm_sound_proxy_stop_sound(int handle);
int mm_sound_proxy_clear_focus(int pid); // Not original focus feature, only for cleaning up tone/wav player internal focus usage.
int mm_sound_proxy_add_play_sound_end_callback(mm_sound_stop_callback_wrapper_func func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id);
int mm_sound_proxy_remove_play_sound_end_callback(unsigned subs_id);
int mm_sound_proxy_get_current_connected_device_list(int device_flags, GList** device_list);
int mm_sound_proxy_add_device_connected_callback(int device_flags, mm_sound_device_connected_wrapper_cb func, void *userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id);
int mm_sound_proxy_remove_device_connected_callback(unsigned subs_id);
int mm_sound_proxy_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_wrapper_cb func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id);
int mm_sound_proxy_remove_device_info_changed_callback(unsigned subs_id);
int mm_sound_proxy_set_volume_by_type(const char *volume_type, const unsigned volume_level);
int mm_sound_proxy_add_volume_changed_callback(mm_sound_volume_changed_wrapper_cb func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id);
int mm_sound_proxy_remove_volume_changed_callback(unsigned subs_id);

#ifdef USE_FOCUS
int mm_sound_proxy_get_unique_id(int *id);
int mm_sound_proxy_register_focus(int id, int instance, const char *stream_type, mm_sound_focus_changed_cb callback, bool is_for_session, void* user_data);
int mm_sound_proxy_unregister_focus(int instance, int id, bool is_for_session);
int mm_sound_proxy_set_foucs_reacquisition(int instance, int id, bool reacquisition);
int mm_sound_proxy_get_acquired_focus_stream_type(int focus_type, char **stream_type, char **additional_info);
int mm_sound_proxy_acquire_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session);
int mm_sound_proxy_release_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session);
int mm_sound_proxy_set_focus_watch_callback(int instance, int handle, mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, bool is_for_session, void *user_data);
int mm_sound_proxy_unset_focus_watch_callback(int focus_tid, int handle, bool is_for_session);
int mm_sound_proxy_emergent_exit_focus(int exit_pid);
#endif

int mm_sound_proxy_add_test_callback(mm_sound_test_cb func, void *userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id);
int mm_sound_proxy_remove_test_callback(unsigned subs_id);
int mm_sound_proxy_test(int a, int b, int* get);

int mm_sound_proxy_initialize(void);
int mm_sound_proxy_finalize(void);

#endif /* __MM_SOUND_PROXY_H__ */