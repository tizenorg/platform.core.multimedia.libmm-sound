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

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#include <errno.h>

#include "include/mm_sound_mgr_common.h"
#include "../include/mm_sound_common.h"

#include <mm_error.h>
#include <mm_debug.h>

#include "include/mm_sound_mgr_dock.h"
#include "include/mm_sound_mgr_session.h"

/******************************* Dock Code **********************************/

#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "mm_ipc.h"
#include "mm_sound_common.h"
#include "mm_sound.h"

#include <vconf.h>
#include <vconf-keys.h>

static void dock_changed_cb(keynode_t* node, void* data)
{
	int dock_available = 0;

	/* Get actual vconf value */
	vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &dock_available);

	debug_msg ("[%s] changed callback called, key value is [%d]\n", vconf_keynode_get_name(node), dock_available);

	/* Set device available based on vconf key value */
	MMSoundMgrSessionSetDeviceAvailable (DEVICE_DOCK, dock_available, 0, NULL);
}

int _register_dock_status ()
{
	/* set callback for vconf key change */
	int ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS, dock_changed_cb, NULL);
	debug_msg ("vconf [%s] set ret = %d\n", VCONFKEY_SYSMAN_CRADLE_STATUS, ret);
	return ret;
}


int MMSoundMgrDockInit(void)
{
	debug_enter("\n");

	_register_dock_status ();

	debug_leave("\n");
	return MM_ERROR_NONE;
}

int MMSoundMgrDockFini(void)
{
	debug_enter("\n");

	debug_leave("\n");
	return MM_ERROR_NONE;
}

