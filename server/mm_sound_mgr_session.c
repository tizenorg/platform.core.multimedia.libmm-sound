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
#include "../include/mm_sound.h"

#include <mm_error.h>
#include <mm_debug.h>

#include <audio-session-manager.h>
#include <avsys-audio.h>

#include "include/mm_sound_mgr_session.h"
#include "include/mm_sound_mgr_asm.h"

#include <vconf.h>
#include <vconf-keys.h>

#define EARJACK_WITH_MIC	2

#define MAX_STRING_LEN	64

#define DEFAULT_SINK_BT		"bluez"
#define DEFAULT_SINK_ALSA	"alsa"

#define	MM_SOUND_DEVICE_OUT_ANY 0x0000FF00
#define MM_SOUND_DEVICE_IN_ANY	 0x000000FF

#define	MM_SOUND_DEVICE_OUT_FILTER 0x000000FF
#define MM_SOUND_DEVICE_IN_FILTER	 0x0000FF00

pthread_mutex_t g_mutex_session = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_SESSION()  do { debug_log("(*)LOCKING\n"); /*pthread_mutex_lock(&g_mutex_session);*/ debug_log("(+)LOCKED\n"); }while(0)
#define UNLOCK_SESSION()  do {  /* pthread_mutex_unlock(&g_mutex_session);*/ debug_log("(-)UNLOCKED\n"); }while(0)

#define RESET_ACTIVE(x)    (g_info.device_active &= x)
#define RESET_AVAILABLE(x)    (g_info.device_available &= x)

#define SET_ACTIVE(x)    (g_info.device_active |= x)
#define SET_AVAILABLE(x)    (g_info.device_available |= x)

#define SET_PLAYBACK_ONLY_ACTIVE(x)  do { RESET_ACTIVE(MM_SOUND_DEVICE_OUT_FILTER); SET_ACTIVE(x); }while(0)
#define SET_CAPTURE_ONLY_ACTIVE(x)  do {  RESET_ACTIVE(MM_SOUND_DEVICE_IN_FILTER); SET_ACTIVE(x); }while(0)


#define UNSET_ACTIVE(x)    (g_info.device_active &= (~x))
#define UNSET_AVAILABLE(x)    (g_info.device_available &= (~x))

#define TOGGLE_ACTIVE(x)    (g_info.device_active ^= x)
#define TOGGLE_AVAILABLE(x)    (g_info.device_available ^= x)

#define IS_ACTIVE(x)    (g_info.device_active & x)
#define IS_AVAILABLE(x)    (g_info.device_available & x)

#define GET_AVAILABLE_PLAYBACK()	IS_AVAILABLE(MM_SOUND_DEVICE_OUT_ANY)
#define GET_AVAILABLE_CAPTURE()	IS_AVAILABLE(MM_SOUND_DEVICE_IN_ANY)

#define GET_ACTIVE_PLAYBACK()	IS_ACTIVE(MM_SOUND_DEVICE_OUT_ANY)
#define GET_ACTIVE_CAPTURE()	IS_ACTIVE(MM_SOUND_DEVICE_IN_ANY)

#define IS_COMMUNICATION_SESSION() ((g_info.session == SESSION_VOICECALL) || (g_info.session == SESSION_VOIP))
#define IS_NOTIFICATION_SESSION() (g_info.session == SESSION_NOTIFICATION)

static int __set_sound_path_for_current_active ();
static int __set_sound_path_to_dual ();

#define ENABLE_CALLBACK
#ifndef ENABLE_CALLBACK
#define _mm_sound_mgr_device_available_device_callback(a,b,c)	MM_ERROR_NONE
#define _mm_sound_mgr_device_active_device_callback(a,b)	MM_ERROR_NONE
#endif

#define SOUND_DOCK_ON	"/usr/share/svi/sound/operation/new_chat.wav"
#define SOUND_DOCK_OFF	"/usr/share/svi/sound/operation/sent_chat.wav"

typedef struct _session_info_struct
{
	int asm_handle;
	int device_available;
	int device_active;
	int headset_type;

	session_t session;
	subsession_t subsession;

	char bt_name[MAX_STRING_LEN];
	char default_sink_name[MAX_STRING_LEN];


} SESSION_INFO_STRUCT;


SESSION_INFO_STRUCT g_info;

#define PLAYBACK_NUM	6
#define CAPTURE_NUM	3

typedef enum
{
	NO_NOTI = 0,
	DO_NOTI
} noti_t;


