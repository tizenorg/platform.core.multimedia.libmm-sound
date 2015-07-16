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

#ifndef __MM_SOUND_MSG_H__
#define __MM_SOUND_MSG_H__

#include <sys/time.h>
#include <unistd.h>
#include <mm_ipc.h>

#define KEY_BASE_PATH	"/tmp"
#define RCV_MSG	0x21	/* rcv key */
#define SND_MSG 0x24	/* snd key */
#define CB_MSG   0x64		/* cb key */
#define RCV_CB_MSG   0x41	/* cb rcv key */
#define SND_CB_MSG   0x44	/* cb snd key */

#define MEMTYPE_SUPPORT_MAX (1024 * 1024) /* 1MB */

enum {
	MM_SOUND_MSG_REQ_FILE = 1,
	MM_SOUND_MSG_REQ_MEMORY = 2,
	MM_SOUND_MSG_REQ_STOP = 3,
	MM_SOUND_MSG_RES_FILE = 4,
	MM_SOUND_MSG_RES_MEMORY = 5,
	MM_SOUND_MSG_RES_STOP = 6,
	MM_SOUND_MSG_INF_STOP_CB = 7,
	MM_SOUND_MSG_RES_ERROR = 8,
	MM_SOUND_MSG_INF_DESTROY_CB = 9,
#ifdef PULSE_CLIENT
	MM_SOUND_MSG_REQ_GET_AUDIO_ROUTE = 16,
	MM_SOUND_MSG_RES_GET_AUDIO_ROUTE = 17,
	MM_SOUND_MSG_REQ_SET_AUDIO_ROUTE = 18,
	MM_SOUND_MSG_RES_SET_AUDIO_ROUTE = 19,
#endif // PULSE_CLIENT
	MM_SOUND_MSG_REQ_IS_BT_A2DP_ON = 20,
	MM_SOUND_MSG_RES_IS_BT_A2DP_ON = 21,
	MM_SOUND_MSG_REQ_DTMF  = 22,
	MM_SOUND_MSG_RES_DTMF = 23,
	MM_SOUND_MSG_REQ_IS_ROUTE_AVAILABLE,
	MM_SOUND_MSG_RES_IS_ROUTE_AVAILABLE,
	MM_SOUND_MSG_REQ_FOREACH_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_RES_FOREACH_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_INF_FOREACH_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_REQ_SET_ACTIVE_ROUTE,
	MM_SOUND_MSG_RES_SET_ACTIVE_ROUTE,
	MM_SOUND_MSG_REQ_SET_ACTIVE_ROUTE_AUTO,
	MM_SOUND_MSG_RES_SET_ACTIVE_ROUTE_AUTO,
	MM_SOUND_MSG_REQ_GET_ACTIVE_DEVICE,
	MM_SOUND_MSG_RES_GET_ACTIVE_DEVICE,
	MM_SOUND_MSG_REQ_ADD_ACTIVE_DEVICE_CB,
	MM_SOUND_MSG_RES_ADD_ACTIVE_DEVICE_CB,
	MM_SOUND_MSG_REQ_REMOVE_ACTIVE_DEVICE_CB,
	MM_SOUND_MSG_RES_REMOVE_ACTIVE_DEVICE_CB,
	MM_SOUND_MSG_INF_ACTIVE_DEVICE_CB,
	MM_SOUND_MSG_REQ_ADD_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_RES_ADD_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_REQ_REMOVE_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_RES_REMOVE_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_INF_AVAILABLE_ROUTE_CB,
	MM_SOUND_MSG_REQ_ADD_VOLUME_CB,
	MM_SOUND_MSG_RES_ADD_VOLUME_CB,
	MM_SOUND_MSG_REQ_REMOVE_VOLUME_CB,
	MM_SOUND_MSG_RES_REMOVE_VOLUME_CB,
	MM_SOUND_MSG_INF_VOLUME_CB,
	MM_SOUND_MSG_REQ_SET_PATH_FOR_ACTIVE_DEVICE,
	MM_SOUND_MSG_RES_SET_PATH_FOR_ACTIVE_DEVICE,
	MM_SOUND_MSG_REQ_GET_AUDIO_PATH,
	MM_SOUND_MSG_RES_GET_AUDIO_PATH,
	MM_SOUND_MSG_REQ_ADD_DEVICE_CONNECTED_CB,
	MM_SOUND_MSG_RES_ADD_DEVICE_CONNECTED_CB,
	MM_SOUND_MSG_REQ_REMOVE_DEVICE_CONNECTED_CB,
	MM_SOUND_MSG_RES_REMOVE_DEVICE_CONNECTED_CB,
	MM_SOUND_MSG_INF_DEVICE_CONNECTED_CB,
	MM_SOUND_MSG_REQ_ADD_DEVICE_INFO_CHANGED_CB,
	MM_SOUND_MSG_RES_ADD_DEVICE_INFO_CHANGED_CB,
	MM_SOUND_MSG_REQ_REMOVE_DEVICE_INFO_CHANGED_CB,
	MM_SOUND_MSG_RES_REMOVE_DEVICE_INFO_CHANGED_CB,
	MM_SOUND_MSG_INF_DEVICE_INFO_CHANGED_CB,
	MM_SOUND_MSG_REQ_GET_CONNECTED_DEVICE_LIST,
	MM_SOUND_MSG_RES_GET_CONNECTED_DEVICE_LIST,
#ifdef USE_FOCUS
	MM_SOUND_MSG_REQ_REGISTER_FOCUS,
	MM_SOUND_MSG_RES_REGISTER_FOCUS,
	MM_SOUND_MSG_REQ_UNREGISTER_FOCUS,
	MM_SOUND_MSG_RES_UNREGISTER_FOCUS,
	MM_SOUND_MSG_REQ_ACQUIRE_FOCUS,
	MM_SOUND_MSG_RES_ACQUIRE_FOCUS,
	MM_SOUND_MSG_REQ_RELEASE_FOCUS,
	MM_SOUND_MSG_RES_RELEASE_FOCUS,
	MM_SOUND_MSG_INF_FOCUS_CHANGED_CB,
	MM_SOUND_MSG_REQ_SET_FOCUS_WATCH_CB,
	MM_SOUND_MSG_RES_SET_FOCUS_WATCH_CB,
	MM_SOUND_MSG_REQ_UNSET_FOCUS_WATCH_CB,
	MM_SOUND_MSG_RES_UNSET_FOCUS_WATCH_CB,
	MM_SOUND_MSG_INF_FOCUS_WATCH_CB,
#endif
};


