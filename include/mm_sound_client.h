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

#ifndef __MM_SOUND_CLIENT_H__
#define __MM_SOUND_CLIENT_H__

#include "mm_sound_private.h"
#include "mm_sound_device.h"
#ifdef USE_FOCUS
#include "mm_sound_focus.h"
#endif

//#define MEMTYPE_TRANS_PER_MAX (1024 * 1024) /* 1MB */

int mm_sound_client_initialize(void);
int mm_sound_client_finalize(void);
int mm_sound_client_play_tone(int number, int volume_config, double volume, int time, int *handle, bool enable_session);
int mm_sound_client_play_tone_with_stream_info(int tone, char *stream_type, int stream_id, double volume, int duration, int *handle);
int mm_sound_client_play_sound(MMSoundPlayParam *param, int tone, int *handle);
int mm_sound_client_play_sound_with_stream_info(MMSoundPlayParam *param, int *handle, char* stream_type, int stream_id);
int mm_sound_client_stop_sound(int handle);
int mm_sound_client_dbus_clear_sound_focus(int pid);
int mm_sound_client_is_route_available(mm_sound_route route, bool *is_available);
int mm_sound_client_foreach_available_route_cb(mm_sound_available_route_cb, void *user_data);
int mm_sound_client_set_active_route(mm_sound_route route, bool need_broadcast);
int mm_sound_client_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out);
int mm_sound_client_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data);
int mm_sound_client_remove_active_device_changed_callback(const char *name);
int mm_sound_client_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data);
int mm_sound_client_remove_available_route_changed_callback(void);
int mm_sound_client_set_volume_by_type(const int volume_type, const unsigned int volume_level);
int mm_sound_client_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data);
int mm_sound_client_remove_volume_changed_callback(void);
int mm_sound_client_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in);
int mm_sound_client_get_current_connected_device_list(int device_flgas, mm_sound_device_list_t **device_list);
int mm_sound_client_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data);
int mm_sound_client_remove_device_connected_callback(void);
int mm_sound_client_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data);
int mm_sound_client_remove_device_info_changed_callback(void);
#ifdef USE_FOCUS
int mm_sound_client_get_uniq_id(int *id);
int mm_sound_client_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data);
int mm_sound_client_unregister_focus(int id);
int mm_sound_client_acquire_focus(int id, mm_sound_focus_type_e type, const char *option);
int mm_sound_client_release_focus(int id, mm_sound_focus_type_e type, const char *option);
int mm_sound_client_set_focus_watch_callback(mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, void* user_data, int *id);
int mm_sound_client_unset_focus_watch_callback(int id);
#endif

int mm_sound_client_is_bt_a2dp_on (bool *connected, char** bt_name);
int mm_sound_client_set_active_route_auto(void);
int mm_sound_client_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out);

int mm_sound_client_add_test_callback(mm_sound_test_cb func, void* user_data);
int mm_sound_client_remove_test_callback(void);
int mm_sound_client_test(int a, int b, int* get);

#endif /* __MM_SOUND_CLIENT_H__ */
