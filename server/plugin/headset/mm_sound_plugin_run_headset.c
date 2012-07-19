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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <pthread.h>
#include <avsys-audio.h>
#include <vconf.h>
#include <pulse/pulseaudio.h>

#include "../../include/mm_sound_plugin_run.h"

#include <audio-session-manager.h>
#include "../../../include/mm_ipc.h"
#include "../../../include/mm_sound_common.h"
#include "../../../include/mm_sound.h"

#include <string.h>

static int g_asm_handle;
static avsys_audio_playing_devcie_t g_device;

static pthread_mutex_t _asm_mutex = PTHREAD_MUTEX_INITIALIZER;

#define PULSE_DEFAULT_SINK_ALSA	0
#define PULSE_DEFAULT_SINK_BT		1
#define EARJACK_EJECTED	0

bool _asm_register_for_headset (int * handle)
{
	int asm_error = 0;

	if (handle == NULL) {
		debug_error ("Handle is not valid!!!\n");
		return false;
	}

	if(!ASM_register_sound(-1, handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &asm_error)) {
		debug_warning("earjack event register failed with 0x%x\n", asm_error);
		return false;
	}
	return true;
}

void _asm_pause_process(int handle)
{
	int asm_error = 0;
	MMSOUND_ENTER_CRITICAL_SECTION( &_asm_mutex )
	if(!ASM_set_sound_state(handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_PLAYING, ASM_RESOURCE_NONE, &asm_error )) {
		debug_error("earjack event set sound state to playing failed with 0x%x\n", asm_error);
	}

	if(!ASM_set_sound_state(handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_STOP, ASM_RESOURCE_NONE, &asm_error )) {
		debug_error("earjack event set sound state to stop failed with 0x%x\n", asm_error);
	}
	MMSOUND_LEAVE_CRITICAL_SECTION( &_asm_mutex )
}

