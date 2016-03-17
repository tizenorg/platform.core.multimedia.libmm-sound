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
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <vconf.h>

#include <sys/stat.h>
#include <errno.h>

#include <semaphore.h>
#include <errno.h>

#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_debug.h>
#include <mm_sound.h>
#include <mm_sound_private.h>

#define KEYTONE_PATH        "/tmp/keytone"  /* Keytone pipe path */
#define FILE_FULL_PATH 1024				/* File path lenth */
#define MAX_RETRY 40
#define RETRY_INTERVAL_USEC 50000

#define ROLE_NAME_LEN 64				/* Role name length */
#define VOLUME_GAIN_TYPE_LEN 64		/* Volume gain type length */

typedef struct {
	char filename[FILE_FULL_PATH];
	char role[ROLE_NAME_LEN];
	char volume_gain_type[VOLUME_GAIN_TYPE_LEN];
} ipc_t;

#define MMSOUND_STRNCPY(dst,src,size)\
do { \
	if(src != NULL && dst != NULL && size > 0) {\
		strncpy(dst,src,size-1); \
		dst[size-1] = '\0';\
	} else if(dst == NULL) {       \
		debug_error("STRNCPY ERROR: Destination String is NULL\n"); \
	}	\
	else if(size <= 0) {      \
		debug_error("STRNCPY ERROR: Destination String is NULL\n"); \
	}	\
	else {    \
		debug_error("STRNCPY ERROR: Destination String is NULL\n"); \
	}	\
} while(0)

EXPORT_API
int mm_sound_boot_ready(int timeout_sec)
{
	struct timespec ts;
	sem_t* sem = NULL;

	debug_msg("[BOOT] check for sync....");
	if ((sem = sem_open ("booting-sound", O_CREAT, 0660, 0))== SEM_FAILED) {
		debug_error ("error creating sem : %d", errno);
		return -1;
	}

	debug_msg("[BOOT] start to wait ready....timeout is set to %d sec", timeout_sec);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout_sec;

	if (sem_timedwait(sem, &ts) == -1) {
		if (errno == ETIMEDOUT)
			debug_warning("[BOOT] timeout!\n");
	} else {
		debug_msg("[BOOT] ready wait success!!!!");
		sem_post(sem);
	}

	return 0;
}

EXPORT_API
int mm_sound_boot_play_sound(char* path)
{
	int err = 0;
	int fd = -1;
	int size = 0;
	ipc_t data = {{0,},{0,},{0,}};

	debug_msg("[BOOT] play boot sound [%s]!!!!", path);
	if (path == NULL)
		return MM_ERROR_SOUND_INVALID_FILE;

	/* Check whether file exists */
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		char str_error[256];
		strerror_r(errno, str_error, sizeof(str_error));
		debug_error("file open failed with [%s][%d]\n", str_error, errno);
		switch (errno) {
		case ENOENT:
			return MM_ERROR_SOUND_FILE_NOT_FOUND;
		default:
			return MM_ERROR_SOUND_INTERNAL;
		}
	}
	close(fd);
	fd = -1;

	/* Open PIPE */
	fd = open(KEYTONE_PATH, O_WRONLY | O_NONBLOCK);
	if (fd == -1) {
		debug_error("Fail to open pipe\n");
		return MM_ERROR_SOUND_FILE_NOT_FOUND;
	}

	MMSOUND_STRNCPY(data.filename, path, FILE_FULL_PATH);
	MMSOUND_STRNCPY(data.role, "system", ROLE_NAME_LEN);
	MMSOUND_STRNCPY(data.volume_gain_type, "booting", VOLUME_GAIN_TYPE_LEN);

	debug_msg("filepath=[%s], role=[%s], volume_gain_type=[%s]\n", data.filename, data.role, data.volume_gain_type);
	size = sizeof(ipc_t);

	/* Write to PIPE */
	err = write(fd, &data, size);
	if (err < 0) {
		char str_error[256];
		strerror_r(errno, str_error, sizeof(str_error));
		debug_error("Fail to write data: [%s][%d]\n", str_error, errno);
		close(fd);
		return MM_ERROR_SOUND_INTERNAL;
	}
	/* Close PIPE */
	close(fd);

	return MM_ERROR_NONE;
}