/* TODO : make this general , can be used in other IPC not only dbus */
enum {
        METHOD_CALL_TEST,
        METHOD_CALL_PLAY_FILE_START,
        METHOD_CALL_PLAY_FILE_START_WITH_STREAM_INFO,
        METHOD_CALL_PLAY_FILE_STOP,
        METHOD_CALL_PLAY_DTMF,
        METHOD_CALL_PLAY_DTMF_WITH_STREAM_INFO,
        METHOD_CALL_CLEAR_FOCUS, // Not original focus feature, only for tone/wav player internal focus usage.
        METHOD_CALL_GET_BT_A2DP_STATUS,
        METHOD_CALL_SET_PATH_FOR_ACTIVE_DEVICE,
        METHOD_CALL_GET_CONNECTED_DEVICE_LIST,
        METHOD_CALL_GET_AUDIO_PATH,
        METHOD_CALL_SET_VOLUME_LEVEL,

        METHOD_CALL_REGISTER_FOCUS,
        METHOD_CALL_UNREGISTER_FOCUS,
        METHOD_CALL_ACQUIRE_FOCUS,
        METHOD_CALL_RELEASE_FOCUS,
        METHOD_CALL_WATCH_FOCUS,
        METHOD_CALL_UNWATCH_FOCUS,

        METHOD_CALL_ASM_REGISTER_SOUND,
        METHOD_CALL_ASM_UNREGISTER_SOUND,
        METHOD_CALL_ASM_REGISTER_WATCHER,
        METHOD_CALL_ASM_UNREGISTER_WATCHER,
        METHOD_CALL_ASM_GET_MYSTATE,
        METHOD_CALL_ASM_GET_STATE,
        METHOD_CALL_ASM_SET_STATE,
        METHOD_CALL_ASM_SET_SUBSESSION,
        METHOD_CALL_ASM_GET_SUBSESSION,
        METHOD_CALL_ASM_SET_SUBEVENT,
        METHOD_CALL_ASM_GET_SUBEVENT,
        METHOD_CALL_ASM_SET_SESSION_OPTION,
        METHOD_CALL_ASM_GET_SESSION_OPTION,
        METHOD_CALL_ASM_RESET_RESUME_TAG,
        METHOD_CALL_ASM_DUMP,
        METHOD_CALL_ASM_EMERGENT_EXIT,

        METHOD_CALL_MAX,
};


typedef enum sound_server_signal {
        SIGNAL_TEST,
        SIGNAL_PLAY_FILE_END,
        SIGNAL_VOLUME_CHANGED,
        SIGNAL_DEVICE_CONNECTED,
        SIGNAL_DEVICE_INFO_CHANGED,
        SIGNAL_FOCUS_CHANGED,
        SIGNAL_FOCUS_WATCH,
        SIGNAL_MAX
} sound_server_signal_t;

typedef enum pulseaudio_property {
        PULSEAUDIO_PROP_AUDIO_BALANCE,
        PULSEAUDIO_PROP_MONO_AUDIO,
        PULSEAUDIO_PROP_MUTE_ALL,
        PULSEAUDIO_PROP_MAX
} pulseaudio_property_t;

struct mm_sound_dbus_method_info{
        const char* name;
        /*
        const char* argument;
        const char* reply;
        */
};

struct mm_sound_dbus_signal_info{
        const char* name;
        const char* argument;
};

struct pulseaudio_dbus_property_info {
        const char* name;
};

#define DSIZE sizeof(mm_ipc_msg_t)-sizeof(long)	/* data size for rcv & snd */

#endif /* __MM_SOUND_MSG_H__  */