int _update_route_policy_from_vconf()
{
	//This functionis trivial work to match avsystem shared memory route information with vconf route information in db.
	int err = MM_ERROR_NONE;
	int lv_route = 0;
    mode_t prev_mask = 0;

    prev_mask = umask(0);
	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		umask(prev_mask);
		return MM_ERROR_SOUND_INTERNAL;
	}
	umask(prev_mask);

	err = vconf_get_int(ROUTE_VCONF_KEY, &lv_route);
	if(err < 0 ) {
		debug_error("Can not get route policy from vconf. during Headset plugin init. set default.\n");
		err = vconf_set_int(ROUTE_VCONF_KEY, (int)AVSYS_AUDIO_ROUTE_POLICY_DEFAULT);
		if(err < 0) {
			debug_error("Set route polpa_threaded_mainloop *mainloopicy to vconf failed in headset plugin\n");
		}
	} else {
		avsys_audio_route_policy_t av_route;
		err = avsys_audio_get_route_policy((avsys_audio_route_policy_t*)&av_route);
		if(AVSYS_FAIL(err)) {
			debug_error("Can not get route policy to avsystem 0x%x\n", err);
			av_route = -1;
		}
		if(av_route != lv_route) {
			//match vconf & shared mem info
			err = avsys_audio_set_route_policy(lv_route);
			if(AVSYS_FAIL(err)) {
				debug_error("avsys_audio_set_route_policy failed 0x%x\n", err);
			}
		}
	}

	if(MM_ERROR_NONE != __mm_sound_unlock()) {
		debug_error("Unlock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

bool _is_normal_state ()
{
	int gain, out, in, option;

	if (avsys_audio_get_path_ex(&gain, &out, &in, &option) == AVSYS_STATE_SUCCESS) {
		debug_msg ("[%s] gain = %x, out = %x, in = %x, option = %x\n", __func__, gain, out, in, option);
		if (gain != AVSYS_AUDIO_GAIN_EX_RINGTONE &&
			gain != AVSYS_AUDIO_GAIN_EX_VOICECALL &&
			gain != AVSYS_AUDIO_GAIN_EX_VIDEOCALL  &&
			gain != AVSYS_AUDIO_GAIN_EX_CALLTONE) {
			return true;
		}
	} else {
		debug_error ("Failed to get path\n");
	}

	return false;
}

int _set_audio_route_to_default()
{
	int ret = MM_ERROR_NONE;
	int codec_option = AVSYS_AUDIO_PATH_OPTION_JACK_AUTO;
	int gain, out, in, option;
	debug_msg("Set audio route to default by sound_server earjack plugin");

	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

    
	avsys_audio_get_path_ex(&gain, &out, &in, &option);
	debug_msg ("gain = %x, out = %x, in = %x, option = %x\n", gain, out, in, option);
	if (gain == AVSYS_AUDIO_GAIN_EX_FMRADIO) { 
		debug_msg ("This is FM radio gain mode.....Nothing to do with sound path\n");
	} else {
                                                                                                       
		ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_KEYTONE, AVSYS_AUDIO_PATH_EX_SPK, AVSYS_AUDIO_PATH_EX_NONE, codec_option);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set playback sound path 0x%x\n", ret);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}
			return ret;
		}
		ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_VOICEREC, AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_EX_MIC, codec_option);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set capture sound path 0x%x\n", ret);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}
				return ret;
		}
	}

	ret = avsys_audio_set_route_policy(AVSYS_AUDIO_ROUTE_POLICY_DEFAULT);
	if(AVSYS_FAIL(ret)) {
		debug_error("Can not set route policy to avsystem 0x%x\n", ret);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return ret;
	}

	ret = vconf_set_int(ROUTE_VCONF_KEY, (int)AVSYS_AUDIO_ROUTE_POLICY_DEFAULT);
	if(ret < 0) {
		debug_error("Can not set route policy to vconf %s\n", ROUTE_VCONF_KEY);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return MM_ERROR_SOUND_INTERNAL;
	}

	if(MM_ERROR_NONE != __mm_sound_unlock()) {
		debug_error("Unlock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

static void
card_subscribe_cb (pa_context * c,
    pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
	const char* api_name = NULL;

	debug_msg (">>>>>>>>> [%s][%d] type=(0x%x) idx=(%u)\n", __func__, __LINE__,  t, idx);

	if ((t &  PA_SUBSCRIPTION_EVENT_FACILITY_MASK) != PA_SUBSCRIPTION_EVENT_CARD) {
		debug_msg ("[%s][%d] type=(0x%x) idx=(%u) is not sink event, skip...\n", __func__, __LINE__,  t, idx);
		return;
	}

	if ( (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_REMOVE &&
		 (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_NEW) {
		debug_msg ("[%s][%d] type=(0x%x) idx=(%u) is not sink (remove/new) event, skip...\n", __func__, __LINE__,  t, idx);
		return;
	}

	/* FIXME: We assumed that card is bt, card new/remove = bt new/remove */

	if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
		/* BT is removed */

		/* Check current policy */
		avsys_audio_route_policy_t cur_policy;
		if (avsys_audio_get_route_policy(&cur_policy) != AVSYS_STATE_SUCCESS) {
			debug_error ("avsys_audio_get_route_policy() failed");
			return;
		}

		/* We Do pause if policy is default, if not, do nothing */
		if (cur_policy == AVSYS_AUDIO_ROUTE_POLICY_DEFAULT)	{
			bool is_alsa_loud = false;

			 /* Do pause here */
			debug_msg("Do pause here");

			/* If no asm handle register here */
			if (g_asm_handle ==  -1) {
				debug_msg ("ASM handle is not valid, try to register once more\n");
				/* This register should be success */
				if (_asm_register_for_headset (&g_asm_handle)) {
					debug_msg("_asm_register_for_headset() success\n");
				} else {
					debug_error("_asm_register_for_headset() failed\n");
				}
			}

			//do pause
			debug_warning("Send earphone unplug event to Audio Session Manager Server for BT headset\n");
			_asm_pause_process(g_asm_handle);

			//update playing device info
			if(AVSYS_FAIL(avsys_audio_path_check_loud(&is_alsa_loud))) {
				debug_error("avsys_audio_path_check_loud() failed");
			}
			g_device = (is_alsa_loud)? AVSYS_AUDIO_ROUTE_DEVICE_HANDSET : AVSYS_AUDIO_ROUTE_DEVICE_EARPHONE;
		} else {
			debug_msg("Policy is not default, Do nothing");
		}
	} else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) {
		/* BT is loaded */

		/* Restore audio route policy to default */
		if(MM_ERROR_NONE != _set_audio_route_to_default()) {
			debug_error("set_audio_route_to_default() failed\n");
		} else {
			debug_msg("set_audio_route_to_default() done.\n");
			g_device = AVSYS_AUDIO_ROUTE_DEVICE_BLUETOOTH;
		}
	} /* if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) */
}

void sink_state_cb (pa_context *c, void *userdata)
{
	pa_threaded_mainloop *mainloop = (pa_threaded_mainloop*)userdata;
	assert(mainloop);
	assert(c);

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			break;

		case PA_CONTEXT_READY:
		{
			 /* Do CARD Subscribe */
			pa_context_set_subscribe_callback(c, card_subscribe_cb, NULL);

			pa_operation *o;
			if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)PA_SUBSCRIPTION_MASK_CARD, NULL, NULL))) {
				return;
			}
			pa_operation_unref(o);

			/* Signal */
			debug_msg ("signaling--------------\n");
			pa_threaded_mainloop_signal (mainloop, 0);

			break;
		}
	}

	return;
}

