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

#ifndef __MM_SOUND_MGR_IPC_MSGQ_H__
#define __MM_SOUND_MGR_IPC_MSGQ_H__

#include "../../include/mm_sound_msg.h"

#define SOUND_MSG_SET(sound_msg, x_msgtype, x_handle, x_code, x_msgid) \
do { \
	sound_msg.msgtype = x_msgtype; \
	sound_msg.handle = x_handle; \
	sound_msg.code = x_code; \
	sound_msg.msgid = x_msgid; \
} while(0)


int MMSoundMgrIpcMsgqInit(void);
int MMSoundMgrIpcMsgqFini(void);
int MMSoundMgrIpcMsgqReady(void);

int _MMIpcMsgqCBSndMsg(mm_ipc_msg_t *msg);
int _MMIpcMsgqCBRecvMsg(mm_ipc_msg_t *msg);
int _MMIpcMsgqCBMsgEnQueueAgain(mm_ipc_msg_t *msg);

#endif /* __MM_SOUND_MGR_H__ */

