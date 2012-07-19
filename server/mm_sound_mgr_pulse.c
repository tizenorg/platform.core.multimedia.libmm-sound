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
#include "../include/mm_sound_msg.h"
#include "../include/mm_sound_common.h"

#include <mm_error.h>
#include <mm_debug.h>

#include <audio-session-manager.h>
#include <avsys-audio.h>

#include <pulse/ext-policy.h>

#define SUPPORT_MONO_AUDIO
//#define BT_DISCONNECT_PAUSE

#include "include/mm_sound_mgr_pulse.h"

#define SOUND_MSG_SET(sound_msg, x_msgtype, x_handle, x_code, x_msgid) \
do { \
	sound_msg.msgtype = x_msgtype; \
	sound_msg.handle = x_handle; \
	sound_msg.code = x_code; \
	sound_msg.msgid = x_msgid; \
} while(0)

pa_threaded_mainloop *g_m;
pa_context *g_context;

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;


#ifdef	BT_DISCONNECT_PAUSE
int g_asm_handle = -1;

bool _asm_register_for_bt ()
{
	int asm_error = 0;
	if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &asm_error))
	{
		debug_warning("earjack event register failed with 0x%x\n", asm_error);
		return false;
	}
	return true;
}


int set_audio_route_to_default()
{
	int ret = MM_ERROR_NONE;
	int codec_option = AVSYS_AUDIO_PATH_OPTION_JACK_AUTO;

	debug_msg("Set audio route to default by sound_server pulse mgr\n");

	if(MM_ERROR_NONE != __mm_sound_lock()) {
		debug_error("Lock failed\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

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

int process_asm_pause ()
{
	int asm_error = 0;

	// mute is not affecting now
	//if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_MUTE)))
	  //debug_error("Set unmute failed\n");

	if (g_asm_handle ==  -1) {
		debug_msg ("ASM handle is not valid, try to register once more\n");
		/* This register should be success */
		if (_asm_register_for_bt ()) {
			debug_msg("_asm_register_for_bt() success\n");
		} else {
			debug_error("_asm_register_for_bt() failed\n");
		}
	}

	if(!ASM_set_sound_state(g_asm_handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_PLAYING, ASM_RESOURCE_NONE, &asm_error ))
		debug_error("earjack event set sound state to playing failed with 0x%x\n", asm_error);

	if(!ASM_set_sound_state(g_asm_handle, ASM_EVENT_EARJACK_UNPLUG, ASM_STATE_STOP, ASM_RESOURCE_NONE, &asm_error ))
		debug_error("earjack event set sound state to stop failed with 0x%x\n", asm_error);

	//if(AVSYS_FAIL(avsys_audio_set_global_mute(AVSYS_AUDIO_UNMUTE)))
		//debug_error("Set unmute failed\n");
}

static void
context_subscribe_cb (pa_context * c,
    pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
    debug_msg (">>>>>>>>> [%s][%d] type=(%d) idx=(%u)\n", __func__, __LINE__,  t, idx);

    /* if event is "sink" index is not 0 (alsa) then bluetooth sink device is load/unloaded */
    /* FIXME : what if alsa index is not 0???? we have check more strictly*/
    if ((t &  PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK &&	idx != 0) {
   		if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) { /* unloaded */
			/* Check current policy */
	    	avsys_audio_route_policy_t cur_policy;
	    	if (avsys_audio_get_route_policy(&cur_policy) == AVSYS_STATE_SUCCESS) {
	    		/* We Do pause if policy is default, if not, do nothing */
				if (cur_policy == AVSYS_AUDIO_ROUTE_POLICY_DEFAULT)	{
					 /* Do pause here */
					debug_msg("Do pause here");
					process_asm_pause();
				} else {
					debug_msg("Policy is not default, Do nothing");
				}
	    	} else {
    			debug_error ("avsys_audio_get_route_policy() failed");
			}
		} else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) { /* loaded */
			/* Restore audio route policy to default */
			if(MM_ERROR_NONE != set_audio_route_to_default()) {
				debug_error("set_audio_route_to_default() failed\n");
			} else {
				debug_msg("set_audio_route_to_default() done.\n");
			}
		}
    }
}

#endif

void context_state_cb (pa_context *c, void *userdata)
{
	//g_print (">>>>>>>>> [%s][%d]\n", __func__, __LINE__);

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
			//g_print ("PA_CONTEXT_READY\n");

			 if (g_context == c) {

#ifdef	BT_DISCONNECT_PAUSE
				 /* Do SINK Subscribe */
				pa_context_set_subscribe_callback(c, context_subscribe_cb, NULL);

				pa_operation *o;
				if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)PA_SUBSCRIPTION_MASK_SINK, NULL, NULL))) {
					//g_print ("pa_context_subscribe() failed\n");
					return;
				}
				pa_operation_unref(o);