typedef struct _sinkinfo_struct
{
	pa_threaded_mainloop *mainloop;
	int is_speaker_on;
	int is_headset_on;
	int is_bt_on;
	int route_to;
	char speaker_name[256];
	/* Add headset_name to handle the instance that headset and speaker are two separate alsa sinks */
	/* And we should switch speaker/headset sinks manually */
	char headset_name[256];
	char bt_name[256];

} sinkinfo_struct;

void __get_sink_info_callback(pa_context *c, const pa_sink_info *i, int is_last, void *userdata)
{
	sinkinfo_struct *ss = (sinkinfo_struct *)userdata;

   if (is_last < 0) {
	   debug_error("Failed to get sink information: %s\n", pa_strerror(pa_context_errno(c)));
	   debug_error("Error to MM_SOUND_MSG_REQ_SET_AUDIO_ROUTE.\n");
   }

   if (is_last) {
	   /* Change default sink which we want to */
           /* if route_to == PULSE_DEFAULT_SINK_ALSA (earjack is inserted) */
	   /* switch to headset sink if it exists, else switch to speaker sink */
	   if (ss->route_to == 0 && ss->is_headset_on) {
		   debug_error ("Trying to set default sink to [%s] \n", ss->headset_name);
		   pa_operation_unref(pa_context_set_default_sink(c, ss->headset_name, NULL, NULL));
	   } else if (ss->route_to == 0 && ss->is_speaker_on ) {
		   debug_error ("Trying to set default sink to [%s] \n", ss->speaker_name);
		   pa_operation_unref(pa_context_set_default_sink(c, ss->speaker_name, NULL, NULL));
	   /* if route_to == PULSE_DEFAULT_SINK_BT (earjack is ejected) */ 
	   /* switch to bt sink if it exists, else switch to speaker sink if speaker/headset sinks should be switched manually */
	   } else if (ss->route_to == 1 && ss->is_bt_on) {
		   debug_error ("Trying to set default sink to [%s] \n", ss->bt_name);
		   pa_operation_unref(pa_context_set_default_sink(c, ss->bt_name, NULL, NULL));
	   } else if (ss->route_to == 1 && ss->is_headset_on && ss->is_speaker_on) {
		   debug_error ("Trying to set default sink to [%s] \n", ss->speaker_name);
		   pa_operation_unref(pa_context_set_default_sink(c, ss->speaker_name, NULL, NULL));
	   } else {
		   debug_error ("No match for [%d] \n", ss->route_to);
	   }

		/* Signal */
		debug_msg ("signaling--------------\n");
		pa_threaded_mainloop_signal (ss->mainloop, 0);
		return;
   }

   if (i->name) {
		debug_error("sink name = [%s]\n", i->name);
		/* Check the sink name for headset if it has been loaded in pulseaudio */
		if (strstr (i->name, "headset")) {
			ss->is_headset_on = 1;
			strncpy (ss->headset_name, i->name, sizeof(ss->headset_name)-1);
		/* If sink for headset is not defined, set the default alsa sink as speaker sink */
		} else if (strstr (i->name, "alsa_")) {
			ss->is_speaker_on = 1;
			strncpy (ss->speaker_name, i->name, sizeof(ss->speaker_name)-1);
		} else if (strstr (i->name, "bluez")) {
			ss->is_bt_on = 1;
			strncpy (ss->bt_name, i->name, sizeof(ss->bt_name)-1);
		} else
	   		debug_error("Unknown sink name!!!\n");
   }

}