static void dump_info ()
{
	int i = 0;

	char *playback_device_str[] = { "SPEAKER ", "RECEIVER ", "HEADSET ", "BTSCO ", "BTA2DP ", "DOCK " };
	char *capture_device_str[] = { "MAINMIC ", "HEADSET ", "BTMIC "  };

	static char tmp_str[128];
	static char tmp_str2[128];

	debug_log ("<----------------------------------------------------->\n");


	strcpy (tmp_str, "PLAYBACK = [ ");
	for (i=0; i<PLAYBACK_NUM; i++) {
		if (((g_info.device_available & MM_SOUND_DEVICE_OUT_ANY) >> 8) & (0x01 << i)) {
			strcat (tmp_str, playback_device_str[i]);
		}
	}
	strcat (tmp_str, "]");

	strcpy (tmp_str2, "CAPTURE = [ ");
		for (i=0; i<CAPTURE_NUM; i++) {
			if ((g_info.device_available & MM_SOUND_DEVICE_IN_ANY) & (0x01 << i)) {
				strcat (tmp_str2, capture_device_str[i]);
			}
	}
	strcat (tmp_str2, "]");
	debug_log ("*** Available = [0x%08x], %s %s", g_info.device_available, tmp_str, tmp_str2);

	strcpy (tmp_str, "PLAYBACK = [ ");
	for (i=0; i<PLAYBACK_NUM; i++) {
		if (((g_info.device_active & MM_SOUND_DEVICE_OUT_ANY) >> 8) & (0x01 << i)) {
			strcat (tmp_str, playback_device_str[i]);
		}
	}
	strcat (tmp_str, "]");

	strcpy (tmp_str2, "CAPTURE = [ ");
		for (i=0; i<CAPTURE_NUM; i++) {
			if ((g_info.device_active & MM_SOUND_DEVICE_IN_ANY) & (0x01 << i)) {
				strcat (tmp_str2, capture_device_str[i]);
			}
	}
	strcat (tmp_str2, "]");
	debug_log ("***    Active = [0x%08x], %s %s", g_info.device_active, tmp_str, tmp_str2);


	debug_log ("*** Headset type = [%d], BT = [%s], default sink = [%s]\n", g_info.headset_type, g_info.bt_name, g_info.default_sink_name);
	debug_log ("*** Session = [%d], SubSession = [%d]\n", g_info.session, g_info.subsession);
	debug_log ("<----------------------------------------------------->\n");
}

/* ------------------------- ASM ------------------------------------*/
static pthread_mutex_t _asm_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool _asm_register_for_headset (int * handle)
{
	int asm_error = 0;

	if (handle == NULL) {
		debug_error ("Handle is not valid!!!\n");
		return false;
	}

	if(!ASM_register_sound_ex (-1, handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &asm_error, __asm_process_message)) {
			debug_warning("earjack event register failed with 0x%x\n", asm_error);
			return false;
	}

	return true;
}

static void _asm_pause_process(int handle)
{
	int asm_error = 0;

	MMSOUND_ENTER_CRITICAL_SECTION( &_asm_mutex )

	/* If no asm handle register here */
	if (g_info.asm_handle ==  -1) {
		debug_msg ("ASM handle is not valid, try to register once more\n");

		/* This register should be success */
		if (_asm_register_for_headset (&g_info.asm_handle)) {
			debug_msg("_asm_register_for_headset() success\n");
		} else {
			debug_error("_asm_register_for_headset() failed\n");
		}
	}

	//do pause
	debug_warning("Send earphone unplug event to Audio Session Manager Server for BT headset\n");

	if(!ASM_set_sound_state_ex(handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_PLAYING, ASM_RESOURCE_NONE, &asm_error, __asm_process_message)) {
		debug_error("earjack event set sound state to playing failed with 0x%x\n", asm_error);
	}

	if(!ASM_set_sound_state_ex(handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_STOP, ASM_RESOURCE_NONE, &asm_error, __asm_process_message)) {
		debug_error("earjack event set sound state to stop failed with 0x%x\n", asm_error);
	}

	MMSOUND_LEAVE_CRITICAL_SECTION( &_asm_mutex )
}

static bool _asm_unregister_for_headset (int *handle)
{
	int asm_error = 0;

	if (handle == NULL) {
		debug_error ("Handle is not valid!!!\n");
		return false;
	}

	if(!ASM_unregister_sound_ex(handle, ASM_EVENT_EARJACK_UNPLUG, &asm_error, __asm_process_message)) {
		debug_error("earjack event unregister failed with 0x%x\n", asm_error);
		return false;
	}

	return true;
}

/* ------------------------- INTERNAL FUNCTIONS ------------------------------------*/

static void __update_volume_value(volume_type_t volume_type)
{
	int ret = 0;
	int volume_value = 0;

	ret = mm_sound_volume_get_value(volume_type, &volume_value);
	if (ret == MM_ERROR_NONE) {
		ret = mm_sound_volume_set_value(volume_type, volume_value);
		if (ret != MM_ERROR_NONE) {
			debug_error("mm_sound_volume_get_value failed....ret = [%x]\n", ret);
		}
	} else {
		debug_error("mm_sound_volume_get_value failed....ret = [%x]\n", ret);
	}
}

static void _set_path_with_notification(noti_t noti)
{
	int ret = MM_ERROR_NONE;

	debug_msg ("[%s] noti=%d\n", __func__,noti);

	/* Set path based on current active device */
	ret = __set_sound_path_for_current_active();
	if (ret != MM_ERROR_NONE) {
		debug_error ("__set_sound_path_for_current_active() failed [%x]\n", ret);
		return;
	}

	if (noti == DO_NOTI) {
		/* Notify current active device */
		ret = _mm_sound_mgr_device_active_device_callback(GET_ACTIVE_CAPTURE(), GET_ACTIVE_PLAYBACK());
		if (ret != MM_ERROR_NONE) {
			debug_error ("_mm_sound_mgr_device_active_device_callback() failed [%x]\n", ret);
		}
	}
}

