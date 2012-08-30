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
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>

#include <mm_error.h>
#include <mm_debug.h>
#include "include/mm_sound_thread_pool.h"
#include "include/mm_sound_mgr_run.h"
#include "include/mm_sound_mgr_codec.h"
#include "include/mm_sound_mgr_ipc.h"
#include "include/mm_sound_mgr_pulse.h"
#include "include/mm_sound_mgr_asm.h"
#include "include/mm_sound_mgr_session.h"
#include "include/mm_sound_mgr_headset.h"
#include "include/mm_sound_mgr_dock.h"
#include "include/mm_sound_recovery.h"
#include "include/mm_sound_utils.h"
#include "include/mm_sound_common.h"

#include <heynoti.h>

#include <glib.h>

#define PLUGIN_ENV "MM_SOUND_PLUGIN_PATH"
#define PLUGIN_DIR "/usr/lib/soundplugins/"
#define PLUGIN_MAX 30

#define HIBERNATION_SOUND_CHECK_PATH	"/tmp/hibernation/sound_ready"
#define USE_SYSTEM_SERVER_PROCESS_MONITORING

#define	ASM_CHECK_INTERVAL	10000

typedef struct {
    char *plugdir;
    int startserver;
    int printlist;
    int testmode;
} server_arg;

static int getOption(int argc, char **argv, server_arg *arg);
static int usgae(int argc, char **argv);

static struct sigaction sigint_action;  /* Backup pointer of SIGINT handler */
static struct sigaction sigabrt_action; /* Backup pointer of SIGABRT signal handler */
static struct sigaction sigsegv_action; /* Backup pointer of SIGSEGV fault signal handler */
static struct sigaction sigterm_action; /* Backup pointer of SIGTERM signal handler */
static struct sigaction sigsys_action;  /* Backup pointer of SIGSYS signal handler */
static void _exit_handler(int sig);

GMainLoop *g_mainloop;

void* pulse_handle;

GThreadFunc event_loop_thread(gpointer data)
{
	g_mainloop = g_main_loop_new(NULL, TRUE);
	if(g_mainloop == NULL) {
		debug_error("g_main_loop_new() failed\n");
	}
	g_main_loop_run(g_mainloop);
	return NULL;
}

void hibernation_leave_cb()
{
	MMSoundMgrPulseHandleRegisterMonoAudio(pulse_handle);
	MMSoundMgrPulseHandleRegisterBluetoothStatus (pulse_handle);

	if(sound_system_bootup_recovery()) {
		debug_error("Audio reset failed\n");
	} else {
		debug_msg("Audio reset success\n");
	}
}

void wait_for_asm_ready ()
{
	int retry_count = 0;
	int asm_ready = 0;
	while (!asm_ready) {
		debug_log("Checking ASM ready....[%d]\n", retry_count++);
		if (vconf_get_int(ASM_READY_KEY, &asm_ready)) {
			debug_warning("vconf_get_int for ASM_READY_KEY (%s) failed\n", ASM_READY_KEY);
		}
		usleep (ASM_CHECK_INTERVAL);
	}
	debug_log("ASM is now ready...clear key!!!\n");
	vconf_unset (ASM_READY_KEY);
}

int main(int argc, char **argv)
{
	server_arg serveropt;
	struct sigaction action;
	int ret;
	int heynotifd = -1;


	action.sa_handler = _exit_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);

#if !defined(USE_SYSTEM_SERVER_PROCESS_MONITORING)
	int pid;
#endif

	if (getOption(argc, argv, &serveropt))
		return 1;

	/* Daemon process create */
	if (!serveropt.testmode && serveropt.startserver) {
#if !defined(USE_SYSTEM_SERVER_PROCESS_MONITORING)
		daemon(0,0); //chdir to ("/"), and close stdio
#endif
	}

	signal(SIGPIPE, SIG_IGN); //ignore SIGPIPE
	/*
	 * Set oom_adj lowest 2010.03.09
	 */

	if(sound_system_bootup_recovery()) {
		debug_error("Audio reset failed\n");
	} else {
		debug_msg("Audio reset success\n");
	}

	heynotifd = heynoti_init();
	if(heynoti_subscribe(heynotifd, "HIBERNATION_LEAVE", hibernation_leave_cb, NULL)) {
		debug_error("heynoti_subscribe failed...\n");
	} else {
		debug_msg("heynoti_subscribe() success\n");
	}

	if(heynoti_attach_handler(heynotifd)) {
		debug_error("heynoti_attach_handler() failed\n");
	} else {
		debug_msg("heynoti_attach_handler() success\n");
	}

#if !defined(USE_SYSTEM_SERVER_PROCESS_MONITORING)
	while(1)
	{
		if ((pid = fork()) < 0)
		{
			fprintf(stderr, "Sub Fork Error\n");
			return 2;
		}
		else if(pid == 0)
		{
			break;
		}
		else if(pid > 0)
		{
			wait(&ret);
			fprintf(stderr, "Killed by signal [%05X]\n", ret);	
			fprintf(stderr, "Daemon is run againg\n");
		}
	}