int __set_audio_route (pa_threaded_mainloop *mainloop, pa_context *context, int route)
{
	debug_enter("\n");
	int ret = MM_ERROR_NONE;
	int current_route = 0;
	int policy_to_set;

	sinkinfo_struct *ss = malloc (sizeof (sinkinfo_struct)); /* this will be freed end of this function */
	memset (ss, 0, sizeof(sinkinfo_struct));
	/* parament set */
	ss->mainloop = mainloop;
	ss->route_to = route; // 0=alsa 1=bt
	ss->is_speaker_on = ss->is_headset_on = ss->is_bt_on = 0;

	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	/* vconf check */
	ret = vconf_get_int(ROUTE_VCONF_KEY, &current_route);
	if(ret < 0) {
		debug_error("Can not get current route policy\n");
		current_route = SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT;
		if(0 > vconf_set_int(ROUTE_VCONF_KEY, current_route)) {
			debug_error("Can not save current audio route policy to %s\n", ROUTE_VCONF_KEY);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}
			return MM_ERROR_SOUND_INTERNAL;
		}
	}

	/* Do Pulseaudio works : start  */
    pa_threaded_mainloop_lock(mainloop);
	/* Do async pulse operation */
	pa_operation_unref(pa_context_get_sink_info_list(context, __get_sink_info_callback, ss));

    /* Wait until the context is ready */
    pa_threaded_mainloop_wait(mainloop);
    pa_threaded_mainloop_unlock(mainloop);
	/* Do Pulseaudio works : end  */

	/* Sound path for ALSA */
	if(route == PULSE_DEFAULT_SINK_ALSA)
	{
		ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_KEYTONE, AVSYS_AUDIO_PATH_EX_SPK, AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_OPTION_JACK_AUTO);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set playback sound path 0x%x\n", ret);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}    
			return ret; 
		}    
		ret = avsys_audio_set_path_ex(AVSYS_AUDIO_GAIN_EX_VOICEREC, AVSYS_AUDIO_PATH_EX_NONE, AVSYS_AUDIO_PATH_EX_MIC, AVSYS_AUDIO_PATH_OPTION_JACK_AUTO);
		if(AVSYS_FAIL(ret)) {
			debug_error("Can not set capture sound path 0x%x\n", ret);
			if(MM_ERROR_NONE != __mm_sound_unlock()) {
				debug_error("Unlock failed\n");
				return MM_ERROR_SOUND_INTERNAL;
			}    
			return ret; 
		}    
	}    


	/* set policy to avsystem & vconf : start */ 
	/* select proper policy */
	if (route == PULSE_DEFAULT_SINK_ALSA && ss->is_bt_on == 1) {
		/* default sink changed to alsa successful but bt exists, this means policy is IGNORE A2DP */
		policy_to_set = AVSYS_AUDIO_ROUTE_POLICY_IGNORE_A2DP;
	} else {
		/* otherwise set to DEFAULT policy */
		policy_to_set = AVSYS_AUDIO_ROUTE_POLICY_DEFAULT;
	}

	/* set avsys policy */
	ret = avsys_audio_set_route_policy(policy_to_set);
	if(AVSYS_FAIL(ret)) {
		debug_error("Can not set route policy to avsystem 0x%x\n", ret);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
	}

	/* vconf */
	ret = vconf_set_int(ROUTE_VCONF_KEY, policy_to_set);
	if(ret < 0) {
		debug_error("Can not set route policy to vconf %s\n", ROUTE_VCONF_KEY);
		if(MM_ERROR_NONE != __mm_sound_unlock()) {
			debug_error("Unlock failed\n");
			return MM_ERROR_SOUND_INTERNAL;
		}
		return MM_ERROR_SOUND_INTERNAL;
	}
	if(MM_ERROR_NONE != __mm_sound_unlock()) {
		debug_error("Unlock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}
	/* set policy to avssytem & vconf : end */

	/* clean up */
	if (ss)	{
		free (ss);
		ss = NULL;
	}

	debug_leave("\n");
	return ret;
}


static int (*g_thread_pool_func)(void*, void (*)(void*)) = NULL;

int MMSoundPlugRunHeadsetControlRun(void)
{
	int current_type = 0;
	int new_type = 0;
	int waitfd = 0;
	int err = AVSYS_STATE_SUCCESS;
	int error = PA_ERR_INTERNAL;
	int asm_error = 0;
	int eject_event_count = 0;
	int need_mute = 0;
	pa_threaded_mainloop *mainloop = NULL;
	pa_context *context = NULL;

	g_asm_handle = -1;
	g_device = AVSYS_AUDIO_ROUTE_DEVICE_UNKNOWN;

	if(MM_ERROR_NONE != (err = _update_route_policy_from_vconf()))
		return err;

	if(MM_ERROR_NONE != (err = _set_audio_route_to_default()))
		return err;

	if(AVSYS_FAIL(avsys_audio_earjack_manager_init(&current_type, &waitfd)))
		return MM_ERROR_SOUND_DEVICE_NOT_OPENED;

	if(current_type > 0)
		g_device= AVSYS_AUDIO_ROUTE_DEVICE_EARPHONE;
	else if(current_type == 0)
		g_device = AVSYS_AUDIO_ROUTE_DEVICE_HANDSET;

	//register pulseaudio event subscribe callback here
    if (!(mainloop = pa_threaded_mainloop_new())) {
    	debug_error("pa_threaded_mainloop_new failed\n");
        goto fail;
    }

    if (!(context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), NULL))) {
    	debug_error("pa_context_new failed\n")
        goto fail;
    }

    pa_context_set_state_callback(context, sink_state_cb, (void*)mainloop);

    if (pa_context_connect(context, NULL, 0, NULL) < 0) {
        error = pa_context_errno(context);
        goto fail;
    }

    pa_threaded_mainloop_lock(mainloop);

    if (pa_threaded_mainloop_start(mainloop) < 0) {
    	debug_error("pa_threaded_mainloop_start failed\n");
        goto unlock_and_fail;
    }

    debug_msg("pa_threaded_mainloop_start() success");

    for (;;) {
        pa_context_state_t state;

        state = pa_context_get_state(context);

        if (state == PA_CONTEXT_READY)
            break;

        if (!PA_CONTEXT_IS_GOOD(state)) {
            error = pa_context_errno(context);
            debug_error("Connection with pa daemon failed %s\n", pa_strerror(error));
            goto unlock_and_fail;
        }

        /* Wait until the context is ready */
        pa_threaded_mainloop_wait(mainloop);
    }

    pa_threaded_mainloop_unlock(mainloop);

	//share information between earphone and bluetooth

	while(1)
	{
		//update playing device info
		if(AVSYS_FAIL(avsys_audio_get_playing_device_info(&g_device)))
			debug_error("AVSYS_FAILavsys_audio_get_playing_device_info() failed");

		//waiting earjack event
		err = avsys_audio_earjack_manager_wait(waitfd, &current_type, &new_type, &need_mute);
		debug_log ("wait result  = %x, current_type= %d, new_type = %d, need_mute = %d\n", err, current_type, new_type, need_mute);
		if(err & AVSYS_STATE_ERROR) {
#if !defined(_MMFW_I386_ALL_SIMULATOR)
			if(err != AVSYS_STATE_ERR_NULL_POINTER) {
				if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
					debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
					err = MM_ERROR_SOUND_INTERNAL;
					goto fail;
				}
			}
#endif
			break;
		} else if((err & AVSYS_STATE_WARING)) {
			if(err != AVSYS_STATE_WAR_INVALID_VALUE) {
				if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
					debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
					err = MM_ERROR_SOUND_INTERNAL;
					goto fail;
				}
			}
			continue; /* Ignore current changes and do wait again */
		}
		debug_warning("Current type is %d, New type is %d\n", current_type, new_type);

		if(current_type == new_type) {
			if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
				debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
				err = MM_ERROR_SOUND_INTERNAL;
				goto fail;
			}
			continue; /* Ignore current changes and do wait again */
		} else {
			current_type = new_type;
		}
		debug_warning("Current type is %d\n", current_type);

		/* mute if needed, unmute will be done end of this loop */
		if(need_mute) {
			//set global mute
			if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_MUTE_NOLOCK)))
				debug_error("Set mute failed\n");
		}

		//update playing device info
		if(AVSYS_FAIL(avsys_audio_get_playing_device_info(&g_device))) {
			debug_error("AVSYS_FAILavsys_audio_get_playing_device_info() failed");
		}
		debug_msg ("Current playing device was [%d]\n", g_device);

		//current_type '0' means earjack ejected
		if(current_type == EARJACK_EJECTED) {
			eject_event_count++;
			if(eject_event_count == 1) {
				debug_msg ("earjack [EJECTED]\n");
			
				avsys_audio_route_policy_t route_policy;
				avsys_audio_get_route_policy(&route_policy);
			
				debug_msg ("Current policy is [%d]\n", route_policy);
				if (g_device == AVSYS_AUDIO_ROUTE_DEVICE_EARPHONE) {
					debug_msg ("playing device was earjack while policy is not handset only : Do Pause!!!\n");
					if (g_asm_handle ==  -1) {
						debug_msg ("ASM handle is not valid, try to register once more\n");

						/* This register should be success */
						if (_asm_register_for_headset (&g_asm_handle)) {
							debug_msg("_asm_register_for_headset() success\n");
						} else {
							debug_error("_asm_register_for_headset() failed\n");
						}
					}

					debug_warning("Send earphone unplug event to Audio Session Manager Server\n");
					_asm_pause_process(g_asm_handle);

					/* FIXME: Check state */
					if (_is_normal_state ()) {
						debug_msg ("Playing device was disconnected, set to default and try to move bt if available\n");
						__set_audio_route(mainloop, context, PULSE_DEFAULT_SINK_BT);
					}
				}
			}
		} else if(current_type != EARJACK_EJECTED) { /* INSERT */
			debug_msg ("earjack is [INSERTED], device was = %d\n", g_device);
			eject_event_count = 0;

			/* FIXME: Check state */
			if (_is_normal_state ()) {
				__set_audio_route(mainloop, context, PULSE_DEFAULT_SINK_ALSA);
			}
		}

		//process change
		err = avsys_audio_earjack_manager_process(current_type);
		if(err & AVSYS_STATE_ERROR) {
			debug_error("Earjack Managing Fatal Error 0x%x\n", err);
			if(need_mute) {
				if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_UNMUTE_NOLOCK))) {
					debug_error("Set unmute failed\n");
				}
			}