static int __set_playback_route_communication (session_state_t state)
{
	int ret = MM_ERROR_NONE;
	int gain;

	debug_fenter();

	if (state == SESSION_START) {
		SET_AVAILABLE(MM_SOUND_DEVICE_OUT_RECEIVER);
		debug_log ("voicecall session started...only receiver available set on...");

		ret = _mm_sound_mgr_device_available_device_callback(MM_SOUND_DEVICE_IN_NONE, MM_SOUND_DEVICE_OUT_RECEIVER, 1);
		if (ret != MM_ERROR_NONE) {
			debug_error ("_mm_sound_mgr_device_available_device_callback() failed [%x]\n", ret);
			goto ROUTE_COMM_EXIT;
		}

		/* (speaker = receiver, headset = headset, bt a2dp = bt sco) */
		/* OUT */
		if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER)) {
			debug_log ("active out was SPEAKER => activate receiver!!\n");
			SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_RECEIVER);

		} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
			debug_log ("active out was BT A2DP => activate BT SCO!!\n");
			SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_BT_SCO);
			SET_CAPTURE_ONLY_ACTIVE(MM_SOUND_DEVICE_IN_BT_SCO);
		}
		/* FIXME : Do we have to set IN device ??? */

		_set_path_with_notification(DO_NOTI);

		dump_info();

	} else { /* SESSION_END */
		UNSET_AVAILABLE(MM_SOUND_DEVICE_OUT_RECEIVER);

		ret = _mm_sound_mgr_device_available_device_callback(MM_SOUND_DEVICE_IN_NONE, MM_SOUND_DEVICE_OUT_RECEIVER, 0);
		if (ret != MM_ERROR_NONE) {
			debug_error ("_mm_sound_mgr_device_available_device_callback() failed [%x]\n", ret);
			goto ROUTE_COMM_EXIT;
		}

		// RESET
		if (g_info.session == SESSION_VOICECALL)
			gain = AVSYS_AUDIO_GAIN_EX_VOICECALL;
		else if (g_info.session == SESSION_VOIP)
			gain = AVSYS_AUDIO_GAIN_EX_VIDEOCALL;
		else {
			debug_warning ("Not valid session info....\n");
			gain = AVSYS_AUDIO_GAIN_EX_VOICECALL;
		}


		if (AVSYS_FAIL(avsys_audio_set_path_ex( gain,
				AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_EX_NONE,
				AVSYS_AUDIO_PATH_OPTION_NONE ))) {
			debug_error ("avsys_audio_set_path_ex() failed [%x]\n", ret);
			ret = MM_ERROR_SOUND_INTERNAL;
			goto ROUTE_COMM_EXIT;
		}

		/* (speaker = receiver, headset = headset, bt a2dp = bt sco) */
		/* OUT */
		if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_RECEIVER)) {
			debug_log ("active out was RECEIVER => activate SPEAKER!!\n");
			SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER);
		} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_SCO)) {
			debug_log ("active out was BT SCO => activate BT A2DP!!\n");
			SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP);
			if (IS_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY)) {
				SET_CAPTURE_ONLY_ACTIVE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
			} else {
				SET_CAPTURE_ONLY_ACTIVE(MM_SOUND_DEVICE_IN_MIC);
			}
		} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER) || IS_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
			debug_log ("Keep current active device\n");
		} else {
			debug_error ("Error!!! No active device when call exit!!! Set default one\n");
			/* FIXME : This should be based on priority???? */
			SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER);
			SET_CAPTURE_ONLY_ACTIVE(MM_SOUND_DEVICE_IN_MIC);
		}

		debug_log ("voicecall session stopped...set path based on current active device");
		_set_path_with_notification(DO_NOTI);

		dump_info();
	}

ROUTE_COMM_EXIT:

	debug_fleave();

	return ret;
}

