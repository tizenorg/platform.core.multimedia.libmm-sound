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

#include <vconf.h>
#include <mm_error.h>
#include <mm_debug.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>


#include "../include/mm_sound_common.h"
#include "../include/mm_sound_utils.h"
#include "include/mm_sound_mgr_focus.h"
#include "include/mm_sound_mgr_focus_ipc.h"
#include "include/mm_sound_mgr_focus_dbus.h"

#include <glib.h>

#define USE_SYSTEM_SERVER_PROCESS_MONITORING

typedef struct {
    int startserver;
    int printlist;
    int testmode;
} server_arg;

static int get_option(int argc, char **argv, server_arg *arg);
static int usage(int argc, char **argv);

static struct sigaction sigint_action;  /* Backup pointer of SIGINT handler */
static struct sigaction sigabrt_action; /* Backup pointer of SIGABRT signal handler */
static struct sigaction sigsegv_action; /* Backup pointer of SIGSEGV fault signal handler */
static struct sigaction sigterm_action; /* Backup pointer of SIGTERM signal handler */
static struct sigaction sigsys_action;  /* Backup pointer of SIGSYS signal handler */
static void _exit_handler(int sig);

GMainLoop *g_mainloop;

void* pulse_handle;

void mainloop_run()
{
	g_mainloop = g_main_loop_new(NULL, TRUE);
	if(g_mainloop == NULL) {
		debug_error("g_main_loop_new() failed\n");
	}
	g_main_loop_run(g_mainloop);
}

int main(int argc, char **argv)
{
	server_arg serveropt;
	struct sigaction action;
#if !defined(USE_SYSTEM_SERVER_PROCESS_MONITORING)
	int pid;
#endif

	action.sa_handler = _exit_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);

	if (get_option(argc, argv, &serveropt))
		return 1;

	debug_warning("focus_server [%d] init \n", getpid());

	/* Daemon process create */
	if (!serveropt.testmode && serveropt.startserver) {
#if !defined(USE_SYSTEM_SERVER_PROCESS_MONITORING)
		daemon(0,0); //chdir to ("/"), and close stdio
#endif
	}

	/* focus Server Starts!!!*/
	debug_warning("focus_server [%d] start \n", getpid());

	signal(SIGPIPE, SIG_IGN); //ignore SIGPIPE

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

	if (serveropt.startserver) {
		MMSoundMgrFocusDbusInit();
		MMSoundMgrFocusInit();
	}

	debug_warning("focus_server [%d] initialization complete...now, start running!!\n", getpid());

	if (serveropt.startserver) {
//		unlink(PA_READY); // remove pa_ready file after focus-server init.

		mainloop_run();
	}

	debug_warning("focus_server [%d] terminating \n", getpid());

	if (serveropt.startserver) {
		MMSoundMgrFocusDbusFini();
		MMSoundMgrFocusFini();
	}

	debug_warning("focus_server [%d] exit ----------------- END \n", getpid());

	return 0;
}

static int get_option(int argc, char **argv, server_arg *arg)
{
	int c;
	static struct option long_options[] = {
		{"start", 0, 0, 'S'},
		{"help", 0, 0, 'H'},
		{"testmode", 0, 0, 'T'},
		{0, 0, 0, 0}
	};
	memset(arg, 0, sizeof(server_arg));

	arg->testmode = 0;

	while (1)
	{
		int opt_idx = 0;

		c = getopt_long (argc, argv, "SFLHRUP:Tiurd", long_options, &opt_idx);
		if (c == -1)
			break;
		switch (c)
		{
		case 'S': /* Start daemon */
			arg->startserver = 1;
			break;
		case 'T': /* Test mode */
			arg->testmode = 1;
			break;
		case 'H': /* help msg */
		default:
		return usage(argc, argv);
		}
	}
	if (argc == 1)
		return usage(argc, argv);
	return 0;
}

//__attribute__ ((destructor))
static void _exit_handler(int sig)
{
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

static int usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [Options]\n", argv[0]);
	fprintf(stderr, "\t%-20s: start focus server.\n", "--start,-S");
	fprintf(stderr, "\t%-20s: help message.\n", "--help,-H");

	return 1;
}

