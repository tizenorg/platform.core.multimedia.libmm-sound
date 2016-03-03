/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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
 * @file		mm_sound_intf.h
 * @brief		Internal audio interfaces for audio module.
 * @date
 * @version		Release
 *
 * Internal audio interfaces for audio module.
 * Client or Service providers can request(or handle) services,
 * or be notified events.
 */

#ifndef __MM_SOUND_INTF_H__
#define __MM_SOUND_INTF_H__

/* audio service(methods, signals) providers */
typedef enum audio_provider {
	AUDIO_PROVIDER_SOUND_SERVER,
	AUDIO_PROVIDER_DEVICE_MANAGER,
	AUDIO_PROVIDER_STREAM_MANAGER,
	AUDIO_PROVIDER_FOCUS_SERVER,
	AUDIO_PROVIDER_AUDIO_CLIENT,
	AUDIO_PROVIDER_MAX
} audio_provider_t;

typedef enum audio_method {
	AUDIO_METHOD_TEST,
	AUDIO_METHOD_PLAY_FILE_START,
	AUDIO_METHOD_PLAY_FILE_START_WITH_STREAM_INFO,
	AUDIO_METHOD_PLAY_FILE_STOP,
	AUDIO_METHOD_PLAY_DTMF,
	AUDIO_METHOD_PLAY_DTMF_WITH_STREAM_INFO,
	AUDIO_METHOD_CLEAR_FOCUS, // Not original focus feature, only for tone/wav player internal focus usage.
	AUDIO_METHOD_GET_BT_A2DP_STATUS,
	AUDIO_METHOD_SET_PATH_FOR_ACTIVE_DEVICE,
	AUDIO_METHOD_GET_CONNECTED_DEVICE_LIST,
	AUDIO_METHOD_GET_AUDIO_PATH,
	AUDIO_METHOD_SET_VOLUME_LEVEL,

	AUDIO_METHOD_GET_UNIQUE_ID,
	AUDIO_METHOD_REGISTER_FOCUS,
	AUDIO_METHOD_UNREGISTER_FOCUS,
	AUDIO_METHOD_SET_FOCUS_REACQUISITION,
	AUDIO_METHOD_GET_ACQUIRED_FOCUS_STREAM_TYPE,
	AUDIO_METHOD_ACQUIRE_FOCUS,
	AUDIO_METHOD_RELEASE_FOCUS,
	AUDIO_METHOD_WATCH_FOCUS,
	AUDIO_METHOD_UNWATCH_FOCUS,

	AUDIO_METHOD_MAX
} audio_method_t;

typedef enum audio_event {
	AUDIO_EVENT_TEST,
	AUDIO_EVENT_PLAY_FILE_END,
	AUDIO_EVENT_VOLUME_CHANGED,
	AUDIO_EVENT_DEVICE_CONNECTED,
	AUDIO_EVENT_DEVICE_INFO_CHANGED,
	AUDIO_EVENT_FOCUS_CHANGED,
	AUDIO_EVENT_FOCUS_WATCH,
	AUDIO_EVENT_EMERGENT_EXIT,
	AUDIO_EVENT_CLIENT_SUBSCRIBED, /* Clients send this signal when they subscribed some signal. */
	AUDIO_EVENT_CLIENT_HANDLED, /* Clients send this siganl when they handled some signal. */
	AUDIO_EVENT_MAX
} audio_event_t;

#endif /* __MM_SOUND_INTF_H__  */