static int __set_playback_route_fmradio (session_state_t state)
{
	int ret = MM_ERROR_NONE;
	int out;

	debug_fenter();

	if (state == SESSION_START) {

		if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER))
			out = AVSYS_AUDIO_PATH_EX_SPK;
		else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY))
			out = AVSYS_AUDIO_PATH_EX_HEADSET;
		else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP))
			out = AVSYS_AUDIO_PATH_EX_A2DP;

		/* PATH SET */
		if (AVSYS_FAIL(avsys_audio_set_path_ex( AVSYS_AUDIO_GAIN_EX_FMRADIO,
										out, AVSYS_AUDIO_PATH_EX_FMINPUT,
										AVSYS_AUDIO_PATH_OPTION_NONE))) {
			debug_error ("avsys_audio_set_path_ex() failed\n");
			ret = MM_ERROR_SOUND_INTERNAL;
			goto ROUTE_FMRADIO_EXIT;
		}


	} else { /* SESSION_END */
		/* PATH RELEASE */
		if (AVSYS_FAIL(avsys_audio_set_path_ex( AVSYS_AUDIO_GAIN_EX_FMRADIO,
										AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_EX_NONE,
										AVSYS_AUDIO_PATH_OPTION_NONE ))) {
			debug_error ("avsys_audio_set_path_ex() failed\n");
			ret = MM_ERROR_SOUND_INTERNAL;
			goto ROUTE_FMRADIO_EXIT;
		}

		/* Set as current active status */
		_set_path_with_notification (NO_NOTI);
	}

	if (AVSYS_FAIL(avsys_audio_set_ext_device_status(AVSYS_AUDIO_EXT_DEVICE_FMRADIO, state))) {
		debug_error ("avsys_audio_set_ext_device_status() failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	}

ROUTE_FMRADIO_EXIT:

	debug_fleave();

	return ret;
}

static int __set_playback_route_notification (session_state_t state)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (state == SESSION_START) {
		ret = __set_sound_path_to_dual ();
		if (ret != MM_ERROR_NONE) {
			debug_error ("__set_sound_path_to_dual() failed [%x]\n", ret);
		}

	} else { /* SESSION_END */
		_set_path_with_notification (NO_NOTI);
	}

	debug_fleave();

	return ret;
}


static int __set_sound_path_for_current_active ()
{
	int ret = MM_ERROR_NONE;
	int option = AVSYS_AUDIO_PATH_OPTION_NONE;
	int in, out, gain;
	bool sound_play;

	if (IS_NOTIFICATION_SESSION()) {
		debug_log ("Current session is NOTI, pending path setting. path set will be done after NOTI ends")
		return ret;
	}

	debug_fenter();

	/* Pulseaudio route */
	if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
		debug_log ("BT A2DP is active, Set default sink to BLUEZ");
		MMSoundMgrPulseSetDefaultSink (DEFAULT_SINK_BT);
	} else {
		debug_log ("BT A2DP is not active, Set default sink to ALSA");
		MMSoundMgrPulseSetDefaultSink (DEFAULT_SINK_ALSA);
	}

	/* IN */
	if (IS_ACTIVE(MM_SOUND_DEVICE_IN_MIC)) {
		in = AVSYS_AUDIO_PATH_EX_MIC;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY)) {
		in = AVSYS_AUDIO_PATH_EX_HEADSETMIC;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_IN_BT_SCO)) {
		in = AVSYS_AUDIO_PATH_EX_BTMIC;
	}

	/* OUT */
	if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER)) {
		out = AVSYS_AUDIO_PATH_EX_SPK;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_RECEIVER)) {
		out = AVSYS_AUDIO_PATH_EX_RECV;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
		out = AVSYS_AUDIO_PATH_EX_HEADSET;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_SCO)) {
		out = AVSYS_AUDIO_PATH_EX_BTHEADSET;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
		out = AVSYS_AUDIO_PATH_EX_A2DP;
	} else if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_DOCK)) {
		out = AVSYS_AUDIO_PATH_EX_DOCK;
	}

	/* GAIN */
	switch (g_info.session)
	{
	case SESSION_MEDIA:
	case SESSION_NOTIFICATION:
		gain = AVSYS_AUDIO_GAIN_EX_KEYTONE;
		break;

	case SESSION_VOICECALL:
	case SESSION_VOIP:
		if (g_info.subsession == SUBSESSION_RINGTONE) {
			gain = AVSYS_AUDIO_GAIN_EX_RINGTONE;
			in = AVSYS_AUDIO_PATH_EX_NONE;

			/* If sound is mute mode, force ringtone path to headset */
			vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &sound_play);
			if (sound_play) {
				/* Normal Ringtone */
				out = AVSYS_AUDIO_PATH_EX_SPK;
				option = AVSYS_AUDIO_PATH_OPTION_DUAL_OUT;
			} else {
				/* Mute Ringtone */
				out = AVSYS_AUDIO_PATH_EX_HEADSET;
			}
		} else if (g_info.subsession == SUBSESSION_MEDIA) {
			gain = AVSYS_AUDIO_GAIN_EX_CALLTONE;
			in = AVSYS_AUDIO_PATH_EX_NONE;
		} else {
			gain = (g_info.session == SESSION_VOICECALL)?
					AVSYS_AUDIO_GAIN_EX_VOICECALL : AVSYS_AUDIO_GAIN_EX_VIDEOCALL;
		}
		break;

	case SESSION_FMRADIO:
		gain = AVSYS_AUDIO_GAIN_EX_FMRADIO;
		in = AVSYS_AUDIO_PATH_EX_FMINPUT;
		break;
	}

	debug_log ("Trying to set avsys set path gain[%d], out[%d], in[%d], option[%d]\n", gain, out, in, option);

	/* Set Path */
	if(AVSYS_FAIL(avsys_audio_set_path_ex(gain, out, in, option))) {
		debug_error ("avsys_audio_set_path_ex failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	/* clean up */
	debug_fleave();
	return ret;
}


static int __set_sound_path_to_dual ()
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	/* Sound path for ALSA */
	debug_log ("Set path to DUAL.\n");
	if(AVSYS_FAIL(avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_KEYTONE,
						AVSYS_AUDIO_PATH_EX_SPK, AVSYS_AUDIO_PATH_EX_NONE,
						AVSYS_AUDIO_PATH_OPTION_DUAL_OUT))) {
		debug_error ("avsys_audio_set_path_ex failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
	}

	/* clean up */
	debug_fleave();
	return ret;
}