#else
				//pa_context_set_subscribe_callback(c, NULL, NULL);
#endif

				/* Signal */
				debug_msg ("signaling--------------\n");
				pa_threaded_mainloop_signal (g_m, 0);
			}
			break;
		}
	}

	return;
}


int pulse_init ()
{
	int res;

	debug_msg (">>>>>>>>> [%s][%d]\n", __func__, __LINE__);
#ifdef	BT_DISCONNECT_PAUSE
	/* Set audio route policy to default when sound server startup */
	if(MM_ERROR_NONE != set_audio_route_to_default())
	{
		debug_error("Set audio route policy to default failed\n");
	}
#endif
	/* Create new mainloop */
	g_m = pa_threaded_mainloop_new();
	//g_assert(g_m);

	res = pa_threaded_mainloop_start (g_m);
	//g_assert (res == 0);

	/* LOCK thread */
	pa_threaded_mainloop_lock (g_m);

	/* Get mainloop API */
	pa_mainloop_api *api = pa_threaded_mainloop_get_api(g_m);

	/* Create new Context */
	g_context = pa_context_new(api, "SOUND_SERVER_ROUTE_MANAGER");

	/* Set Callback */
	pa_context_set_state_callback (g_context, context_state_cb, NULL);

	/* Connect */
	if (pa_context_connect (g_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
		debug_error ("connection error\n");
	}

	for (;;) {
		pa_context_state_t state;

		state = pa_context_get_state (g_context);

		debug_msg ("context state is now %d\n", state);

		if (!PA_CONTEXT_IS_GOOD (state)) {
			debug_error ("connection failed\n");
			break;
		}

		if (state == PA_CONTEXT_READY)
			break;

		/* Wait until the context is ready */
		debug_msg ("waiting..................\n");
		pa_threaded_mainloop_wait (g_m);
	}

	/* UNLOCK thread */
	pa_threaded_mainloop_unlock (g_m);

	debug_msg ("<<<<<<<<<< [%s][%d]\n", __func__, __LINE__);

	return 0;
}

int pulse_deinit ()
{
	pa_threaded_mainloop_lock (g_m);

	if (g_context) {
		pa_context_disconnect (g_context);

		/* Make sure we don't get any further callbacks */
		pa_context_set_state_callback (g_context, NULL, NULL);

		pa_context_unref (g_context);
		g_context = NULL;
	}
	pa_threaded_mainloop_unlock (g_m);

	pa_threaded_mainloop_stop (g_m);
	pa_threaded_mainloop_free (g_m);

	debug_msg ("<<<<<<<<<< [%s][%d]\n", __func__, __LINE__);

	return 0;

}

void get_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
	int ret;
	mm_ipc_msg_t respmsg = {0,};
	server_struct *ss = (server_struct *)userdata;
	if (ss == NULL) {
		debug_error ("Input userdata is NULL\n");
		return;
	}

    if (!i) {
    	debug_error("Error to MM_SOUND_MSG_REQ_GET_AUDIO_ROUTE.\n");
		SOUND_MSG_SET(respmsg.sound_msg, MM_SOUND_MSG_RES_ERROR, -1, 0, ss->msg->sound_msg.msgid);
    } else {
		debug_msg ("We got default sink = [%s]\n", i->default_sink_name);
		SOUND_MSG_SET(respmsg.sound_msg, MM_SOUND_MSG_RES_GET_AUDIO_ROUTE, ss->msg->sound_msg.handle, 0, ss->msg->sound_msg.msgid);
		if (strstr (i->default_sink_name, "alsa"))
			respmsg.sound_msg.code = 0;
		else if (strstr (i->default_sink_name, "bluez"))
			respmsg.sound_msg.code = 1;
		else
			respmsg.sound_msg.code = 2;
    }

	if (ss->func) {
		ret = ss->func (&respmsg);
		if (ret != MM_ERROR_NONE) {
			debug_error ("Fail to send message [%x]\n", ret);
		} else {
			debug_msg("Sent msg to client msgid [%d] [codechandle %d][message type %d] (code 0x%08X)\n",
			  		ss->msg->sound_msg.msgid, respmsg.sound_msg.handle, respmsg.sound_msg.msgtype, respmsg.sound_msg.code);
		}
	} else {
		debug_error ("No send function\n");
	}

	debug_msg("Ready to next msg\n");
	if (ss->msg) {
		debug_msg ("free [%p]\n", ss->msg);
		free (ss->msg);
		ss->msg = NULL;
	}

	if (ss) {
		free (ss);
		ss = NULL;
	}
	pthread_mutex_unlock(&g_mutex);
}

