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

#define KEY_BASE_PATH	"/"
#define SHM_NAME "SHM"
#define RCV_MSG	0x21	/* rcv key */
#define SND_MSG 0x24	/* snd key */
#define CB_MSG   0x64		/* cb key */

#define SERVER_RCV_TYPE 100000	/* rcv message type in server */
#define SERVER_CB_TYPE 200000	/* cb message type in server */
#define CLIENT_CB_TYPE 300000	/* cb message type in client */

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
	MM_SOUND_MSG_REQ_IS_BT_A2DP_ON = 20,
	MM_SOUND_MSG_RES_IS_BT_A2DP_ON = 21,
#endif // PULSE_CLIENT
        MM_SOUND_MSG_REQ_DTMF  	= 22,
	MM_SOUND_MSG_RES_DTMF  	= 23,
};

#define DSIZE sizeof(mm_ipc_msg_t)-sizeof(long)	/* data size for rcv & snd */
#define CONVERT_MSG_REQ_RES(A) ((A) + (MM_SOUND_MSG_RES_NODE - MM_SOUND_MSG_REQ_NODE))

#endif /* __MM_SOUND_MSG_H__  */

