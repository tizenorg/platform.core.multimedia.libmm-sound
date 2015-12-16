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

/**
 * @file		mm_sound_stream.h
 * @brief		Application interface library for sound module.
 * @date
 *
 * Application interface library for sound module.
 */

#ifndef	__MM_SOUND_STREAM_H__
#define	__MM_SOUND_STREAM_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <glib.h>
#define MAX_STREAM_TYPE_LEN 64
#define NUM_OF_STREAM_IO_TYPE         2    /* playback / capture */
#define AVAIL_STREAMS_MAX 32
typedef struct _stream_list {
	gchar *stream_types[AVAIL_STREAMS_MAX];
	int priorities[AVAIL_STREAMS_MAX];
} stream_list_t;

#ifdef __cplusplus
}
#endif

#endif	/* __MM_SOUND_STREAM_H__ */