static void _select_playback_active_device ()
{
	if (IS_ACTIVE(MM_SOUND_DEVICE_OUT_ANY)) {
		debug_log ("Active device exists. Nothing needed...\n");
		return;
	}

	debug_log ("No playback active device, set active based on priority!!\n");

	/* set active device based on device priority (bt>ear>spk) */
	if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
		debug_log ("BT A2DP available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP);
	} else if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_DOCK)) {
		debug_log ("DOCK available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_OUT_DOCK);
	} else if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
		debug_log ("WIRED available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
	} else {
		debug_log ("SPEAKER available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_OUT_SPEAKER);
	}
}

static void _select_capture_active_device ()
{
	if (IS_ACTIVE(MM_SOUND_DEVICE_IN_ANY)) {
		debug_log ("Active device exists. Nothing needed...\n");
		return;
	}

	debug_log ("No capture active device, set active based on priority!!\n");

	/* set active device based on device priority (bt>ear>spk) */
	if (IS_AVAILABLE(MM_SOUND_DEVICE_IN_BT_SCO) && IS_COMMUNICATION_SESSION()) {
		debug_log ("BT SCO available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_IN_BT_SCO);
	} else if (IS_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY)) {
		debug_log ("WIRED available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
	} else {
		debug_log ("MIC available, set as active!!\n");
		SET_ACTIVE(MM_SOUND_DEVICE_IN_MIC);
	}
}

static void _set_initial_active_device ()
{
	int type = 0;
	bool a2dp = 0, sco = 0;

	/* Set SPK & MIC as default available device */
	/* FIXME : spk & mic can be always on??? */
	SET_AVAILABLE(MM_SOUND_DEVICE_OUT_SPEAKER);
	SET_AVAILABLE(MM_SOUND_DEVICE_IN_MIC);

	/* Get wired status and set available status */
	MMSoundMgrHeadsetGetType (&type);
	if (type > 0) {
		SET_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
		if (type == EARJACK_WITH_MIC) {
			SET_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
		}
	}

	/* Get BT status and set available status */
	MMSoundMgrPulseGetInitialBTStatus (&a2dp, &sco);
	if (a2dp) {
		SET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP);
	}
	if (sco) {
		SET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_SCO);
		SET_AVAILABLE(MM_SOUND_DEVICE_IN_BT_SCO);
	}

	/* Set Active device based on priority */
	_select_playback_active_device ();
	_select_capture_active_device ();

	_set_path_with_notification (NO_NOTI);

	dump_info();
}

static void handle_bt_a2dp_on ()
{
	int ret = MM_ERROR_NONE;

	/* at this time, pulseaudio default sink is bt sink */
	if (IS_COMMUNICATION_SESSION()) {
		debug_log ("Current session is VOICECALL, no auto-activation!!!\n");
		return;
	}

	debug_log ("Activate BT_A2DP device\n");
	SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP);

	ret = _mm_sound_mgr_device_active_device_callback(GET_ACTIVE_CAPTURE(), GET_ACTIVE_PLAYBACK());
	if (ret != MM_ERROR_NONE) {
		debug_error ("_mm_sound_mgr_device_active_device_callback() failed [%x]\n", ret);
	}

	dump_info ();
}

static void handle_bt_a2dp_off ()
{
	if (!IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
		debug_msg("MM_SOUND_DEVICE_OUT_BT_A2DP was not active. nothing to do here.");
		dump_info ();
		return;
	}

	/* if bt was active, then do asm pause */
	debug_msg("Do pause here");
	_asm_pause_process (g_info.asm_handle);

	/* set bt device to none */
	debug_msg("Deactivate BT_A2DP device\n");
	UNSET_ACTIVE(MM_SOUND_DEVICE_OUT_BT_A2DP);

	/* activate current available device based on priority */
	_select_playback_active_device();

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}

static void handle_bt_sco_off ()
{
	/* If sco is not activated, just return */
	if (!IS_ACTIVE(MM_SOUND_DEVICE_OUT_BT_SCO) && !IS_ACTIVE(MM_SOUND_DEVICE_IN_BT_SCO)) {
		debug_msg("BT SCO was not active. nothing to do here.");
		dump_info ();
		return;
	}

	/* set bt device to none */
	debug_msg("Deactivate BT_SCO device\n");
	UNSET_ACTIVE(MM_SOUND_DEVICE_OUT_BT_SCO);
	UNSET_ACTIVE(MM_SOUND_DEVICE_IN_BT_SCO);

	/* activate current available device based on priority */
	_select_playback_active_device();
	_select_capture_active_device();

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}