void get_sink_info_callback(pa_context *c, const pa_sink_info *i, int is_last, void *userdata)
{
	int ret;
	mm_ipc_msg_t respmsg = {0,};
	sink_struct *ss = (sink_struct *)userdata;
	if (ss == NULL) {
		debug_error ("Input userdata is NULL\n");
		return;
	}

   if (is_last) {
		if (is_last < 0) { /* ERROR */ 
			/* Compose Error Response */
			debug_error("Failed to get sink information: %s\n", pa_strerror(pa_context_errno(c)));
			debug_error("Error to MM_SOUND_MSG_REQ_SET_AUDIO_ROUTE.\n");
			SOUND_MSG_SET(respmsg.sound_msg, MM_SOUND_MSG_RES_ERROR, -1, 0, ss->msg->sound_msg.msgid);
		} else { /* FINISHED WELL */
			/* Compose Response */
			SOUND_MSG_SET(respmsg.sound_msg, MM_SOUND_MSG_RES_SET_AUDIO_ROUTE, ss->msg->sound_msg.handle, ss->route_to, ss->msg->sound_msg.msgid);

			/* Do further action related with route request */
			/* ToDo : is this sync operation??? */
			if (ss->route_to == 0 && ss->is_speaker_on ) {
				/* route to speaker and speaker is on */
				debug_msg ("Trying to set default sink to [%s] \n", ss->speaker_name);
				pa_operation_unref(pa_context_set_default_sink(g_context, ss->speaker_name, NULL, NULL));
			} else if (ss->route_to == 1 && ss->is_bt_on) {
				/* route to bt and bt is on */
				debug_msg ("Trying to set default sink to [%s] \n", ss->bt_name);
				pa_operation_unref(pa_context_set_default_sink(g_context, ss->bt_name, NULL, NULL));
			} else if (ss->route_to == 2 && ss->is_headset_on) {
				/* route to headset and headset is on (PA loads 2 alsa sinks for speaker & headset device)*/
                                debug_msg ("Trying to set default sink to [%s] \n", ss->headset_name);
                                pa_operation_unref(pa_context_set_default_sink(g_context, ss->headset_name, NULL, NULL));
			} else if (ss->route_to == 2 && ss->is_speaker_on) {
                                /* route to headset and PA loads one alsa sink for speaker&headset devices*/
                                debug_msg ("Trying to set default sink to [%s] \n", ss->speaker_name);
                                pa_operation_unref(pa_context_set_default_sink(g_context, ss->speaker_name, NULL, NULL));
			} else {
				/* Nothing to do.... */
				debug_warning ("No match for [%d] \n", ss->route_to);
				respmsg.sound_msg.code = -1; // this means no exists
			}
		}
		
  		if (ss) { 
  			/* Send Response */
  			if (ss->func) {
  				ret = ss->func (&respmsg);
				if (ret != MM_ERROR_NONE) {
					debug_error ("Fail to send message \n");
				}
				debug_msg("Sent msg to client msgid [%d] [codechandle %d][message type %d] (code 0x%08X)\n",
								ss->msg->sound_msg.msgid, respmsg.sound_msg.handle, respmsg.sound_msg.msgtype, respmsg.sound_msg.code);
			} else {
				debug_error ("No send function\n");
			}

			/* Cleanup */
			if (ss->msg)	{
				debug_msg ("free [%p]\n", ss->msg);
				free (ss->msg);
				ss->msg = NULL;
			}
			free (ss);
			ss = NULL;
		}
  		debug_msg("Ready to next msg\n");
  		pthread_mutex_unlock(&g_mutex);
	   	   
	   return;
   }

   //pa_assert(i);

   if (i->name) {
		debug_msg("sink name = [%s]\n", i->name);	 
   
		if (strstr (i->name, "headset")) {
			ss->is_headset_on = 1;
			strncpy (ss->headset_name, i->name, sizeof(ss->headset_name)-1);
		} else if (strstr (i->name, "alsa_")) {
			ss->is_speaker_on = 1;
			strncpy (ss->speaker_name, i->name, sizeof(ss->speaker_name)-1);
		} else if (strstr (i->name, "bluez")) {
			ss->is_bt_on = 1;
			strncpy (ss->bt_name, i->name, sizeof(ss->bt_name)-1);
		} else
	   		debug_warning("Unknown sink name!!!\n");
   }
		   
}

