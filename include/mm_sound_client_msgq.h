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

#ifndef __MM_SOUND_CLIENT_MSGQ_H__
#define __MM_SOUND_CLIENT_MSGQ_H__

#include "mm_sound_private.h"
#include "mm_sound_device.h"
#ifdef USE_FOCUS
#include "mm_sound_focus.h"
#endif

//#define MEMTYPE_TRANS_PER_MAX (1024 * 1024) /* 1MB */

int MMSoundClientMsgqInit(void);
int MMSoundClientMsgqCallbackFini(void);
int MMSoundClientMsgqPlayTone(int number, int volume_config, double volume, int time, int *handle, bool enable_session);
int MMSoundClientMsgqPlaySound(MMSoundPlayParam *param, int tone, int keytone, int *handle);
int MMSoundClientMsgqStopSound(int handle);
int _mm_sound_client_msgq_is_route_available(mm_sound_route route, bool *is_available);
int _mm_sound_client_msgq_foreach_available_route_cb(mm_sound_available_route_cb , void *user_data);
int _mm_sound_client_msgq_set_active_route(mm_sound_route route, bool need_broadcast);
int _mm_sound_client_msgq_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out);
int _mm_sound_client_msgq_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data);
int _mm_sound_client_msgq_remove_active_device_changed_callback(const char *name);
int _mm_sound_client_msgq_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data);
int _mm_sound_client_msgq_remove_available_route_changed_callback(void);
int _mm_sound_client_msgq_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data);
int _mm_sound_client_msgq_remove_volume_changed_callback(void);
int _mm_sound_client_msgq_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in);
int _mm_sound_client_msgq_get_current_connected_device_list(int device_flgas, mm_sound_device_list_t **device_list);
int _mm_sound_client_msgq_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data);
int _mm_sound_client_msgq_remove_device_connected_callback(void);
int _mm_sound_client_msgq_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data);
int _mm_sound_client_msgq_remove_device_info_changed_callback(void);
#ifdef USE_FOCUS
int _mm_sound_client_msgq_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data);
int _mm_sound_client_msgq_unregister_focus(int id);
int _mm_sound_client_msgq_acquire_focus(int id, mm_sound_focus_type_e type, const char *option);
int _mm_sound_client_msgq_release_focus(int id, mm_sound_focus_type_e type, const char *option);
int _mm_sound_client_msgq_set_focus_watch_callback(mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, void* user_data);
int _mm_sound_client_msgq_unset_focus_watch_callback(void);
#endif

#ifdef PULSE_CLIENT
int MMSoundClientMsgqIsBtA2dpOn (bool *connected, char** bt_name);
#endif
int _mm_sound_client_msgq_set_active_route_auto(void);
int _mm_sound_client_msgq_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out);

#endif /* __MM_SOUND_CLIENT_MSGQ_H__ */