static void handle_headset_on (int type)
{
#ifdef SEPARATE_EARPHONE_VOLUME
	int ret = MM_ERROR_NONE;
	volume_type_t volume_type = VOLUME_TYPE_MAX;

	/* get current volume type before changing device */
	ret = mm_sound_volume_get_current_playing_type(&volume_type);
	if (ret != MM_ERROR_NONE) {
		debug_error("mm_sound_volume_get_current_playing_type failed....ret = [%x]\n", ret);
	}
#endif

	/* at this time, pulseaudio default sink is bt sink */
	/* if fmradio session, do nothing */

	/* Skip when noti session */

	/* ToDo : alarm/notification session ???? */
	if (IS_COMMUNICATION_SESSION()) {
		debug_log ("Current session is VOICECALL, no auto-activation!!!\n");
		return;
	}

	debug_log ("Activate WIRED OUT device\n");
	SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
	if (type == EARJACK_WITH_MIC) {
		debug_log ("Activate WIRED IN device\n");
		SET_CAPTURE_ONLY_ACTIVE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
	}

#ifdef SEPARATE_EARPHONE_VOLUME
	__update_volume_value(volume_type);
#endif

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}

static void handle_headset_off ()
{
#ifdef SEPARATE_EARPHONE_VOLUME
	int ret = MM_ERROR_NONE;
	volume_type_t volume_type = VOLUME_TYPE_MAX;

	/* get current volume type before changing device */
	ret = mm_sound_volume_get_current_playing_type(&volume_type);
	if (ret != MM_ERROR_NONE) {
		debug_error("mm_sound_volume_get_current_playing_type failed....ret = [%x]\n", ret);
	}
#endif

	if (!IS_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
		debug_msg("MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY was not active. nothing to do here.");
		return;
	}

	/* if bt was active, then do asm pause */
	debug_msg("Do pause here");
	_asm_pause_process (g_info.asm_handle);

	/* set bt device to none */
	debug_msg("Deactivate WIRED IN/OUT device\n");
	UNSET_ACTIVE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
	UNSET_ACTIVE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);

	/* For voicecall session, activation device is up-to application policy */
	if (IS_COMMUNICATION_SESSION()) {
		debug_log ("Current session is VOICECALL, no auto-activation!!!\n");
		return;
	}

	/* activate current available device based on priority */
	_select_playback_active_device();
	_select_capture_active_device();

#ifdef SEPARATE_EARPHONE_VOLUME
	__update_volume_value(volume_type);
#endif

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}

void _sound_finished_cb (void *data)
{
	debug_log ("sound play finished!!!\n");
	*(bool*)data = true;
}

static void _play_dock_sound_sync(int is_on)
{
	int handle;
	bool is_play_finished = false;

	debug_log ("start to play dock sound [%d]\n", is_on);
	mm_sound_play_loud_solo_sound((is_on? SOUND_DOCK_ON : SOUND_DOCK_OFF), VOLUME_TYPE_FIXED, _sound_finished_cb, &is_play_finished, &handle);
	/* FIXME : need to enhance waiting method */
	debug_log ("waiting for dock sound finish\n");
	while (!is_play_finished) {
		usleep (10000); // 10 ms
	}
	debug_log ("dock sound finished!!!\n");
}

static void handle_dock_on ()
{
	/* ToDo : alarm/notification session ???? */
	if (IS_COMMUNICATION_SESSION()) {
		debug_log ("Current session is VOICECALL, no auto-activation!!!\n");
		return;
	}

	debug_log ("Activate DOCK device\n");
	SET_PLAYBACK_ONLY_ACTIVE(MM_SOUND_DEVICE_OUT_DOCK);

	/* Enforced audio */
	_play_dock_sound_sync(true);

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}

static void handle_dock_off ()
{
	if (!IS_ACTIVE(MM_SOUND_DEVICE_OUT_DOCK)) {
		debug_msg("MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY was not active. nothing to do here.");
		return;
	}

	/* if DOCK was active, then do asm pause */
	debug_msg("Do pause here");
	_asm_pause_process (g_info.asm_handle);

	/* Enforced audio */
	_play_dock_sound_sync(false);

	/* set DOCK device to none */
	debug_msg("Deactivate DOCK device\n");
	UNSET_ACTIVE(MM_SOUND_DEVICE_OUT_DOCK);

	/* activate current available device based on priority */
	_select_playback_active_device();

	/* Do set path and notify result */
	_set_path_with_notification(DO_NOTI);

	dump_info ();
}


