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

#define MAX_STREAM_TYPE_LEN 64

#define STREAM_TYPE_MEDIA             "media"
#define STREAM_TYPE_SYSTEM            "system"
#define STREAM_TYPE_ALARM             "alarm"
#define STREAM_TYPE_NOTIFICATION      "notification"
#define STREAM_TYPE_EMERGENCY         "emergency"
#define STREAM_TYPE_TTS               "tts"
#define STREAM_TYPE_RINGTONE          "ringtone"
#define STREAM_TYPE_CALL              "call"
#define STREAM_TYPE_VOIP              "voip"
#define STREAM_TYPE_VOICE_RECOGNITION "voice_recognition"
#define STREAM_TYPE_RADIO             "radio"
#define STREAM_TYPE_LOOPBACK          "loopback"

#define NUM_OF_STREAM_IO_TYPE         2    /* playback / capture */

#ifdef __cplusplus
}
#endif

#endif	/* __MM_SOUND_STREAM_H__ */

