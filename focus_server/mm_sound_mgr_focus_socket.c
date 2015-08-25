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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <pthread.h>

#include <errno.h>
#include "include/mm_sound_mgr_focus_ipc.h"

#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
#include "../include/mm_sound_socket.h"
#include "include/mm_sound_mgr_focus.h"
#include "include/mm_sound_mgr_focus_socket.h"
#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>


#if 0
struct mgr_focus_socket_data {
    mm_sound_socket_server *socket_server;
};



int MMSoundMgrFocusSocketInit(mgr_focus_socket_data **_mgr_data)
{
    int ret = MM_ERROR_NONE;
    mgr_focus_socket_data *mgr_data = NULL;

    debug_fenter();

    if (!_mgr_data) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = make_focus_dir(FOCUS_SERVER_PATH)) != MM_ERROR_NONE) {
        debug_error("make focus directory failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((mgr_data = g_malloc0(sizeof(mgr_focus_socket_data))) == NULL) {
        debug_error("Allocate Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = mm_sound_socket_server_new(SOCK_PATH, &mgr_data->socket_server)) != MM_ERROR_NONE) {
        debug_error("Socket Server New Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = mm_sound_socket_server_start(mgr_data->socket_server)) != MM_ERROR_NONE) {
        debug_error("Socket Server Start Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    *_mgr_data = mgr_data;

FINISH:
    debug_fleave();

    return ret;
}

void MMSoundMgrFocusSocketFini(mgr_focus_socket_data *mgr_data)
{
    debug_fenter();

    if (!mgr_data || !mgr_data->socket_server) {
        debug_error("Invalid Parameter");
        goto FINISH;
    }

    mm_sound_socket_server_stop(mgr_data->socket_server);
    mm_sound_socket_server_destroy(mgr_data->socket_server);

FINISH:
    debug_fleave();
}

#endif
