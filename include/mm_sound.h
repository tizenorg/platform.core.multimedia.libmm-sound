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

#ifndef	__MM_SOUND_H__
#define	__MM_SOUND_H__

#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
	@addtogroup SOUND
	@{
	@par
	This part is describes the sound module of multimedia framework. Sound
	module provides APIs to implement play wav file with simple api, to handle volume information,
	to handle audio route policy.

	@par
	There is six different volume type for normal usage. application should set proper volume type to multimedia playback APIs.
	<div> <table>
	<tr>
	<td><B>Type</B></td>
	<td><B>Description</B></td>
	</tr>
	<tr>
	<td>VOLUME_TYPE_SYSTEM</td>
	<td>volume for normal system sound (e.g. keysound, camera shutter)</td>
	</tr>
	<tr>
	<td>VOLUME_TYPE_NOTIFICATION</td>
	<td>volume for notification (e.g. message, email notification)</td>
	</tr>
	<tr>
	<td>VOLUME_TYPE_RINGTONE</td>
	<td>volume for incoming call ring</td>
	</tr>
	<tr>
	<td>VOLUME_TYPE_MEDIA</td>
	<td>volume for media playback (e.g. music, video playback)</td>
	</tr>
	<tr>
	<td>VOLUME_TYPE_CALL</td>
	<td>volume for call</td>
	</tr>
	</table> </div>

	@par
	application can change audio route policy with mm-sound API.
	Audio route is input and output of audio stream.

	@par
	@image html		audio_device.png	"Figure1. Audio Devices of mobile phone"	width=12cm
	@image latex	audio_device.png	"Figure1. Audio Devices of mobile phone"	width=12cm

	@par
	Default audio route policy is like follows
	@par
	for playback
	<div><table>
	<tr>
	<td><B>Bluetooth headset</B></td>
	<td><B>Wired headset</B></td>
	<td><B>Playback Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>Bluetooth headset</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>Bluetooth headset</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>Wired headset</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>Loud speaker</td>
	</tr>
	</table></div>

	@par
	for capture (bluetooth headset mic used only in call mode)
	<div><table>
	<tr>
	<td><B>Bluetooth headset mic</B></td>
	<td><B>Wired headset mic</B></td>
	<td><B>Capture Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>Wired headset mic</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>microphone</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>Wired headset mic</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>Wired headset mic</td>
	</tr>
	</table></div>

	@par
	If application changes routing policy to SYSTEM_AUDIO_ROUTE_POLICY_IGNORE_A2DP with mm_sound_route_set_system_policy
	audio routing policy has changed to ignore bluetooth headset connection.
	@par
	for playback
	<div><table>
	<tr>
	<td><B>Bluetooth headset</B></td>
	<td><B>Wired headset</B></td>
	<td><B>Playback Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>Wired headset</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>Loud speaker</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>Wired headset</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>Loud speaker</td>
	</tr>
	</table></div>

	@par
	for capture (bluetooth headset mic used only in call mode)
	<div><table>
	<tr>
	<td><B>Bluetooth headset mic</B></td>
	<td><B>Wired headset mic</B></td>
	<td><B>Capture Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>Wired headset mic</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>microphone</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>Wired headset mic</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>Wired headset mic</td>
	</tr>
	</table></div>

	@par
	If application changes routing policy to SYSTEM_AUDIO_ROUTE_POLICY_HANDSET_ONLY with mm_sound_route_set_system_policy
	audio routing policy has changed to use only loud speaker and microphone.
	@par
	for playback
	<div><table>
	<tr>
	<td><B>Bluetooth headset</B></td>
	<td><B>Wired headset</B></td>
	<td><B>Playback Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>Loud speaker</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>Loud speaker</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>Loud speaker</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>Loud speaker</td>
	</tr>
	</table></div>

	@par
	for capture (bluetooth headset mic used only in call mode)
	<div><table>
	<tr>
	<td><B>Bluetooth headset mic</B></td>
	<td><B>Wired headset mic</B></td>
	<td><B>Capture Device</B></td>
	</tr>
	<tr>
	<td>connected</td>
	<td>plugged</td>
	<td>microphone</td>
	</tr>
	<tr>
	<td>connected</td>
	<td>unplugged</td>
	<td>microphone</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>plugged</td>
	<td>microphone</td>
	</tr>
	<tr>
	<td>disconnected</td>
	<td>unplugged</td>
	<td>microphone</td>
	</tr>
	</table></div>

 */
#define MM_SOUND_STREAM_TYPE_LEN 64

/*
 * MMSound Volume APIs
 */

/**
 * Enumerations of Volume type.
 */

	typedef enum {
		VOLUME_TYPE_SYSTEM,			/**< System volume type */
		VOLUME_TYPE_NOTIFICATION,	/**< Notification volume type */
		VOLUME_TYPE_ALARM,			/**< Alarm volume type */
		VOLUME_TYPE_RINGTONE,		/**< Ringtone volume type */
		VOLUME_TYPE_MEDIA,			/**< Media volume type */
		VOLUME_TYPE_CALL,			/**< Call volume type */
		VOLUME_TYPE_VOIP,			/**< VOIP volume type */
		VOLUME_TYPE_VOICE,			/**< VOICE volume type */
		VOLUME_TYPE_FIXED,			/**< Volume type for fixed acoustic level */
		VOLUME_TYPE_MAX,			/**< Volume type count */
		VOLUME_TYPE_UNKNOWN = -1,
								/**< volume type is not determined */
	} volume_type_t;

	typedef enum {
		VOLUME_GAIN_DEFAULT = 0,
		VOLUME_GAIN_DIALER = 1 << 8,
		VOLUME_GAIN_TOUCH = 2 << 8,
		VOLUME_GAIN_AF = 3 << 8,
		VOLUME_GAIN_SHUTTER1 = 4 << 8,
		VOLUME_GAIN_SHUTTER2 = 5 << 8,
		VOLUME_GAIN_CAMCORDING = 6 << 8,
		VOLUME_GAIN_MIDI = 7 << 8,
		VOLUME_GAIN_BOOTING = 8 << 8,
		VOLUME_GAIN_VIDEO = 9 << 8,
		VOLUME_GAIN_TTS = 10 << 8,
	} volume_gain_t;

/**
 * @brief Enumerations of supporting source_type
 */
	typedef enum {
		SUPPORT_SOURCE_TYPE_DEFAULT,
		SUPPORT_SOURCE_TYPE_MIRRORING,
		SUPPORT_SOURCE_TYPE_VOICECONTROL,
		SUPPORT_SOURCE_TYPE_SVR,
		SUPPORT_SOURCE_TYPE_VIDEOCALL,
		SUPPORT_SOURCE_TYPE_VOICERECORDING,
		SUPPORT_SOURCE_TYPE_VOIP,	/* Supporting VoIP source */
		SUPPORT_SOURCE_TYPE_CALL_FORWARDING,
		SUPPORT_SOURCE_TYPE_FMRADIO,
		SUPPORT_SOURCE_TYPE_LOOPBACK,
	} mm_sound_source_type_e;

/**
 * Active volume change callback function type.
 *
 * @param	type			[in]	The sound type of changed volume
 * @param	volume			[in]	The new volume value
 * @param	user_data		[in]	Argument passed when callback has called
 *
 * @return	No return value
 * @remark	None.
 * @see		mm_sound_add_volume_changed_callback mm_sound_remove_volume_changed_callback
 */
	typedef void (*mm_sound_volume_changed_cb) (volume_type_t type, unsigned int volume, void *user_data);

	int mm_sound_add_volume_changed_callback(mm_sound_volume_changed_cb func, void *user_data, unsigned int *subs_id);

/**
 * This function is to remove volume changed callback.
 *
 * @param	type			[in]	volume type to set change callback function
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	None.
 * @pre		Callback function should be registered previously for given volume type.
 * @post	Callback function will not be called anymore.
 * @see		volume_type_t
 * @par Example
 * @code
void _volume_callback(void *data)
{
	printf("Callback function\n");
}

int volume_callback()
{
	int ret = 0;
	int vol_type = VOLUME_TYPE_MEDIA;

	mm_sound_volume_add_callback(vol_type, _volume_callback, NULL);

	ret = mm_sound_volume_remove_callback(vol_type);
	if ( MM_ERROR_NONE == ret)
	{
		printf("Remove callback success\n");
	}
	else
	{
		printf("Remove callback failed\n");
	}

	return ret;
}

 * @endcode
 */
	int mm_sound_volume_remove_callback(volume_type_t type);

/**
 * This function is to remove volume change callback.
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 * 			with error code.
 **/
	int mm_sound_remove_volume_changed_callback(unsigned int subs_id);

/**
 * This function is to set volume level of certain volume type.
 *
 * @param	type			[in]	volume type to set value.
 * @param	value			[in]	volume value.
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	value should be 0 ~ MAX
 * @see		mm_sound_volume_get_value volume_type_t
 * @pre		None.
 * @post	Volume value will be changed to given value for given volume type.
 * @par Example
 */
	int mm_sound_volume_set_value(volume_type_t type, const unsigned int volume_level);

/**
 * This function is to get volume level of certain volume type.
 *
 * @param	type			[in]	volume type to get value.
 * @param	value			[out]	volume value.
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	None.
 * @pre		None.
 * @post	None.
 * @see		volume_type_t mm_sound_volume_set_value
 * @par Example
 * @code
int value = 0;
int ret = 0;

ret = mm_sound_volume_get_value(VOLUME_TYPE_SYSTEM, &value);
if(ret < 0)
{
	printf("Can not get volume\n");
}
else
{
	printf("System type volume is %d\n", value);
}
 * @endcode
 * @see		mm_sound_volume_set_value
 */
	int mm_sound_volume_get_value(volume_type_t type, unsigned int *value);

/**
 * This function is to set primary volume type.
 *
 * @param	type			[in]	volume type to set as primary volume type.
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Application should use this function during foreground.
 * @pre		None.
 * @post	Volume app. will be update given volume type when H/W volume control key pressed.
 * @see		volume_type_t
 * @par Example
 * @code
static int _resume(void *data)
{
	int ret = 0;

	ret = mm_sound_volume_primary_type_set(VOLUME_TYPE_MEDIA);
	if(ret < 0)
	{
		printf("Can not set primary volume type\n");
	}
	...
}

static int _pause(void* data)
{
	int ret = 0;

	ret = mm_sound_volume_primary_type_set(VOLUME_TYPE_UNKNOWN);
	if(ret < 0)
	{
		printf("Can not clear primary volume type\n");
	}
	...
}

int main()
{
	...
	struct appcore_ops ops = {
		.create = _create,
		.terminate = _terminate,
		.pause = _pause,
		.resume = _resume,
		.reset = _reset,
	};
	...
	return appcore_efl_main(PACKAGE, ..., &ops);
}
 * @endcode
 */
	int mm_sound_volume_primary_type_set(volume_type_t type);
	int mm_sound_volume_primary_type_get(volume_type_t * type);

/**
 * Terminate callback function type.
 *
 * @param	data		[in]	Argument passed when callback was set
 * @param	id	    	[in]	handle which has completed playing
 *
 * @return	No return value
 * @remark	It is not allowed to call MMSound API recursively or do time-consuming
 *			task in this callback because this callback is called synchronously.
 * @see		mm_sound_play_sound
 */
	typedef void (*mm_sound_stop_callback_func) (void *data, int id);

/*
 * MMSound Play APIs
 */

/**
 * This function is to play system sound.
 *
 * @param	filename		[in] Sound filename to play
 * @param	volume config	[in] Volume type & volume gain
 * @param	callback		[in] Callback function pointer when playing is terminated.
 * @param	data			[in] Pointer to user data when callback is called.
 * @param	handle			[out] Handle of sound play.
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	When the stop callback is set, it will be called when system sound is
 *			terminated. If mm_sound_stop_sound() is called apparently before
 *			system sound is terminated, stop_callback will not be called.
 * @see		mm_sound_stop_sound mm_sound_stop_callback_func volume_type_t volume_gain_t
 * @pre		None.
 * @post	Sound will be generated with given filename.
 * @par Example
 * @code
int g_stop=0;
void _stop_callback(void* data)
{
	printf("Stop callback\n");
	g_stop = 1;
}

int play_file()
{
	char filename[] ="/opt/media/Sound/testfile.wav";
	volume_type_t volume = VOLUME_TYPE_SYSTEM;
	int ret = 0;
	int handle = -1;

	ret = mm_sound_play_sound(filename, volume, _stop_callback, NULL, &handle);
	if(ret < 0)
	{
		printf("play file failed\n");
	}
	else
	{
		printf("play file success\n");
	}
	while(g_stop == 0)
	{
		sleep(1);
	}
	printf("play stopped\n");
	return 0;
}
 * @endcode
 */
	int mm_sound_play_sound(const char *filename, int volume_config,
							mm_sound_stop_callback_func callback, void *data, int *handle);

/**
 * This function is to play system sound. And other audio stream will be mute during playing time
 *
 * @param	filename		[in] Sound filename to play
 * @param	volume config	[in] Volume type & volume gain
 * @param	callback		[in] Callback function pointer when playing is terminated.
 * @param	data			[in] Pointer to user data when callback is called.
 * @param	handle			[out] Handle of sound play.
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is almost same with mm_sound_play_sound,
 * 			but this make other audio playback stream to mute during playing time.
 * @see		mm_sound_stop_sound mm_sound_stop_callback_func volume_type_t volume_gain_t
 * @pre		None.
 * @post	Sound will be generated with given filename.
 * @par Example
 * @code
int g_stop=0;
void _stop_callback(void* data)
{
	printf("Stop callback\n");
	g_stop = 1;
}

int play_file()
{
	char filename[] ="/opt/media/Sound/testfile.wav";
	volume_type_t volume = VOLUME_TYPE_SYSTEM;
	int ret = 0;
	int handle = -1;

	ret = mm_sound_play_loud_solo_sound(filename, volume, _stop_callback, NULL, &handle);
	if(ret < 0)
	{
		printf("play file failed\n");
	}
	else
	{
		printf("play file success\n");
	}
	while(g_stop == 0)
	{
		sleep(1);
	}
	printf("play stopped\n");
	return 0;
}
 * @endcode
 */
	int mm_sound_play_loud_solo_sound(const char *filename, int volume_config,
									  mm_sound_stop_callback_func callback, void *data, int *handle);

/**
 * This function is to stop playing system sound.
 *
 * @param	handle	[in] Handle of mm_sound_play_sound
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 *
 * @remark	When system sound is terminated with this function call, it does not
 *			call stop callback which was set when start playing system sound.
 * @see		mm_sound_play_sound
 * @pre		An sound play handle should be valid.
 * @post	Playing sound file will be stopped.
 * @par Example
 * @code
int g_stop=0;
void _stop_callback(void* data)
{
	printf("Stop callback\n");
	g_stop = 1;
}

int play_file_one_second()
{
	char filename[] ="/opt/media/Sound/testfile.wav";
	volume_type_t volume = VOLUME_TYPE_SYSTEM;
	int ret = 0;
	int handle = -1;

	ret = mm_sound_play_sound(filename, volume, _stop_callback, NULL, &handle);
	if(ret < 0)
	{
		printf("play file failed\n");
	}
	else
	{
		printf("play file success\n");
	}

	sleep(1); //wait 1 second

	ret = mm_sound_stop_sound(handle);
	if(ret < 0)
	{
		printf("stop failed\n");
	}
	else
	{
		printf("play stopped\n");
	}
	return 0;
}
 * @endcode
 */
	int mm_sound_stop_sound(int handle);

/**
 * Enumerations for TONE
 */

	typedef enum {
		MM_SOUND_TONE_DTMF_0 = 0,		/**< Predefined DTMF 0 */
		MM_SOUND_TONE_DTMF_1,			/**< Predefined DTMF 1 */
		MM_SOUND_TONE_DTMF_2,			/**< Predefined DTMF 2 */
		MM_SOUND_TONE_DTMF_3,			/**< Predefined DTMF 3 */
		MM_SOUND_TONE_DTMF_4,			/**< Predefined DTMF 4 */
		MM_SOUND_TONE_DTMF_5,			/**< Predefined DTMF 5 */
		MM_SOUND_TONE_DTMF_6,			/**< Predefined DTMF 6 */
		MM_SOUND_TONE_DTMF_7,			/**< Predefined DTMF 7 */
		MM_SOUND_TONE_DTMF_8,			/**< Predefined DTMF 8 */
		MM_SOUND_TONE_DTMF_9,			/**< Predefined DTMF 9 */
		MM_SOUND_TONE_DTMF_S,				/**< Predefined DTMF Star - Asterisk (*) */
		MM_SOUND_TONE_DTMF_P,			/**< Predefined DTMF sharP (#) */
		MM_SOUND_TONE_DTMF_A,			/**< Predefined DTMF A (A) */
		MM_SOUND_TONE_DTMF_B,			/**< Predefined DTMF B (B) */
		MM_SOUND_TONE_DTMF_C,			/**< Predefined DTMF C (C) */
		MM_SOUND_TONE_DTMF_D,			/**< Predefined DTMF D (D) */

	/**< Pre-defined TONE */
		MM_SOUND_TONE_SUP_DIAL,				/**Call supervisory tone, Dial tone: CEPT: 425Hz, continuous */
		MM_SOUND_TONE_ANSI_DIAL,			/**Call supervisory tone, Dial tone: ANSI (IS-95): 350Hz+440Hz, continuous */
		MM_SOUND_TONE_JAPAN_DIAL,			/**Call supervisory tone, Dial tone: JAPAN: 400Hz, continuous*/
		MM_SOUND_TONE_SUP_BUSY,			/**Call supervisory tone, Busy: CEPT: 425Hz, 500ms ON, 500ms OFF... */
		MM_SOUND_TONE_ANSI_BUSY,			/**Call supervisory tone, Busy: ANSI (IS-95): 480Hz+620Hz, 500ms ON, 500ms OFF... */
		MM_SOUND_TONE_JAPAN_BUSY,			/**Call supervisory tone, Busy: JAPAN: 400Hz, 500ms ON, 500ms OFF...*/
		MM_SOUND_TONE_SUP_CONGESTION,	/**Call supervisory tone, Congestion: CEPT, JAPAN: 425Hz, 200ms ON, 200ms OFF */
		MM_SOUND_TONE_ANSI_CONGESTION,	/**Call supervisory tone, Congestion: ANSI (IS-95): 480Hz+620Hz, 250ms ON, 250ms OFF... */
		MM_SOUND_TONE_SUP_RADIO_ACK,		/**Call supervisory tone, Radio path acknowlegment : CEPT, ANSI: 425Hz, 200ms ON  */
		MM_SOUND_TONE_JAPAN_RADIO_ACK,	/**Call supervisory tone, Radio path acknowlegment : JAPAN: 400Hz, 1s ON, 2s OFF...*/
		MM_SOUND_TONE_SUP_RADIO_NOTAVAIL,	/**Call supervisory tone, Radio path not available: 425Hz, 200ms ON, 200 OFF 3 bursts */
		MM_SOUND_TONE_SUP_ERROR,			/**Call supervisory tone, Error/Special info: 950Hz+1400Hz+1800Hz, 330ms ON, 1s OFF... */
		MM_SOUND_TONE_SUP_CALL_WAITING,	/**Call supervisory tone, Call Waiting: CEPT, JAPAN: 425Hz, 200ms ON, 600ms OFF, 200ms ON, 3s OFF...  */
		MM_SOUND_TONE_ANSI_CALL_WAITING,	/**Call supervisory tone, Call Waiting: ANSI (IS-95): 440 Hz, 300 ms ON, 9.7 s OFF, (100 ms ON, 100 ms OFF, 100 ms ON, 9.7s OFF ...) */
		MM_SOUND_TONE_SUP_RINGTONE,		/**Call supervisory tone, Ring Tone: CEPT, JAPAN: 425Hz, 1s ON, 4s OFF... */
		MM_SOUND_TONE_ANSI_RINGTONE,		/**Call supervisory tone, Ring Tone: ANSI (IS-95): 440Hz + 480Hz, 2s ON, 4s OFF... */
		MM_SOUND_TONE_PROP_BEEP,			/**General beep: 400Hz+1200Hz, 35ms ON */
		MM_SOUND_TONE_PROP_ACK,				/**Proprietary tone, positive acknowlegement: 1200Hz, 100ms ON, 100ms OFF 2 bursts */
		MM_SOUND_TONE_PROP_NACK,			/**Proprietary tone, negative acknowlegement: 300Hz+400Hz+500Hz, 400ms ON */
		MM_SOUND_TONE_PROP_PROMPT,		/**Proprietary tone, prompt tone: 400Hz+1200Hz, 200ms ON	 */
		MM_SOUND_TONE_PROP_BEEP2,			/**Proprietary tone, general double beep: twice 400Hz+1200Hz, 35ms ON, 200ms OFF, 35ms ON */
		MM_SOUND_TONE_SUP_INTERCEPT,					/**Call supervisory tone (IS-95), intercept tone: alternating 440 Hz and 620 Hz tones, each on for 250 ms */
		MM_SOUND_TONE_SUP_INTERCEPT_ABBREV,			/**Call supervisory tone (IS-95), abbreviated intercept: intercept tone limited to 4 seconds */
		MM_SOUND_TONE_SUP_CONGESTION_ABBREV,			/**Call supervisory tone (IS-95), abbreviated congestion: congestion tone limited to 4 seconds */
		MM_SOUND_TONE_SUP_CONFIRM,					/**Call supervisory tone (IS-95), confirm tone: a 350 Hz tone added to a 440 Hz tone repeated 3 times in a 100 ms on, 100 ms off cycle */
		MM_SOUND_TONE_SUP_PIP,						/**Call supervisory tone (IS-95), pip tone: four bursts of 480 Hz tone (0.1 s on, 0.1 s off). */
		MM_SOUND_TONE_CDMA_DIAL_TONE_LITE,			/**425Hz continuous */
		MM_SOUND_TONE_CDMA_NETWORK_USA_RINGBACK,	/**CDMA USA Ringback: 440Hz+480Hz 2s ON, 4000 OFF ...*/
		MM_SOUND_TONE_CDMA_INTERCEPT,				/**CDMA Intercept tone: 440Hz 250ms ON, 620Hz 250ms ON ...*/
		MM_SOUND_TONE_CDMA_ABBR_INTERCEPT,			/**CDMA Abbr Intercept tone: 440Hz 250ms ON, 620Hz 250ms ON */
		MM_SOUND_TONE_CDMA_REORDER,						/**CDMA Reorder tone: 480Hz+620Hz 250ms ON, 250ms OFF... */
		MM_SOUND_TONE_CDMA_ABBR_REORDER,			/**CDMA Abbr Reorder tone: 480Hz+620Hz 250ms ON, 250ms OFF repeated for 8 times */
		MM_SOUND_TONE_CDMA_NETWORK_BUSY,			/**CDMA Network Busy tone: 480Hz+620Hz 500ms ON, 500ms OFF continuous */
		MM_SOUND_TONE_CDMA_CONFIRM,						/**CDMA Confirm tone: 350Hz+440Hz 100ms ON, 100ms OFF repeated for 3 times */
		MM_SOUND_TONE_CDMA_ANSWER,					/**CDMA answer tone: silent tone - defintion Frequency 0, 0ms ON, 0ms OFF */
		MM_SOUND_TONE_CDMA_NETWORK_CALLWAITING,			/**CDMA Network Callwaiting tone: 440Hz 300ms ON */
		MM_SOUND_TONE_CDMA_PIP,							/**CDMA PIP tone: 480Hz 100ms ON, 100ms OFF repeated for 4 times */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_NORMAL,		/**ISDN Call Signal Normal tone: {2091Hz 32ms ON, 2556 64ms ON} 20 times, 2091 32ms ON, 2556 48ms ON, 4s OFF */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_INTERGROUP,	/**ISDN Call Signal Intergroup tone: {2091Hz 32ms ON, 2556 64ms ON} 8 times, 2091Hz 32ms ON, 400ms OFF, {2091Hz 32ms ON, 2556Hz 64ms ON} 8times, 2091Hz 32ms ON, 4s OFF.*/
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_SP_PRI,		/**ISDN Call Signal SP PRI tone:{2091Hz 32ms ON, 2556 64ms ON} 4 times 2091Hz 16ms ON, 200ms OFF, {2091Hz 32ms ON, 2556Hz 64ms ON} 4 times, 2091Hz 16ms ON, 200ms OFF */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_PAT3,	/**SDN Call sign PAT3 tone: silent tone */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_PING_RING,
													/**ISDN Ping Ring tone: {2091Hz 32ms ON, 2556Hz 64ms ON} 5 times 2091Hz 20ms ON */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_PAT5,	/**ISDN Pat5 tone: silent tone */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_PAT6,	/**ISDN Pat6 tone: silent tone */
		MM_SOUND_TONE_CDMA_CALL_SIGNAL_ISDN_PAT7,	/**ISDN Pat7 tone: silent tone */
		MM_SOUND_TONE_CDMA_HIGH_L,					/**TONE_CDMA_HIGH_L tone: {3700Hz 25ms, 4000Hz 25ms} 40 times 4000ms OFF, Repeat .... */
		MM_SOUND_TONE_CDMA_MED_L,					/**TONE_CDMA_MED_L tone: {2600Hz 25ms, 2900Hz 25ms} 40 times 4000ms OFF, Repeat .... */
		MM_SOUND_TONE_CDMA_LOW_L,					/**TONE_CDMA_LOW_L tone: {1300Hz 25ms, 1450Hz 25ms} 40 times, 4000ms OFF, Repeat .... */
		MM_SOUND_TONE_CDMA_HIGH_SS,						/**CDMA HIGH SS tone: {3700Hz 25ms, 4000Hz 25ms} repeat 16 times, 400ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_MED_SS,					/**CDMA MED SS tone: {2600Hz 25ms, 2900Hz 25ms} repeat 16 times, 400ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_LOW_SS,					/**CDMA LOW SS tone: {1300z 25ms, 1450Hz 25ms} repeat 16 times, 400ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_HIGH_SSL,					/**CDMA HIGH SSL tone: {3700Hz 25ms, 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} repeat 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} repeat 16 times, 4000ms OFF, repeat ... */
		MM_SOUND_TONE_CDMA_MED_SSL,						/**CDMA MED SSL tone: {2600Hz 25ms, 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} repeat 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} repeat 16 times, 4000ms OFF, repeat ... */
		MM_SOUND_TONE_CDMA_LOW_SSL,						/**CDMA LOW SSL tone: {1300Hz 25ms, 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} repeat 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} repeat 16 times, 4000ms OFF, repeat ... */
		MM_SOUND_TONE_CDMA_HIGH_SS_2,				/**CDMA HIGH SS2 tone: {3700Hz 25ms, 4000Hz 25ms} 20 times, 1000ms OFF, {3700Hz 25ms, 4000Hz 25ms} 20 times, 3000ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_MED_SS_2,					/**CDMA MED SS2 tone: {2600Hz 25ms, 2900Hz 25ms} 20 times, 1000ms OFF, {2600Hz 25ms, 2900Hz 25ms} 20 times, 3000ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_LOW_SS_2,					/**CDMA LOW SS2 tone: {1300Hz 25ms, 1450Hz 25ms} 20 times, 1000ms OFF, {1300Hz 25ms, 1450Hz 25ms} 20 times, 3000ms OFF, repeat .... */
		MM_SOUND_TONE_CDMA_HIGH_SLS,					/**CDMA HIGH SLS tone: {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 20 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 3000ms OFF, REPEAT */
		MM_SOUND_TONE_CDMA_MED_SLS,						/**CDMA MED SLS tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 20 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 3000ms OFF, REPEAT */
		MM_SOUND_TONE_CDMA_LOW_SLS,						/**CDMA LOW SLS tone: {1300Hz 25ms, 1450Hz 25ms} 10 times, 500ms OFF, {1300Hz 25ms, 1450Hz 25ms} 20 times, 500ms OFF, {1300Hz 25ms, 1450Hz 25ms} 10 times, 3000ms OFF, REPEAT */
		MM_SOUND_TONE_CDMA_HIGH_S_X4,				/**CDMA HIGH S X4 tone: {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 2500ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_MED_S_X4,					/**CDMA MED S X4 tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 2500ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_LOW_S_X4,					/**CDMA LOW S X4 tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 2500ms OFF, REPEAT....*/
		MM_SOUND_TONE_CDMA_HIGH_PBX_L,				/**CDMA HIGH PBX L: {3700Hz 25ms, 4000Hz 25ms}20 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_MED_PBX_L,				/**CDMA MED PBX L: {2600Hz 25ms, 2900Hz 25ms}20 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_LOW_PBX_L,				/**CDMA LOW PBX L: {1300Hz 25ms,1450Hz 25ms}20 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_HIGH_PBX_SS,					/**CDMA HIGH PBX SS tone: {3700Hz 25ms, 4000Hz 25ms} 8 times 200 ms OFF, {3700Hz 25ms 4000Hz 25ms}8 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_MED_PBX_SS,				/**CDMA MED PBX SS tone: {2600Hz 25ms, 2900Hz 25ms} 8 times 200 ms OFF, {2600Hz 25ms 2900Hz 25ms}8 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_LOW_PBX_SS,				/**CDMA LOW PBX SS tone: {1300Hz 25ms, 1450Hz 25ms} 8 times 200 ms OFF, {1300Hz 25ms 1450Hz 25ms}8 times, 2000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_HIGH_PBX_SSL,				/**CDMA HIGH PBX SSL tone:{3700Hz 25ms, 4000Hz 25ms} 8 times 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 16 times, 1000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_MED_PBX_SSL,					/**CDMA MED PBX SSL tone:{2600Hz 25ms, 2900Hz 25ms} 8 times 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 16 times, 1000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_LOW_PBX_SSL,					/**CDMA LOW PBX SSL tone:{1300Hz 25ms, 1450Hz 25ms} 8 times 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 16 times, 1000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_HIGH_PBX_SLS,				/**CDMA HIGH PBX SLS tone:{3700Hz 25ms, 4000Hz 25ms} 8 times 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 16 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 8 times, 1000ms OFF, REPEAT....  */
		MM_SOUND_TONE_CDMA_MED_PBX_SLS,					/**CDMA MED PBX SLS tone:{2600Hz 25ms, 2900Hz 25ms} 8 times 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 16 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 8 times, 1000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_LOW_PBX_SLS,					/**CDMA LOW PBX SLS tone:{1300Hz 25ms, 1450Hz 25ms} 8 times 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 16 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 8 times, 1000ms OFF, REPEAT.... */
		MM_SOUND_TONE_CDMA_HIGH_PBX_S_X4,			/**CDMA HIGH PBX X S4 tone: {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 800ms OFF, REPEAT... */
		MM_SOUND_TONE_CDMA_MED_PBX_S_X4,				/**CDMA MED PBX X S4 tone: {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 800ms OFF, REPEAT... */
		MM_SOUND_TONE_CDMA_LOW_PBX_S_X4,				/**CDMA LOW PBX X S4 tone: {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 800ms OFF, REPEAT... */
		MM_SOUND_TONE_CDMA_ALERT_NETWORK_LITE,		/**CDMA Alert Network Lite tone: 1109Hz 62ms ON, 784Hz 62ms ON, 740Hz 62ms ON 622Hz 62ms ON, 1109Hz 62ms ON */
		MM_SOUND_TONE_CDMA_ALERT_AUTOREDIAL_LITE,	/**CDMA Alert Auto Redial tone: {1245Hz 62ms ON, 659Hz 62ms ON} 3 times, 1245 62ms ON */
		MM_SOUND_TONE_CDMA_ONE_MIN_BEEP,				/**CDMA One Min Beep tone: 1150Hz+770Hz 400ms ON */
		MM_SOUND_TONE_CDMA_KEYPAD_VOLUME_KEY_LITE,	/**CDMA KEYPAD Volume key lite tone: 941Hz+1477Hz 120ms ON */
		MM_SOUND_TONE_CDMA_PRESSHOLDKEY_LITE,		/**CDMA PRESSHOLDKEY LITE tone: 587Hz 375ms ON, 1175Hz 125ms ON */
		MM_SOUND_TONE_CDMA_ALERT_INCALL_LITE,			/**CDMA ALERT INCALL LITE tone: 587Hz 62ms, 784 62ms, 831Hz 62ms, 784Hz 62ms, 1109 62ms, 784Hz 62ms, 831Hz 62ms, 784Hz 62ms*/
		MM_SOUND_TONE_CDMA_EMERGENCY_RINGBACK,		/**CDMA EMERGENCY RINGBACK tone: {941Hz 125ms ON, 10ms OFF} 3times 4990ms OFF, REPEAT... */
		MM_SOUND_TONE_CDMA_ALERT_CALL_GUARD,		/**CDMA ALERT CALL GUARD tone: {1319Hz 125ms ON, 125ms OFF} 3 times */
		MM_SOUND_TONE_CDMA_SOFT_ERROR_LITE,				/**CDMA SOFT ERROR LITE tone: 1047Hz 125ms ON, 370Hz 125ms */
		MM_SOUND_TONE_CDMA_CALLDROP_LITE,			/**CDMA CALLDROP LITE tone: 1480Hz 125ms, 1397Hz 125ms, 784Hz 125ms */
		MM_SOUND_TONE_CDMA_NETWORK_BUSY_ONE_SHOT,	/**CDMA_NETWORK_BUSY_ONE_SHOT tone: 425Hz 500ms ON, 500ms OFF. */
		MM_SOUND_TONE_CDMA_ABBR_ALERT,				/**CDMA_ABBR_ALERT tone: 1150Hz+770Hz 400ms ON */
		MM_SOUND_TONE_CDMA_SIGNAL_OFF,				/**CDMA_SIGNAL_OFF - silent tone */
		MM_SOUND_TONE_LOW_FRE,				/**100Hz continuous */
		MM_SOUND_TONE_MED_FRE,				/**200Hz continuous */
		MM_SOUND_TONE_HIGH_FRE,				/**300Hz continuous */
		MM_SOUND_TONE_NUM,
	} MMSoundTone_t;

	typedef unsigned long sound_time_msec_t;	/**< millisecond unit */

/**
 * This function is to play tone sound.
 *
 * @param	num				[in] predefined tone type (MMSoundTone_t)
 * 			volume config	[in] volume type & volume gain
 *			volume			[in] volume ratio (0.0 ~1.0)
 * 			duration		[in] millisecond (-1 for infinite)
 *			handle			[in] Handle of mm_sound_play_tone
 *			enable_session	[in] set enable/unable session
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 *
 * @remark	It doesn't provide stop
 * @see	volume_type_t volume_gain_t MMSoundTone_t
 * @pre		None.
 * @post	TONE sound will be played.
 * @par Example
 * @code
int ret = 0;

ret = mm_sound_play_tone_ex(MM_SOUND_TONE_DTMF_9, VOLUME_TYPE_SYSTEM, 1.0, 1000, &handle, TRUE); //play 1 second with volume ratio 1.0
if(ret < 0)
{
	printf("play tone failed\n");
}
else
{
	printf("play tone success\n");
}
 * @endcode
 */
	int mm_sound_play_tone_ex(MMSoundTone_t num, int volume_config,
							  const double volume, const int duration, int *handle, bool enable_session);

/**
 * This function is to play tone sound.
 *
 * @param	num				[in] predefined tone type (MMSoundTone_t)
 * 			volume config	[in] volume type & volume gain
 *			volume			[in] volume ratio (0.0 ~1.0)
 * 			duration		[in] millisecond (-1 for infinite)
 *			handle			[in] Handle of mm_sound_play_tone
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 *
 * @remark	It doesn't provide stop
 * @see	volume_type_t volume_gain_t MMSoundTone_t
 * @pre		None.
 * @post	TONE sound will be played.
 * @par Example
 * @code
int ret = 0;

ret = mm_sound_play_tone(MM_SOUND_TONE_DTMF_9, VOLUME_TYPE_SYSTEM, 1.0, 1000, &handle); //play 1 second with volume ratio 1.0
if(ret < 0)
{
	printf("play tone failed\n");
}
else
{
	printf("play tone success\n");
}
 * @endcode
 */
	int mm_sound_play_tone(MMSoundTone_t num, int volume_config, const double volume, const int duration, int *handle);

	int mm_sound_play_tone_with_stream_info(MMSoundTone_t tone,
											char *stream_type, int stream_id,
											const double volume, const int duration, int *handle);

/*
 * Enumerations of System audio route policy
 */

/*
 * Enumerations of device & route
 */

	typedef enum {
		MM_SOUND_DIRECTION_NONE,
		MM_SOUND_DIRECTION_IN,						/**< Capture */
		MM_SOUND_DIRECTION_OUT,						/**< Playback */
	} mm_sound_direction;

	typedef enum {
		MM_SOUND_DEVICE_IN_NONE = 0x00,
		MM_SOUND_DEVICE_IN_MIC = 0x01,				/**< Device builtin mic. */
		MM_SOUND_DEVICE_IN_WIRED_ACCESSORY = 0x02,	/**< Wired input devices */
		MM_SOUND_DEVICE_IN_BT_SCO = 0x08,	/**< Bluetooth SCO device */
	} mm_sound_device_in;

	typedef enum {
		MM_SOUND_DEVICE_OUT_NONE = 0x000,
		MM_SOUND_DEVICE_OUT_SPEAKER = 0x001 << 8,
												/**< Device builtin speaker */
		MM_SOUND_DEVICE_OUT_RECEIVER = 0x002 << 8,	/**< Device builtin receiver */
		MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY = 0x004 << 8,
													/**< Wired output devices such as headphone, headset, and so on. */
		MM_SOUND_DEVICE_OUT_BT_SCO = 0x008 << 8,	/**< Bluetooth SCO device */
		MM_SOUND_DEVICE_OUT_BT_A2DP = 0x010 << 8,
												/**< Bluetooth A2DP device */
		MM_SOUND_DEVICE_OUT_DOCK = 0x020 << 8,		/**< DOCK device */
		MM_SOUND_DEVICE_OUT_HDMI = 0x040 << 8,		/**< HDMI device */
		MM_SOUND_DEVICE_OUT_MIRRORING = 0x080 << 8,	/**< MIRRORING device */
		MM_SOUND_DEVICE_OUT_USB_AUDIO = 0x100 << 8,	/**< USB Audio device */
		MM_SOUND_DEVICE_OUT_MULTIMEDIA_DOCK = 0x200 << 8,
													/**< Multimedia DOCK device */
	} mm_sound_device_out;

	typedef enum {
		MM_SOUND_VOLUME_DEVICE_OUT_SPEAKER,			/**< Device builtin speaker */
		MM_SOUND_VOLUME_DEVICE_OUT_RECEIVER,		/**< Device builtin receiver */
		MM_SOUND_VOLUME_DEVICE_OUT_WIRED_ACCESSORY,	/**< Wired output devices such as headphone, headset, and so on. */
		MM_SOUND_VOLUME_DEVICE_OUT_BT_SCO,			/**< Bluetooth SCO device */
		MM_SOUND_VOLUME_DEVICE_OUT_BT_A2DP,			/**< Bluetooth A2DP device */
		MM_SOUND_VOLUME_DEVICE_OUT_DOCK,			/**< DOCK device */
		MM_SOUND_VOLUME_DEVICE_OUT_HDMI,			/**< HDMI device */
		MM_SOUND_VOLUME_DEVICE_OUT_MIRRORING,		/**< MIRRORING device */
		MM_SOUND_VOLUME_DEVICE_OUT_USB_AUDIO,		/**< USB Audio device */
		MM_SOUND_VOLUME_DEVICE_OUT_MULTIMEDIA_DOCK,	/**< Multimedia DOCK device */
	} mm_sound_volume_device_out_t;

#define MM_SOUND_ROUTE_NUM 16
#define MM_SOUND_NAME_NUM 32

	typedef enum {
		MM_SOUND_ROUTE_OUT_SPEAKER = MM_SOUND_DEVICE_OUT_SPEAKER,
															  /**< Routing audio output to builtin device such as internal speaker. */
		MM_SOUND_ROUTE_OUT_RECEIVER = MM_SOUND_DEVICE_OUT_RECEIVER,
																/**< Routing audio output to builtin device such as internal receiver. */
		MM_SOUND_ROUTE_OUT_WIRED_ACCESSORY = MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
																			 /**< Routing audio output to wired accessory such as headphone, headset, and so on. */
		MM_SOUND_ROUTE_OUT_BLUETOOTH_SCO = MM_SOUND_DEVICE_OUT_BT_SCO,
																   /**< Routing audio output to bluetooth SCO. */
		MM_SOUND_ROUTE_OUT_BLUETOOTH_A2DP = MM_SOUND_DEVICE_OUT_BT_A2DP,
																	 /**< Routing audio output to bluetooth A2DP. */
		MM_SOUND_ROUTE_OUT_DOCK = MM_SOUND_DEVICE_OUT_DOCK,
														/**< Routing audio output to DOCK */
		MM_SOUND_ROUTE_OUT_HDMI = MM_SOUND_DEVICE_OUT_HDMI,
														/**< Routing audio output to HDMI */
		MM_SOUND_ROUTE_OUT_MIRRORING = MM_SOUND_DEVICE_OUT_MIRRORING,
																  /**< Routing audio output to MIRRORING */
		MM_SOUND_ROUTE_OUT_USB_AUDIO = MM_SOUND_DEVICE_OUT_USB_AUDIO,
																  /**< Routing audio output to USB Audio */
		MM_SOUND_ROUTE_OUT_MULTIMEDIA_DOCK = MM_SOUND_DEVICE_OUT_MULTIMEDIA_DOCK,
																			  /**< Routing audio output to Multimedia DOCK */
		MM_SOUND_ROUTE_IN_MIC = MM_SOUND_DEVICE_IN_MIC,
													/**< Routing audio input to device builtin mic. */
		MM_SOUND_ROUTE_IN_WIRED_ACCESSORY = MM_SOUND_DEVICE_IN_WIRED_ACCESSORY,
																			/**< Routing audio input to wired accessory. */
		MM_SOUND_ROUTE_IN_MIC_OUT_RECEIVER = MM_SOUND_DEVICE_IN_MIC | MM_SOUND_DEVICE_OUT_RECEIVER,
																								/**< Routing audio input to device builtin mic and routing audio output to builtin receiver*/
		MM_SOUND_ROUTE_IN_MIC_OUT_SPEAKER = MM_SOUND_DEVICE_IN_MIC | MM_SOUND_DEVICE_OUT_SPEAKER,
																							   /**< Routing audio input to device builtin mic and routing audio output to builtin speaker */
		MM_SOUND_ROUTE_IN_MIC_OUT_HEADPHONE = MM_SOUND_DEVICE_IN_MIC | MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
																									   /**< Routing audio input to device builtin mic and routing audio output to headphone */
		MM_SOUND_ROUTE_INOUT_HEADSET = MM_SOUND_DEVICE_IN_WIRED_ACCESSORY | MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,/**< Routing audio input and output to headset*/
		MM_SOUND_ROUTE_INOUT_BLUETOOTH = MM_SOUND_DEVICE_IN_BT_SCO | MM_SOUND_DEVICE_OUT_BT_SCO
																							/**< Routing audio input and output to bluetooth SCO */
	} mm_sound_route;

/*
 * MMSound Device APIs
 */

	typedef enum {
		MM_SOUND_DEVICE_IO_DIRECTION_IN_FLAG = 0x0001,	 /**< Flag for input devices */
		MM_SOUND_DEVICE_IO_DIRECTION_OUT_FLAG = 0x0002,	 /**< Flag for output devices */
		MM_SOUND_DEVICE_IO_DIRECTION_BOTH_FLAG = 0x0004, /**< Flag for input/output devices (both directions are available) */
		MM_SOUND_DEVICE_TYPE_INTERNAL_FLAG = 0x0010,	 /**< Flag for built-in devices */
		MM_SOUND_DEVICE_TYPE_EXTERNAL_FLAG = 0x0020,	 /**< Flag for external devices */
		MM_SOUND_DEVICE_STATE_DEACTIVATED_FLAG = 0x1000, /**< Flag for deactivated devices */
		MM_SOUND_DEVICE_STATE_ACTIVATED_FLAG = 0x2000,	 /**< Flag for activated devices */
		MM_SOUND_DEVICE_ALL_FLAG = 0xFFFF,				 /**< Flag for all devices */
	} mm_sound_device_flags_e;

	typedef enum {
		MM_SOUND_DEVICE_IO_DIRECTION_IN = 0x1,
		MM_SOUND_DEVICE_IO_DIRECTION_OUT = 0x2,
		MM_SOUND_DEVICE_IO_DIRECTION_BOTH = MM_SOUND_DEVICE_IO_DIRECTION_IN | MM_SOUND_DEVICE_IO_DIRECTION_OUT,
	} mm_sound_device_io_direction_e;

	typedef enum {
		MM_SOUND_DEVICE_STATE_DEACTIVATED,
		MM_SOUND_DEVICE_STATE_ACTIVATED,
	} mm_sound_device_state_e;

	typedef enum {
		MM_SOUND_DEVICE_TYPE_BUILTIN_SPEAKER,
											/**< Built-in speaker. */
		MM_SOUND_DEVICE_TYPE_BUILTIN_RECEIVER,
											/**< Built-in receiver. */
		MM_SOUND_DEVICE_TYPE_BUILTIN_MIC,	/**< Built-in mic. */
		MM_SOUND_DEVICE_TYPE_AUDIOJACK,		/**< Audio jack such as headphone, headset, and so on. */
		MM_SOUND_DEVICE_TYPE_BLUETOOTH,		/**< Bluetooth */
		MM_SOUND_DEVICE_TYPE_HDMI,			/**< HDMI. */
		MM_SOUND_DEVICE_TYPE_MIRRORING,		/**< MIRRORING. */
		MM_SOUND_DEVICE_TYPE_USB_AUDIO,		/**< USB Audio. */
	} mm_sound_device_type_e;

	typedef void *MMSoundDevice_t;		/**< MMsound Device handle */
	typedef void *MMSoundDeviceList_t;	/**< MMsound Device list handle */
	typedef void (*mm_sound_device_connected_cb) (MMSoundDevice_t device_h, bool is_connected, void *user_data);
	typedef void (*mm_sound_device_info_changed_cb) (MMSoundDevice_t device_h, int changed_info_type, void *user_data);

	int mm_sound_add_device_connected_callback(mm_sound_device_flags_e flags,
											   mm_sound_device_connected_cb func, void *user_data, unsigned int *subs_id);
	int mm_sound_remove_device_connected_callback(unsigned int subs_id);
	int mm_sound_add_device_information_changed_callback(mm_sound_device_flags_e
														 flags,
														 mm_sound_device_info_changed_cb
														 func, void *user_data, unsigned int *subs_id);
	int mm_sound_remove_device_information_changed_callback(unsigned int
															subs_id);

	int mm_sound_get_current_device_list(mm_sound_device_flags_e device_mask, MMSoundDeviceList_t * device_list);
	int mm_sound_free_device_list(MMSoundDeviceList_t device_list);
	int mm_sound_get_next_device(MMSoundDeviceList_t device_list, MMSoundDevice_t * device);
	int mm_sound_get_prev_device(MMSoundDeviceList_t device_list, MMSoundDevice_t * device);
	int mm_sound_get_device_type(MMSoundDevice_t device_h, mm_sound_device_type_e * type);
	int mm_sound_get_device_io_direction(MMSoundDevice_t device_h, mm_sound_device_io_direction_e * io_direction);
	int mm_sound_get_device_id(MMSoundDevice_t device_h, int *id);
	int mm_sound_get_device_state(MMSoundDevice_t device_h, mm_sound_device_state_e * state);
	int mm_sound_get_device_name(MMSoundDevice_t device_h, char **name);

/**
 * This function is to get active playback device and capture device.
 *
 * @param	playback_device			[out]	playback device.
 * @param	capture_device			[out]	capture device.
 *
 * @return 	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	None.
 * @pre		None.
 * @post	None.
 * @see		mm_sound_device_in mm_sound_device_out
 */
	int mm_sound_get_active_device(mm_sound_device_in * device_in, mm_sound_device_out * device_out);

/**
 * Active device changed callback function type.
 *
 * @param	user_data		[in]	Argument passed when callback has called
 *
 * @return	No return value
 * @remark	None.
 * @see		mm_sound_add_active_device_changed_callback mm_sound_remove_active_device_changed_callback
 */
	typedef void (*mm_sound_active_device_changed_cb) (mm_sound_device_in
													   device_in, mm_sound_device_out device_out, void *user_data);

	typedef void (*mm_sound_test_cb) (int a, void *user_data);
	int mm_sound_test(int a, int b, int *get);
	int mm_sound_add_test_callback(mm_sound_test_cb func, void *user_data, unsigned int *subs_id);
	int mm_sound_remove_test_callback(unsigned int sub_id);

	void mm_sound_convert_volume_type_to_stream_type(int volume_type, char *stream_type);

	typedef enum {
		MM_SOUND_SIGNAL_RELEASE_INTERNAL_FOCUS,
		MM_SOUND_SIGNAL_MAX,
	} mm_sound_signal_name_t;

	typedef void (*mm_sound_signal_callback) (mm_sound_signal_name_t signal, int value, void *user_data);
	int mm_sound_subscribe_signal(mm_sound_signal_name_t signal,
								  unsigned int *subscribe_id, mm_sound_signal_callback callback, void *user_data);
	void mm_sound_unsubscribe_signal(unsigned int subscribe_id);
	int mm_sound_send_signal(mm_sound_signal_name_t signal, int value);
	int mm_sound_get_signal_value(mm_sound_signal_name_t signal, int *value);

/**
	@}
 */

#ifdef __cplusplus
}
#endif
#endif							/* __MM_SOUND_H__ */