void check_bt_sink_info_callback(pa_context *c, const pa_sink_info *i, int is_last, void *userdata)
{
	int ret;
	mm_ipc_msg_t respmsg = {0,};
	bt_struct *bs = (bt_struct *)userdata;
	if (bs == NULL) {
		debug_error ("Input userdata is NULL\n");
		return;
	}

	if (is_last) {
		if (is_last < 0) { /* ERROR */
			/* Compose Error Response */
			debug_error("Failed to get sink information: %s\n", pa_strerror(pa_context_errno(c)));	   
			debug_error("Error to MM_SOUND_MSG_RES_IS_BT_A2DP_ON.\n");
			SOUND_MSG_SET(respmsg.sound_msg, MM_SOUND_MSG_RES_ERROR, -1, 0, bs->msg->sound_msg.msgid);
		} else { /* FINISHED WELL */
			/* Compose Response */
			SOUND_MSG_SET(respmsg.sound_msg, 
							MM_SOUND_MSG_RES_IS_BT_A2DP_ON, bs->msg->sound_msg.handle, (bs->bt_found)? 1:0, bs->msg->sound_msg.msgid);
			strncpy (respmsg.sound_msg.filename, bs->bt_name, sizeof (respmsg.sound_msg.filename)-1);
		}  			   

		/* Send resone & clean up */
		if (bs) {
			/* Send Response */
			if (bs->func) {
				ret = bs->func (&respmsg);
				if (ret != MM_ERROR_NONE) {
					debug_error ("Fail to send message \n");
				} else {
					debug_msg("Sent msg to client msgid [%d] [codechandle %d][message type %d] (code 0x%08X)\n",
							bs->msg->sound_msg.msgid, respmsg.sound_msg.handle, respmsg.sound_msg.msgtype, respmsg.sound_msg.code);
				}
			} else {
				debug_error ("No sendfunc!!!!\n");
			}

			/* clean up */
			if (bs->msg) {
				debug_msg ("free [%p]\n", bs->msg);
				free (bs->msg);
				bs->msg = NULL;
			}
			free (bs);
			bs = NULL;
		}

		debug_msg("Ready to next msg\n");
		pthread_mutex_unlock(&g_mutex);
		return;
	}

   //pa_assert(i);

	if (i->name) {
		debug_msg("sink name = [%s]\n", i->name);	 
   
		if (strstr (i->name, "bluez")) {
			char* desc = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_DESCRIPTION);
			if (desc && strlen(desc)>0) {
				debug_msg ("sink device description = [%s]\n", desc);
				bs->bt_found = 1;
				strncpy (bs->bt_name, desc, strlen(desc));	
			} else {
				debug_warning ("No Description!!!!\n");				
			}			
		}				
	}
		   
}

