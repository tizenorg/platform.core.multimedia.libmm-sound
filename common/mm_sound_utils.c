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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <mm_types.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mm_sound_private.h"
#include "../include/mm_sound.h"
#include "../include/mm_sound_utils.h"

static mm_sound_route g_valid_route[] = { MM_SOUND_ROUTE_OUT_SPEAKER, MM_SOUND_ROUTE_OUT_WIRED_ACCESSORY, MM_SOUND_ROUTE_OUT_BLUETOOTH,
											MM_SOUND_ROUTE_IN_MIC, MM_SOUND_ROUTE_IN_WIRED_ACCESSORY, MM_SOUND_ROUTE_IN_MIC_OUT_RECEIVER,
											MM_SOUND_ROUTE_IN_MIC_OUT_SPEAKER, MM_SOUND_ROUTE_IN_MIC_OUT_HEADPHONE,
											MM_SOUND_ROUTE_INOUT_HEADSET, MM_SOUND_ROUTE_INOUT_BLUETOOTH };

EXPORT_API
int _mm_sound_get_valid_route_list(mm_sound_route **route_list)
{
	*route_list = g_valid_route;

	return (int)(sizeof(g_valid_route) / sizeof(mm_sound_route));
}

EXPORT_API
bool _mm_sound_is_route_valid(mm_sound_route route)
{
	mm_sound_route *route_list = 0;
	int route_index = 0;
	int route_list_count = 0;

	route_list_count = _mm_sound_get_valid_route_list(&route_list);
	for (route_index = 0; route_index < route_list_count; route_index++) {
		if (route_list[route_index] == route)
			return 1;
	}

	return 0;
}

EXPORT_API
void _mm_sound_get_devices_from_route(mm_sound_route route, mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	if (device_in && device_out) {
		*device_in = route & 0x00FF;
		*device_out = route & 0xFF00;
	}
}

EXPORT_API
bool _mm_sound_check_hibernation (const char *path)
{
	int fd = -1;
	if (path == NULL) {
		debug_error ("Path is null\n");
		return false;
	}

	fd = open (path, O_RDONLY | O_CREAT, 0644);
	if (fd != -1) {
		debug_log ("Open [%s] success!!\n", path);
	} else {
		debug_error ("Can't create [%s] with errno [%d]\n", path, errno);
		return false;
	}

	close (fd);
	return true;
}