#if !defined(_MMFW_I386_ALL_SIMULATOR)
			if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
				debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
				err = MM_ERROR_SOUND_INTERNAL;
				goto fail;
			}
#endif
			break;
		} else if(err & AVSYS_STATE_WARING) {
			debug_error("Earjack Managing Warning 0x%x\n", err);
			if(need_mute) {
				if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_UNMUTE_NOLOCK))) {
					debug_error("Set unmute failed\n");
				}
			}
			
			if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
				debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
				err = MM_ERROR_SOUND_INTERNAL;
				goto fail;
			}
			continue;
		}

		/* Unmute if muted */
		if(need_mute) {
			//global unmute
			if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_UNMUTE_NOLOCK))) {
				debug_error("Set unmute failed\n");
			}
		}
	
		if(AVSYS_FAIL(avsys_audio_earjack_manager_unlock())) {
			debug_error("avsys_audio_earjack_manager_unlock() failed in %s\n",__func__);
			err = MM_ERROR_SOUND_INTERNAL;
			goto fail;
		}
	} /* while (1) */

	if(!ASM_unregister_sound(g_asm_handle, ASM_EVENT_EARJACK_UNPLUG, &asm_error)) {
		debug_error("earjack event unregister failed with 0x%x\n", asm_error);
	}
	if(AVSYS_FAIL(avsys_audio_earjack_manager_deinit(waitfd))) {
		err = MM_ERROR_SOUND_INTERNAL;
		goto fail;
	}

	return MM_ERROR_NONE;

