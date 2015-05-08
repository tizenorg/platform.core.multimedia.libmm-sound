/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
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

/**
 * @file		mm_sound_focus.h
 * @brief		Application interface library for sound module.
 * @date
 *
 * Application interface library for sound module.
 */

#ifndef	__MM_SOUND_FOCUS_H__
#define	__MM_SOUND_FOCUS_H__

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum {
	FOCUS_IS_RELEASED,
	FOCUS_IS_ACQUIRED,
} mm_sound_focus_state_e;

typedef enum {
	FOCUS_FOR_PLAYBACK = 1,
	FOCUS_FOR_CAPTURE,
	FOCUS_FOR_BOTH,
} mm_sound_focus_type_e;

int mm_sound_focus_get_id(int *id);

typedef void (*mm_sound_focus_changed_cb) (int id, mm_sound_focus_type_e focus_type, mm_sound_focus_state_e state, const char *reason_for_change, const char *additional_info, void *user_data);
int mm_sound_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void *user_data);
int mm_sound_unregister_focus(int id);
int mm_sound_acquire_focus(int id, mm_sound_focus_type_e focus_type, const char *additional_info);
int mm_sound_release_focus(int id, mm_sound_focus_type_e focus_type, const char *additional_info);

typedef void (*mm_sound_focus_changed_watch_cb) (int id, mm_sound_focus_type_e focus_type, mm_sound_focus_state_e state, const char *reason_for_change, const char *additional_info, void *user_data);
int mm_sound_set_focus_watch_callback(mm_sound_focus_type_e focus_type, mm_sound_focus_changed_watch_cb callback, void *user_data, int *id);
int mm_sound_unset_focus_watch_callback(int id);


#ifdef __cplusplus
}
#endif

#endif	/* __MM_SOUND_FOCUS_H__ */