#endif
	sigaction(SIGABRT, &action, &sigabrt_action);
	sigaction(SIGSEGV, &action, &sigsegv_action);
	sigaction(SIGTERM, &action, &sigterm_action);
	sigaction(SIGSYS, &action, &sigsys_action);

	if (!g_thread_supported ())
		g_thread_init (NULL);

	if(NULL == g_thread_create(event_loop_thread, NULL, FALSE, NULL)) {
		fprintf(stderr,"event loop thread create failed\n");
		return 3;
	}


	if (serveropt.startserver || serveropt.printlist) {
		MMSoundThreadPoolInit();
		MMSoundMgrRunInit(serveropt.plugdir);
		MMSoundMgrCodecInit(serveropt.plugdir);
		if (!serveropt.testmode)
			MMSoundMgrIpcInit();

		pulse_handle = MMSoundMgrPulseInit();
		MMSoundMgrASMInit();
		/* Wait for ASM Ready */
		wait_for_asm_ready();
		_mm_sound_mgr_device_init();
		MMSoundMgrHeadsetInit();
		MMSoundMgrDockInit();
		MMSoundMgrSessionInit();
	}

	if (serveropt.startserver) {
		/* Start Run types */
		MMSoundMgrRunRunAll();

		/* set hibernation check */
		_mm_sound_check_hibernation (HIBERNATION_SOUND_CHECK_PATH);

		/* Start Ipc mgr */
		MMSoundMgrIpcReady();
	}

	if (serveropt.startserver || serveropt.printlist) {
		MMSoundMgrRunStopAll();
		if (!serveropt.testmode)
			MMSoundMgrIpcFini();

		MMSoundMgrCodecFini();
		MMSoundMgrRunFini();
		MMSoundThreadPoolFini();

		MMSoundMgrDockFini();
		MMSoundMgrHeadsetFini();
		MMSoundMgrSessionFini();
		_mm_sound_mgr_device_fini();
		MMSoundMgrASMFini();
		MMSoundMgrPulseFini(pulse_handle);

		if(heynoti_unsubscribe(heynotifd, "HIBERNATION_LEAVE", NULL)) {
			debug_error("heynoti_unsubscribe failed..\n");
		}
		heynoti_close(heynotifd);
	}
	return 0;
}

static int getOption(int argc, char **argv, server_arg *arg)
{
	int c;
	static struct option long_options[] = {
		{"start", 0, 0, 'S'},
		{"list", 0, 0, 'L'},
		{"help", 0, 0, 'H'},
		{"plugdir", 1, 0, 'P'},
		{"testmode", 0, 0, 'T'},
		{0, 0, 0, 0}
	};
	memset(arg, 0, sizeof(server_arg));

	arg->plugdir = getenv(PLUGIN_ENV);
	if (arg->plugdir == NULL)
		arg->plugdir = PLUGIN_DIR;
		
	arg->testmode = 0;

	while (1)
	{
		int opt_idx = 0;

		c = getopt_long (argc, argv, "SLHRP:T", long_options, &opt_idx);
		if (c == -1)
			break;
		switch (c)
		{
		case 'S': /* Start daemon */
			arg->startserver = 1;
			break;
		case 'L': /* list of plugins */
			arg->printlist = 1;
			break;
		case 'R':
			MMSoundMgrCodecInit(arg->plugdir);
			break;
		case 'P': /* Custom plugindir */
			arg->plugdir = optarg;
			break;
		case 'T': /* Test mode */
			arg->testmode = 1;
			break;
		case 'H': /* help msg */
		default:
		return usgae(argc, argv);
		}
	}
	if (argc == 1)
		return usgae(argc, argv);
	return 0;
}

//__attribute__ ((destructor))
static void _exit_handler(int sig)
{
	int ret = MM_ERROR_NONE;
	
	ret = MMSoundMgrRunStopAll();
	if (ret != MM_ERROR_NONE) {
		debug_error("Fail to stop run-plugin\n");
	} else {
		debug_log("All run-type plugin stopped\n");
	}

	switch(sig)
	{
	case SIGINT:
		sigaction(SIGINT, &sigint_action, NULL);
		debug_error("signal(SIGINT) error");
		break;
	case SIGABRT:
		sigaction(SIGABRT, &sigabrt_action, NULL);
		debug_error("signal(SIGABRT) error");
		break;
	case SIGSEGV:
		sigaction(SIGSEGV, &sigsegv_action, NULL);
		debug_error("signal(SIGSEGV) error");
		break;
	case SIGTERM:
		sigaction(SIGTERM, &sigterm_action, NULL);
		debug_error("signal(SIGTERM) error");
		break;
	case SIGSYS:
		sigaction(SIGSYS, &sigsys_action, NULL);
		debug_error("signal(SIGSYS) error");
		break;
	default:
		break;
	}
	raise(sig);
}

static int usgae(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [Options]\n", argv[0]);
	fprintf(stderr, "\t%-20s: start sound server.\n", "--start,-S");
	fprintf(stderr, "\t%-20s: print plugin list.\n", "--list,-L");
	fprintf(stderr, "\t%-20s: print this message.\n", "--help,-H");
	fprintf(stderr, "\t%-20s: print this message.\n", "--plugdir,-P");

	return 1;
}