/* ------------------------- EXTERNAL FUNCTIONS ------------------------------------*/
/* DEVICE : Called by mgr_pulse for updating current default_sink_name */
int MMSoundMgrSessionSetDefaultSink (char *default_sink_name)
{
	LOCK_SESSION();

	strcpy (g_info.default_sink_name, default_sink_name);
	debug_msg ("[SESSION][%s][%d] default sink=[%s]\n", __func__, __LINE__, default_sink_name);

	/* ToDo: do something */

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

/* DEVICE : Called by mgr_pulse for bt and mgr_headset for headset */
int MMSoundMgrSessionSetDeviceAvailable (device_type_t device, int available, int type, char* name)
{
	LOCK_SESSION();

	debug_msg ("[SESSION] device = %d, available = %d, type = %d, name = %s\n", device, available, type, name);
	switch (device)
	{
	case DEVICE_WIRED:
		if (available) {

			if (!IS_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
				SET_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
				if (type == EARJACK_WITH_MIC) {
					SET_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
					_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_WIRED_ACCESSORY,
											MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
											AVAILABLE);
				} else {
					_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
											AVAILABLE);
				}
				handle_headset_on(type);
			} else {
				debug_log ("Already device [%d] is available...\n", device);
			}

		} else {

			if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY)) {
				UNSET_AVAILABLE(MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY);
				if (IS_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY)) {
					UNSET_AVAILABLE(MM_SOUND_DEVICE_IN_WIRED_ACCESSORY);
					_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_WIRED_ACCESSORY,
											MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
											NOT_AVAILABLE);

				} else {
					_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_WIRED_ACCESSORY,
											NOT_AVAILABLE);
				}
				handle_headset_off();
			} else {
				debug_log ("Already device [%d] is unavailable...\n", device);
			}

		}
		break;

	case DEVICE_BT_A2DP:
		strcpy (g_info.bt_name, (name)? name : "");
		if (available) {
			if (!IS_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
				SET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_BT_A2DP,
											AVAILABLE);

				handle_bt_a2dp_on();
			} else {
				debug_log ("Already device [%d] is available...\n", device);
			}
		} else {
			if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP)) {
				UNSET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_A2DP);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_BT_A2DP,
											NOT_AVAILABLE);

				handle_bt_a2dp_off();
			} else {
				debug_log ("Already device [%d] is unavailable...\n", device);
			}
		}
		break;

	case DEVICE_BT_SCO:
		if (available) {
			if (!IS_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_SCO)) {
				SET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_SCO);
				SET_AVAILABLE(MM_SOUND_DEVICE_IN_BT_SCO);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_BT_SCO,
											MM_SOUND_DEVICE_OUT_BT_SCO,
											AVAILABLE);
			} else {
				debug_log ("Already device [%d] is available...\n", device);
			}
		} else {
			if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_SCO)) {
				UNSET_AVAILABLE(MM_SOUND_DEVICE_OUT_BT_SCO);
				UNSET_AVAILABLE(MM_SOUND_DEVICE_IN_BT_SCO);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_BT_SCO,
											MM_SOUND_DEVICE_OUT_BT_SCO,
											NOT_AVAILABLE);

				handle_bt_sco_off();
			} else {
				debug_log ("Already device [%d] is unavailable...\n", device);
			}
		}
		break;

	case DEVICE_DOCK:
		if (available) {
			if (!IS_AVAILABLE(MM_SOUND_DEVICE_OUT_DOCK)) {
				SET_AVAILABLE(MM_SOUND_DEVICE_OUT_DOCK);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_DOCK,
											AVAILABLE);
				handle_dock_on();
			} else {
				debug_log ("Already device [%d] is available...\n", device);
			}
		} else {
			if (IS_AVAILABLE(MM_SOUND_DEVICE_OUT_DOCK)) {
				UNSET_AVAILABLE(MM_SOUND_DEVICE_OUT_DOCK);
				_mm_sound_mgr_device_available_device_callback(
											MM_SOUND_DEVICE_IN_NONE,
											MM_SOUND_DEVICE_OUT_DOCK,
											NOT_AVAILABLE);

				handle_dock_off();
			} else {
				debug_log ("Already device [%d] is unavailable...\n", device);
			}
		}
		break;
	}

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

int MMSoundMgrSessionIsDeviceAvailableNoLock (mm_sound_device_out playback, mm_sound_device_in capture, bool *available)
{
	int ret = MM_ERROR_NONE;
	debug_log ("[SESSION][%s][%d] query playback=[0x%X] capture=[0x%X], current available = [0x%X]\n",
			__func__, __LINE__, playback, capture, g_info.device_available);

	if (available) {
		if (playback == MM_SOUND_DEVICE_OUT_NONE) {
			*available = IS_AVAILABLE(capture);
		} else if (capture == MM_SOUND_DEVICE_IN_NONE) {
			*available = IS_AVAILABLE(playback);
		} else {
			*available = (IS_AVAILABLE(playback) && IS_AVAILABLE(capture));
		}
		debug_log ("[%s][%d] return available = [%d]\n", __func__, __LINE__, *available);
	} else {
		debug_warning ("Invalid argument!!!\n");
		ret = MM_ERROR_INVALID_ARGUMENT;
	}

	return ret;
}


int MMSoundMgrSessionIsDeviceAvailable (mm_sound_device_out playback, mm_sound_device_in capture, bool *available)
{
	int ret = MM_ERROR_NONE;

	LOCK_SESSION();
	ret = MMSoundMgrSessionIsDeviceAvailableNoLock (playback, capture, available);
	UNLOCK_SESSION();

	return ret;
}

