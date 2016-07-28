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
#include <unistd.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <mm_sound_thread_pool.h>

#include <glib.h>

GThreadPool *g_pool;

#define MAX_UNUSED_THREADS_IN_THREADPOOL	10

/* FIXME : this is 1 because of keytone plugin which is always on running...
            if we move keytone pluging to other place, then we can assume 0 */
#define MIN_RUNNING_THREAD 0

typedef struct __THREAD_INFO
{
	void (*func)(gpointer data);
	void *param;
} THREAD_INFO;

static void __DummyWork(void *param)
{
	debug_msg("thread index = %d\n", (int)param);
	sleep(1);
}

static void __ThreadWork(gpointer data, gpointer user_data)
{
	THREAD_INFO *info = (THREAD_INFO *)data;
	if (info) {
		if (info->func) {
			debug_log("Calling [%p] with param [%p]\n", info->func, info->param);
			info->func(info->param);
		} else {
			debug_warning("No func to call....\n");
		}

		/* Info was allocated by MMSoundThreadPoolRun().
			The actual content of info should be  freed whether inside func or outside (if handle) */
		debug_log ("free [%p]\n", info);
		free(info);
		info = NULL;
	} else {
		debug_warning("No valid thread info...Nothing to do...\n");
	}
}

int MMSoundThreadPoolDump(int fulldump)
{
	if (g_pool == NULL) {
		debug_error("No thread pool initialized....\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if (fulldump) {
		debug_log("##### [ThreadPool] max threads=[%d], max unused=[%d], max idle time=[%d]\n",
				g_thread_pool_get_max_threads (g_pool),
				g_thread_pool_get_max_unused_threads(),
				g_thread_pool_get_max_idle_time());
	}
	debug_log("***** [ThreadPool] running=[%d], unused=[%d]\n",
			g_thread_pool_get_num_threads (g_pool),
			g_thread_pool_get_num_unused_threads());

	return MM_ERROR_NONE;
}

gboolean IsMMSoundThreadPoolRunning(void)
{
	MMSoundThreadPoolDump(FALSE);
	return (g_thread_pool_get_num_threads(g_pool) > MIN_RUNNING_THREAD);
}

int MMSoundThreadPoolInit()
{
	int i=0;
	GError *error = NULL;

	/* Create thread pool (non-exclude mode with infinite max threads) */
	g_pool = g_thread_pool_new(__ThreadWork, NULL, -1, FALSE, &error);
	if (g_pool == NULL && error != NULL) {
		debug_error("thread pool created failed : %s\n", error->message);
		g_error_free (error);
		return MM_ERROR_SOUND_INTERNAL;
	}
	debug_msg ("thread pool created successfully\n");

	MMSoundThreadPoolDump(TRUE);

	/* Thread pool setting : this will maintain at least 10 unused threads and this will be reused. */
	/* If no unused thread left, new thread will be created, but always maintain 10 unused thread */
	debug_msg ("thread pool set max unused threads to %d\n", MAX_UNUSED_THREADS_IN_THREADPOOL);
	g_thread_pool_set_max_unused_threads(MAX_UNUSED_THREADS_IN_THREADPOOL);

	/* To reserve unused threads, let's start some threads for beginning
		his dummy thread will be remained unused as soon as it started */
	debug_msg("run threads to reserve minimum thread\n");
	for (i = 0; i < MAX_UNUSED_THREADS_IN_THREADPOOL; i++) {
		MMSoundThreadPoolRun((void *)i, __DummyWork);
	}

	MMSoundThreadPoolDump(TRUE);

	return MM_ERROR_NONE;
}

int MMSoundThreadPoolRun(void *param, void (*func)(void*))
{
	GError *error = NULL;

	/* Dump current thread pool */
	MMSoundThreadPoolDump(FALSE);

	/* Create thread info structure.
	   This thread info data will be free in __ThreadWork(), after use. */
	THREAD_INFO *thread_info = (THREAD_INFO *)malloc(sizeof(THREAD_INFO));
	if (thread_info) {
		thread_info->func = func;
		thread_info->param = param;
		debug_log("alloc thread_info = %p\n", thread_info);

		/* Add thread to queue of thread pool */
		g_thread_pool_push(g_pool, thread_info, &error);
		if (error) {
			debug_error("g_thread_pool_push failed : %s\n", error->message);
			g_error_free(error);
			free(thread_info);
			return MM_ERROR_SOUND_INTERNAL;
		}
	} else {
		debug_error("failed to alloc thread info\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

int MMSoundThreadPoolFini(void)
{
	/* If immediate is TRUE, no new task is processed for pool.
	Otherwise pool is not freed before the last task is processed.
	Note however, that no thread of this pool is interrupted, while processing a task.
	Instead at least all still running threads can finish their tasks before the pool is freed.

	If wait_ is TRUE, the functions does not return before all tasks to be processed
	(dependent on immediate, whether all or only the currently running) are ready.
	Otherwise the function returns immediately. */
	debug_msg("thread pool will be free\n");
	g_thread_pool_free(g_pool, TRUE, FALSE);

	return MM_ERROR_NONE;
}
