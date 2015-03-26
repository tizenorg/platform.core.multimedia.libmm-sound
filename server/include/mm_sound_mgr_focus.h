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

#ifndef __MM_SOUND_MGR_FOCUS_H__
#define __MM_SOUND_MGR_FOCUS_H__

#include "../include/mm_sound.h"
#include "../include/mm_sound_focus.h"
#include "../include/mm_sound_stream.h"
#include "../include/mm_ipc.h"
#include "mm_sound_mgr_ipc.h"

typedef enum
{
	FOCUS_COMMAND_RELEASE,
	FOCUS_COMMAND_ACQUIRE,
} focus_command_e;

typedef enum
{
	FOCUS_TYPE_PLAYBACK = 1,
	FOCUS_TYPE_CAPTURE,
	FOCUS_TYPE_BOTH,
} focus_type_e;

typedef enum
{
	FOCUS_STATUS_DEACTIVATED,
	FOCUS_STATUS_ACTIVATED_PLAYBACK,
	FOCUS_STATUS_ACTIVATED_CAPTURE,
	FOCUS_STATUS_ACTIVATED_BOTH,
} focus_status_e;

typedef struct {
	int pid;
	int handle_id;
	char stream_type[MAX_STREAM_TYPE_LEN];
	char option[MM_SOUND_NAME_NUM];
	focus_type_e request_type;
	void *callback;
	void *cbdata;
} _mm_sound_mgr_focus_param_t;

typedef struct _taken_by_id
{
	int pid;
	int handle_id;
} _focus_taken_by_id_t;

typedef struct {
	int pid;
	int handle_id;
	int priority;
	bool is_for_watch;
	char stream_type[MAX_STREAM_TYPE_LEN];
	focus_status_e status;
	_focus_taken_by_id_t taken_by_id[NUM_OF_STREAM_IO_TYPE];
	void *callback;
	void *cbdata;

#ifdef SUPPORT_CONTAINER
	container_info_t container;
#endif
} focus_node_t;

int MMSoundMgrFocusInit(void);
int MMSoundMgrFocusFini(void);
int mm_sound_mgr_focus_create_node (const _mm_sound_mgr_focus_param_t *param);
int mm_sound_mgr_focus_destroy_node (const _mm_sound_mgr_focus_param_t *param);
int mm_sound_mgr_focus_request_acquire (const _mm_sound_mgr_focus_param_t *param);
int mm_sound_mgr_focus_request_release (const _mm_sound_mgr_focus_param_t *param);
int mm_sound_mgr_focus_set_watch_cb (const _mm_sound_mgr_focus_param_t *param);
int mm_sound_mgr_focus_unset_watch_cb (const _mm_sound_mgr_focus_param_t *param);


#endif /* __MM_SOUND_MGR_FOCUS_H__ */

