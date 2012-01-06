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

/**
* @ingroup	MMF_SOUND_API
* @addtogroup	SOUND
*/

/**
* @ingroup	SOUND
* @addtogroup	UTS_MMF_SOUND Unit
*/

/**
* @ingroup	UTS_MMF_SOUND Unit
* @addtogroup	UTS_MMF_SOUND_VOLUME_GET_STEP Uts_Mmf_Sound_Volume_Get_Step
* @{
*/

/**
* @file uts_mm_sound_volume_get_step_func.c
* @brief This is a suit of unit test cases to test mm_sound_volume_get_step API
* @author Kwanghui Cho (kwanghui.cho@samsung.com)
* @version Initial Creation Version 0.1
* @date 2010.10.05
*/


#include "utc_mm_sound_common.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////
// Declare the global variables and registers and Internal Funntions
//-------------------------------------------------------------------------------------------------

#define API_NAME "mm_sound_pcm_capture_read"

struct tet_testlist tet_testlist[] = {
	{utc_mm_sound_pcm_capture_read_func_01, 1},
	{utc_mm_sound_pcm_capture_read_func_02, 2},
	{NULL, 0}
};

MMSoundPcmHandle_t g_handle;

///////////////////////////////////////////////////////////////////////////////////////////////////
/* Initialize TCM data structures */

/* Start up function for each test purpose */
void
startup ()
{
	mm_sound_pcm_capture_open(&g_handle, 44100, MMSOUND_PCM_MONO, MMSOUND_PCM_S16_LE);
}

/* Clean up function for each test purpose */
void
cleanup ()
{
	mm_sound_pcm_capture_close(g_handle);
}

void utc_mm_sound_pcm_capture_read_func_01()
{
	int ret = 0;
	char buffer[4096]= {0,};

	ret = mm_sound_pcm_capture_read(g_handle, (void*)buffer, sizeof(buffer));

	dts_check_ge (API_NAME, ret, 0);

	return;
}


void utc_mm_sound_pcm_capture_read_func_02()
{
	int ret = 0;
	char *buffer = NULL;

	ret = mm_sound_pcm_capture_read(g_handle, (void*)buffer, sizeof(buffer));

	dts_check_lt (API_NAME, ret, 0);

	return;
}

/** @} */