unlock_and_fail:
	debug_error("earjack plugin unlock_and_fail\n");
	pa_threaded_mainloop_unlock(mainloop);

	if(mainloop)
		pa_threaded_mainloop_stop(mainloop);

	if(context)
		pa_context_disconnect(context);
fail:
	debug_error("earjack plugin fail\n");
	if(context)
		pa_context_unref(context);

	if(mainloop)
		pa_threaded_mainloop_free(mainloop);

	debug_error("earphone plugin exit with 0x%x\n", err);

	return err;
}

int MMSoundPlugRunHeadsetControlStop(void)
{
	; /* No impl. Don`t stop */
	return MM_ERROR_NONE;	
}

int MMSoundPlugRunHeadsetSetThreadPool(int (*func)(void*, void (*)(void*)))
{
	debug_enter("(func : %p)\n", func);
	g_thread_pool_func = func;
	debug_leave("\n");
	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundPlugRunGetInterface(mmsound_run_interface_t *intf)
{
	debug_enter("\n");
	intf->run = MMSoundPlugRunHeadsetControlRun;
	intf->stop = MMSoundPlugRunHeadsetControlStop;
	intf->SetThreadPool = NULL;
	debug_leave("\n");

	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundGetPluginType(void)
{
	debug_enter("\n");
	debug_leave("\n");
	return MM_SOUND_PLUGIN_TYPE_RUN;
}