int MMSoundMgrSessionGetAvailableDevices (int *playback, int *capture)
{
	if (playback == NULL || capture == NULL) {
		debug_error ("Invalid input parameter\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	LOCK_SESSION();

	*playback = GET_AVAILABLE_PLAYBACK();
	*capture  = GET_AVAILABLE_CAPTURE();
	debug_msg ("[SESSION][%s][%d] return available playback=[0x%X]/capture=[0x%X]\n", __func__, __LINE__, *playback, *capture);

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

int MMSoundMgrSessionSetDeviceActive (mm_sound_device_out playback, mm_sound_device_in capture)
{
	int ret = MM_ERROR_NONE;
	int old_active = g_info.device_active;
#ifdef SEPARATE_EARPHONE_VOLUME
	volume_type_t volume_type = VOLUME_TYPE_MAX;

	/* get current volume type before changing device */
	ret = mm_sound_volume_get_current_playing_type(&volume_type);
	if (ret != MM_ERROR_NONE) {
		debug_error("mm_sound_volume_get_current_playing_type failed....ret = [%x]\n", ret);
	}
#endif

	LOCK_SESSION();

	debug_msg ("[SESSION][%s][%d] playback=[0x%X] capture=[0x%X]\n", __func__, __LINE__, playback, capture);

	/* Check whether device is available */
	if ((playback && !IS_AVAILABLE(playback)) || (capture && !IS_AVAILABLE(capture))) {
		debug_warning ("Failed to set active state to unavailable device!!!\n");
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto END_SET_DEVICE;
	}

	/* Update active state */
	debug_log ("Update active device as request\n");
	if (playback) {
		SET_PLAYBACK_ONLY_ACTIVE(playback);
	}
	if (capture) {
		SET_CAPTURE_ONLY_ACTIVE(capture);
	}

	/* If there's changes do path set and inform callback */
	if (old_active != g_info.device_active) {
		debug_msg ("Changes happens....set path based on current active device and inform callback!!!\n");

#ifdef SEPARATE_EARPHONE_VOLUME
		__update_volume_value(volume_type);
#endif

		/* Do set path based on current active state */
		_set_path_with_notification(DO_NOTI);
	} else {
		debug_msg ("No changes....nothing to do...\n");
	}

END_SET_DEVICE:
	UNLOCK_SESSION();
	return ret;
}

int MMSoundMgrSessionGetDeviceActive (mm_sound_device_out *playback, mm_sound_device_in *capture)
{
	if (playback == NULL || capture == NULL) {
		debug_error ("Invalid input parameter\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	LOCK_SESSION();

	*playback = GET_ACTIVE_PLAYBACK();
	*capture  = GET_ACTIVE_CAPTURE();
	debug_msg ("[SESSION][%s][%d] return active playback=[0x%X]/capture=[0x%X]\n", __func__, __LINE__,
				*playback,*capture);

	UNLOCK_SESSION();
	return MM_ERROR_NONE;
}

/* SUBSESSION */
int MMSoundMgrSessionSetSession(session_t session, session_state_t state)
{
	LOCK_SESSION();

	debug_msg ("[SESSION][%s][%d] session=[%d] state=[%d]\n", __func__, __LINE__, session, state);

	/* Update session */
	if (state)
		g_info.session = session;
	else
		g_info.session = SESSION_MEDIA;

	/* Do action based on new session */
	switch (session)
	{
	case SESSION_MEDIA:
		/* ToDo:
		start
			mic setting
		end
		*/
		break;

	case SESSION_VOICECALL:
	case SESSION_VOIP:
		__set_playback_route_communication (state);
		break;

	case SESSION_FMRADIO:
		__set_playback_route_fmradio (state);
		break;

	case SESSION_NOTIFICATION:
		__set_playback_route_notification (state);
		break;
	}

	UNLOCK_SESSION();
	return MM_ERROR_NONE;
}

int MMSoundMgrSessionGetSession(session_t *session)
{
	if (session == NULL) {
		debug_error ("Invalid input parameter\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	//LOCK_SESSION();
	*session = g_info.session;
	//UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

/* SUBSESSION */
int MMSoundMgrSessionSetSubSession(subsession_t subsession)
{
	LOCK_SESSION();

	g_info.subsession = subsession;
	debug_msg ("[SESSION][%s][%d] subsession=[%d]\n", __func__, __LINE__, subsession);

	_set_path_with_notification (NO_NOTI);

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

int MMSoundMgrSessionGetSubSession(subsession_t *subsession)
{
	if (subsession == NULL) {
		debug_error ("Invalid input parameter\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	LOCK_SESSION();

	*subsession = g_info.subsession;

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

char* MMSoundMgrSessionGetBtA2DPName ()
{
	return g_info.bt_name;
}

int MMSoundMgrSessionInit(void)
{
	LOCK_SESSION();

	debug_fenter();

	memset (&g_info, 0, sizeof (SESSION_INFO_STRUCT));

	/* FIXME: Initial status should be updated */
	_set_initial_active_device ();

	/* Register for headset unplug */
	if (_asm_register_for_headset (&g_info.asm_handle) == false) {
		debug_error ("Failed to register ASM for headset\n");
	}

	debug_fleave();

	UNLOCK_SESSION();
	return MM_ERROR_NONE;
}

int MMSoundMgrSessionFini(void)
{
	LOCK_SESSION();

	debug_fenter();

	/* Unregister for headset unplug */
	_asm_unregister_for_headset (&g_info.asm_handle);

	debug_fleave();

	UNLOCK_SESSION();

	return MM_ERROR_NONE;
}