int MMSoundMgrPulseHandleGetAudioRouteReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*))
{
	debug_enter("msg = %p , sendfunc = %p\n", msg, sendfunc);

	pthread_mutex_lock(&g_mutex);

	server_struct *ss = malloc (sizeof (server_struct));
	memset (ss, 0, sizeof (server_struct));

	/* common setting */
	ss->msg = msg;
	ss->func = sendfunc;

	/* Do async pulse operation */
	pa_operation_unref(pa_context_get_server_info(g_context, get_server_info_callback, ss));

	debug_leave("\n");
}

int MMSoundMgrPulseHandleSetAudioRouteReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*))
{
	debug_enter("\n");

	pthread_mutex_lock(&g_mutex);

	sink_struct *ss = malloc (sizeof (sink_struct));
	memset (ss, 0, sizeof(sink_struct));

	/* common setting */
	ss->msg = msg;
	ss->func = sendfunc;

	/* specific setting */
	ss->is_speaker_on = ss->is_bt_on = ss->is_headset_on = 0;
	ss->route_to =  msg->sound_msg.handle;

	/* Do async pulse operation */
	pa_operation_unref(pa_context_get_sink_info_list(g_context, get_sink_info_callback, ss));

	debug_leave("\n");
}

int MMSoundMgrPulseHandleIsBtA2DPOnReq (mm_ipc_msg_t *msg, int (*sendfunc)(mm_ipc_msg_t*))
{
	debug_enter("msg = %p, sendfunc = %p\n", msg, sendfunc);

	pthread_mutex_lock(&g_mutex);

	bt_struct *bs = malloc (sizeof(bt_struct));
	memset (bs, 0, sizeof (bt_struct));

	/* common setting */
	bs->msg = msg;
	bs->func = sendfunc;

	/* specific setting */
	bs->bt_found = 0;

	/* Do async pulse operation */
	pa_operation_unref(pa_context_get_sink_info_list(g_context, check_bt_sink_info_callback, bs));

	debug_leave("\n");
}

#ifdef SUPPORT_MONO_AUDIO
#define MONO_KEY "db/setting/accessibility/mono_audio"

void success_cb (pa_context *c, int success, void *userdata)
{
	debug_msg ("success = %d\n", success);
}

void mono_changed_cb(keynode_t* node, void* data)
{
	debug_msg ("%s changed callback called\n",vconf_keynode_get_name(node));

	int key_value;
	vconf_get_bool(MONO_KEY, &key_value);

	debug_msg ("key value = %d\n", key_value);

	pa_operation_unref (pa_ext_policy_set_mono (g_context, key_value, success_cb, NULL));
}

int MMSoundMgrPulseHandleRegisterMonoAudio ()
{
	int ret = vconf_notify_key_changed(MONO_KEY, mono_changed_cb, NULL);
	debug_enter ("vconf set ret = %d\n", ret);
	return ret;
}

#endif

int MMSoundMgrPulseInit(void)
{
	debug_enter("\n");

	pulse_init();

#ifdef SUPPORT_MONO_AUDIO
	MMSoundMgrPulseHandleRegisterMonoAudio();
#endif

#ifdef BT_DISCONNECT_PAUSE
	/* This registeration can be failed when hibernation capture, because of security server */
	_asm_register_for_bt ();

#endif
	debug_leave("\n");
	return MM_ERROR_NONE;
}

int MMSoundMgrPulseFini(void)
{
	debug_enter("\n");

	pulse_deinit();

#ifdef BT_DISCONNECT_PAUSE
	{
		int asm_error = 0;
		if(!ASM_unregister_sound(g_asm_handle, ASM_EVENT_EARJACK_UNPLUG, &asm_error))
		{
			debug_error("earjack event unregister failed with 0x%x\n", asm_error);
		}
	}
#endif

	debug_leave("\n");
	return MM_ERROR_NONE;
}

