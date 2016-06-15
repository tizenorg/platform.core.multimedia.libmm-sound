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
 * @file		mm_sound.h
 * @brief		Application interface library for sound module.
 * @date
 * @version		Release
 *
 * Application interface library for sound module.
 */

#ifndef	__MM_SOUND_PRIVATE_H__
#define	__MM_SOUND_PRIVATE_H__

#include <mm_types.h>
#include <mm_error.h>
#include <mm_sound.h>

#ifdef __cplusplus
	extern "C" {
#endif

/**
  	@internal
	@addtogroup SOUND_PRIVATE
	@{

 */

#define MM_SOUND_VOLUME_CONFIG_TYPE(vol) (vol & 0x00FF)
#define MM_SOUND_VOLUME_CONFIG_GAIN(vol) (vol & 0xFF00)

enum mm_sound_handle_route_t {
	MM_SOUND_HANDLE_ROUTE_USING_CURRENT,
};

typedef struct {
	const char			*filename;		/**< filename to play */
	bool				skip_session;	/**< skip session control */
	int					volume;			/**< relative volume level */
	int					loop;			/**< loop count */
	mm_sound_stop_callback_func	callback;		/**< callback function when playing is terminated */
	void				*data;			/**< user data to callback */
	void				*mem_ptr;		/**< memory buffer to play */
	int					mem_size;		/**< size of memory buffer */
	int					handle_route;	/**< 1 for speaker, 0 for current */
	int					volume_config;	/**< volume type & volume gain */
	int					priority;		/**< 0 or 1 */
} MMSoundPlayParam;

/**
 * This function is to play system sound with specified parameters.
 *
 * @param	param		[in] Reference pointer to MMSoundPlayParam structure
 * @param	handle	[out] Handle of sound play.
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	When the stop callback is set, it will be called when system sound is
 *			terminated. If mm_sound_stop_sound() is called apparently before
 *			system sound is terminated, stop_callback will not be called.
 *			This function can use various sound route path with mm_sound_set_path
 * @see		mm_sound_stop_sound mm_sound_set_path
 * @since		R1, 1.0
 * @limo
 */
int mm_sound_play_sound_ex(MMSoundPlayParam *param, int *handle);

int mm_sound_play_sound_with_stream_info(const char *filename, char *stream_type, int stream_id, unsigned int loop, mm_sound_stop_callback_func _completed_cb, void *data, int *handle);

/**
 * This function is to play key sound.
 *
 * @param	filename		[in] keytone filename to play
 * @param	volume_config	[in] Volume type & volume gain
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 *
 * @remark	This function provide low latency sound play (such as dialer keytone)
 * 			using fixed spec of wave file (44100Hz, mono channel)
 * @see		volume_type_t volume_gain_t mm_sound_volume_set_value
 */
int mm_sound_play_keysound(const char *filename, int volume_config);

int mm_sound_boot_ready(int timeout_sec);

int mm_sound_boot_play_sound(char* path);

/**
	@}
 */

#ifdef __cplusplus
}
#endif

#endif	/* __MM_SOUND_H__ */

