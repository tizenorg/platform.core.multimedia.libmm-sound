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

#include <stdio.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <vconf.h>
#include <avsys-audio.h>

#include "include/mm_sound_common.h"


int sound_system_bootup_recovery()
{
	int err=0;
	char *keystr[] = {VCONF_KEY_VOLUME_TYPE_SYSTEM, VCONF_KEY_VOLUME_TYPE_NOTIFICATION, VCONF_KEY_VOLUME_TYPE_ALARM,
			VCONF_KEY_VOLUME_TYPE_RINGTONE, VCONF_KEY_VOLUME_TYPE_MEDIA, VCONF_KEY_VOLUME_TYPE_CALL,
			VCONF_KEY_VOLUME_TYPE_ANDROID,VCONF_KEY_VOLUME_TYPE_JAVA, VCONF_KEY_VOLUME_TYPE_MEDIA};
	int vol[AVSYS_AUDIO_VOLUME_TYPE_MAX] = {5,7,6,13,7,7,0,11,11}, i=0;

	for(i=0; i<AVSYS_AUDIO_VOLUME_TYPE_MAX; i++) {
		if(vconf_get_int(keystr[i], (int*)&vol[i])) {
			if(vconf_set_int(keystr[i], vol[i])) {
				debug_error("Error on volume vconf key %s\n", keystr[i]);
			} else {
				debug_error("Set %s to default value %d\n", keystr[i], vol[i]);
			}
		} else {
			debug_msg("Volume value of %s is %d\n", keystr[i], vol[i]);
		}
	}

	err = avsys_audio_hibernation_reset(vol);
	return err;
}
