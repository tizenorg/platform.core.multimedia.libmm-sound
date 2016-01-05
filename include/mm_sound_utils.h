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
 * @file		mm_sound_utils.h
 * @brief		Internal utility library for sound module.
 * @date
 * @version		Release
 *
 * Internal utility library for sound module.
 */

#ifndef __MM_SOUND_UTILS_H__
#define __MM_SOUND_UTILS_H__

#include <mm_types.h>
#include <mm_error.h>
#include <unistd.h>

#include "../include/mm_sound.h"

#ifdef __cplusplus
extern "C" {
#endif

	int mm_sound_util_volume_get_value_by_type(volume_type_t type, unsigned int *value);
	int mm_sound_util_volume_set_value_by_type(volume_type_t type, unsigned int value);

	bool mm_sound_util_is_recording(void);
	bool mm_sound_util_is_process_alive(pid_t pid);

#ifdef __cplusplus
}
#endif
#endif							/* __MM_SOUND_UTILS_H__ */
