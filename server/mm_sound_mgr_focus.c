/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Sangchul Lee <sc11.lee@samsung.com>
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

#include "include/mm_sound_mgr_focus.h"
#include "include/mm_sound_thread_pool.h"
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_stream.h"

#include <mm_error.h>
#include <mm_debug.h>
#include <glib.h>
#include <poll.h>
#include <fcntl.h>

#include "include/mm_sound_mgr_ipc.h"
#include "../include/mm_sound_utils.h"
#include <sys/time.h>
#include <sys/stat.h>

static GList *g_focus_node_list = NULL;
static pthread_mutex_t g_focus_node_list_mutex = PTHREAD_MUTEX_INITIALIZER;
stream_list_t g_stream_list;

static const char* focus_status_str[] =
{
	"DEACTIVATED",
	"P ACTIVATED",
	"C ACTIVATED",
	"B ACTIVATED",
};

typedef struct {
	int pid;
	int handle;
	int type;
	int state;
	char stream_type [MAX_STREAM_TYPE_LEN];
	char name [MM_SOUND_NAME_NUM];
}focus_cb_data;

#define CLEAR_DEAD_NODE_LIST(x)  do { \
	debug_warning ("list = %p, node = %p, pid=[%d]", x, node, (node)? node->pid : -1); \
	if (x && node && (_is_pid_exist(node->pid) == FALSE)) { \
		debug_warning("PID:%d does not exist now! remove from device cb list\n", node->pid); \
		g_free (node); \
		x = g_list_remove (x, node); \
	} \
}while(0)

#ifdef SUPPORT_CONTAINER
static void __set_container_data(int pid, int handle, const char* container_name, int container_pid)
{
	GList *list = NULL;
	focus_node_t *node = NULL;

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (!node->is_for_watch && node->pid == pid && node->handle_id == handle) {
			debug_error("Set container [%s][%d] to handle[%d] instanceID[%d]",
						container_name, container_pid, handle, node->pid);
			if (container_name)
				strcpy (node->container.name, container_name);
			node->container.pid = container_pid;
			break;
		}
		else if (node->is_for_watch && node->pid == pid) {
			debug_error("Set container [%s][%d] to instanceID[%d]",
						container_name, container_pid, pid);
			if (container_name)
				strcpy (node->container.name, container_name);
			node->container.pid = container_pid;
			break;
		}
	}
}

static container_info_t* __get_container_info(int instance_id)
{
	GList *list = NULL;
	focus_node_t *node = NULL;

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node->pid == instance_id) {
			return &node->container;
		}
	}
}
#endif /* SUPPORT_CONTAINER */

static char* __get_focus_pipe_path(int instance_id, int handle, const char* postfix, bool is_watch)
{
	gchar* path = NULL;
	gchar* path2 = NULL;

#ifdef SUPPORT_CONTAINER
	container_info_t* container_info = __get_container_info(instance_id);

	if (instance_id == container_info->pid) {
		debug_error ("This might be in the HOST(%s)[%d], let's form normal path",
					container_info->name, instance_id);
		if (is_watch) {
			path = g_strdup_printf("/tmp/FOCUS.%d.wch", instance_id);
		} else {
			path = g_strdup_printf("/tmp/FOCUS.%d.%d", instance_id, handle);
		}
	} else {
		if (is_watch) {
			path = g_strdup_printf("/var/lib/lxc/%s/rootfs/tmp/FOCUS.%d.wch",
									container_info->name, container_info->pid);
		} else {
			path = g_strdup_printf("/var/lib/lxc/%s/rootfs/tmp/FOCUS.%d.%d",
									container_info->name, container_info->pid, handle);
		}
	}
#else
	if (is_watch) {
		path = g_strdup_printf("/tmp/FOCUS.%d.wch", instance_id);
	} else {
		path = g_strdup_printf("/tmp/FOCUS.%d.%d", instance_id, handle);
	}
#endif

	if (postfix) {
		path2 = g_strconcat(path, postfix, NULL);
		g_free (path);
		path = NULL;
		return path2;
	}

	return path;
}


static gboolean _is_pid_exist(int pid)
{
	int ret = 0;
	gchar tmp[25] = { 0, };

	if (pid > 999999 || pid < 2) {
		debug_error ("invalid input pid [%d]", pid);
		return FALSE;
	}

	g_snprintf(tmp, sizeof(tmp), "/proc/%d", pid);
	if ((ret = access(tmp, R_OK)) != 0) {
		debug_log ("[%s] access error [%d]", ret);
		return FALSE;
	}

	return TRUE;
}

static void _clear_focus_node_list_func (focus_node_t *node, gpointer user_data)
{
	CLEAR_DEAD_NODE_LIST(g_focus_node_list);
}

static int _mm_sound_mgr_focus_get_priority_from_stream_type(int *priority, const char *stream_type)
{
	int ret = MM_ERROR_NONE;
	int i = 0;

	debug_fenter();

	if (priority == NULL || stream_type == NULL) {
		ret = MM_ERROR_INVALID_ARGUMENT;
		debug_error("invalid argument, priority[0x%x], stream_type[%s], ret[0x%x]\n", priority, stream_type, ret);
	} else {
		for (i = 0; i < AVAIL_STREAMS_MAX; i++) {
			if (!strncmp(g_stream_list.stream_types[i], stream_type, strlen(stream_type))) {
				*priority = g_stream_list.priorities[i];
				break;
			}
		}
		if (i == AVAIL_STREAMS_MAX) {
			ret = MM_ERROR_INVALID_ARGUMENT;
			debug_error("not supported stream_type[%s], ret[0x%x]\n", stream_type, ret);
		} else {
			debug_log("[%s] has priority of [%d]\n", stream_type, *priority);
		}
	}

	debug_fleave();
	return ret;
}

static int _mm_sound_mgr_focus_do_watch_callback(focus_type_e focus_type, focus_command_e command,
												focus_node_t *my_node, _mm_sound_mgr_focus_param_t *param)
{
	char *filename = NULL;
	char *filename2 = NULL;
	struct timeval time;
	int starttime = 0;
	int endtime = 0;
	int fd_FOCUS_R = -1;
	int fd_FOCUS = -1;
	int ret = -1;
	struct pollfd pfd;
	int pret = 0;
	int pollingTimeout = 2500; /* NOTE : This is temporary code, because of Deadlock issues. If you fix that issue, remove this comment */

	GList *list = NULL;
	focus_node_t *node = NULL;

	int i = 0;

	focus_cb_data cb_data;

	debug_fenter();

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node == my_node) {
			/* skip my own */
		} else {
			if (node->is_for_watch && (node->status & focus_type)) {
				memset(&cb_data, 0, sizeof(focus_cb_data));
				cb_data.pid = node->pid;
				cb_data.handle = node->handle_id;
				cb_data.type = focus_type & node->status;
				cb_data.state = (command == FOCUS_COMMAND_ACQUIRE) ? !FOCUS_STATUS_DEACTIVATED : FOCUS_STATUS_DEACTIVATED;
				MMSOUND_STRNCPY(cb_data.stream_type, my_node->stream_type, MAX_STREAM_TYPE_LEN);
				MMSOUND_STRNCPY(cb_data.name, param->option, MM_SOUND_NAME_NUM);

				/* Set start time */
				gettimeofday(&time, NULL);
				starttime = time.tv_sec * 1000000 + time.tv_usec;

				/**************************************
				 *
				 * Open callback cmd pipe
				 *
				 **************************************/
				filename = __get_focus_pipe_path(cb_data.pid, -1, NULL, true);
				if ((fd_FOCUS = open(filename, O_WRONLY|O_NONBLOCK)) == -1) {
					debug_error("[CallCB] %s open error\n", filename);
					goto fail;
				}

				/******************************************
				 *
				 * Open callback result pipe
				 * before writing callback cmd to pipe
				 *
				 ******************************************/
				 filename2 = __get_focus_pipe_path(cb_data.pid, -1, "r", true);
				if ((fd_FOCUS_R= open(filename2,O_RDONLY|O_NONBLOCK)) == -1) {
					char str_error[256];
					strerror_r (errno, str_error, sizeof(str_error));
					debug_error("[RETCB] Fail to open fifo (%s)\n", str_error);
					goto fail;
				}
				debug_log(" open return cb %s\n", filename2);

				/*******************************************
				 * Write Callback msg
				 *******************************************/
				if (write(fd_FOCUS, &cb_data ,sizeof(cb_data)) == -1) {
					debug_error("[CallCB] %s fprintf error\n", filename);
					goto fail;
				}

				/**************************************
				 *
				 * Close callback cmd pipe
				 *
				 **************************************/
				if (fd_FOCUS != -1) {
					close(fd_FOCUS);
					fd_FOCUS = -1;
				}
				g_free(filename);
				filename = NULL;

				pfd.fd = fd_FOCUS_R;
				pfd.events = POLLIN;

				/*********************************************
				 *
				 * Wait callback result msg
				 *
				 ********************************************/
				debug_error("[RETCB]wait callback(tid=%d, cmd=%d, timeout=%d)\n", cb_data.pid, command, pollingTimeout);
				pret = poll(&pfd, 1, pollingTimeout); /* timeout 7sec */
				debug_error("after poll");
				if (pret < 0) {
					debug_error("[RETCB]poll failed (%d)\n", pret);
					goto fail;
				}
				if (pfd.revents & POLLIN) {
					if (read(fd_FOCUS_R, &ret, sizeof(ret)) == -1) {
						debug_error("fscanf error\n");
						goto fail;
					}
				}

				g_free(filename2);
				filename2 = NULL;

				/* Calculate endtime and display*/
				gettimeofday(&time, NULL);
				endtime = time.tv_sec * 1000000 + time.tv_usec;
				debug_error("[RETCB] ASM_CB_END cbtimelab=%3.3f(second), timeout=%d(milli second) (reciever=%d) Return value = (handle_id=%d)\n", ((endtime-starttime)/1000000.), pollingTimeout, cb_data.pid, ret);

				/**************************************
				 *
				 * Close callback result pipe
				 *
				 **************************************/
				if (fd_FOCUS_R != -1) {
					close(fd_FOCUS_R);
					fd_FOCUS_R = -1;
				}
			}
		}
	}
	debug_fleave();
	return MM_ERROR_NONE;

fail:
	if (filename) {
		g_free (filename);
		filename = NULL;
	}
	if (filename2) {
		g_free (filename2);
		filename2 = NULL;
	}
	if (fd_FOCUS != -1) {
		close(fd_FOCUS);
		fd_FOCUS = -1;
	}
	if (fd_FOCUS_R != -1) {
		close (fd_FOCUS_R);
		fd_FOCUS_R = -1;
	}
	debug_fleave();
	return -1;
}

int _mm_sound_mgr_focus_do_callback(focus_command_e command, focus_node_t *victim_node, _mm_sound_mgr_focus_param_t *assaulter_param, const char *assaulter_stream_type)
{
	char *filename = NULL;
	char *filename2 = NULL;
	struct timeval time;
	int starttime = 0;
	int endtime = 0;
	int fd_FOCUS_R = -1;
	int fd_FOCUS = -1;
	int ret = -1;
	struct pollfd pfd;
	int pret = 0;
	int pollingTimeout = 2500; /* NOTE : This is temporary code, because of Deadlock issues. If you fix that issue, remove this comment */

	int i = 0;
	int flag_for_focus_type = 0;
	int flag_for_taken_index = 0;
	int taken_pid = 0;
	int taken_hid = 0;

	focus_cb_data cb_data;

	debug_error(" __mm_sound_mgr_focus_do_callback_ for pid(%d) handle(%d)\n", victim_node->pid, victim_node->handle_id);

	memset(&cb_data, 0, sizeof(focus_cb_data));
	cb_data.pid= victim_node->pid;
	cb_data.handle= victim_node->handle_id;
	if (command == FOCUS_COMMAND_RELEASE) {
		/* client will lost the acquired focus */
		cb_data.type= assaulter_param->request_type & victim_node->status;
		cb_data.state= FOCUS_STATUS_DEACTIVATED;
	} else {
		/* client will gain the lost focus */
		for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
			if ((victim_node->taken_by_id[i].pid == assaulter_param->pid) && (victim_node->taken_by_id[i].handle_id == assaulter_param->handle_id)) {
				flag_for_focus_type |= i+1; /* playback:1, capture:2 */
			}
		}
		cb_data.type = flag_for_focus_type & assaulter_param->request_type;
		cb_data.state = !FOCUS_STATUS_DEACTIVATED;
	}
	MMSOUND_STRNCPY(cb_data.stream_type, assaulter_stream_type, MAX_STREAM_TYPE_LEN);
	MMSOUND_STRNCPY(cb_data.name, assaulter_param->option, MM_SOUND_NAME_NUM);

	/* Set start time */
	gettimeofday(&time, NULL);
	starttime = time.tv_sec * 1000000 + time.tv_usec;

	/**************************************
	 *
	 * Open callback cmd pipe
	 *
	 **************************************/
	filename = __get_focus_pipe_path(cb_data.pid, cb_data.handle, NULL, false);
	if ((fd_FOCUS = open(filename, O_WRONLY|O_NONBLOCK)) == -1) {
		debug_error("[CallCB] %s open error\n", filename);
		goto fail;
	}

	/******************************************
	 *
	 * Open callback result pipe
	 * before writing callback cmd to pipe
	 *
	 ******************************************/
	filename2 = __get_focus_pipe_path(cb_data.pid, cb_data.handle, "r", false);
	if ((fd_FOCUS_R = open(filename2,O_RDONLY|O_NONBLOCK)) == -1) {
		char str_error[256];
		strerror_r (errno, str_error, sizeof(str_error));
		debug_error("[RETCB] Fail to open fifo (%s)\n", str_error);
		goto fail;
	}
	debug_log(" open return cb %s\n", filename2);


	/*******************************************
	 * Write Callback msg
	 *******************************************/
	if (write(fd_FOCUS, &cb_data, sizeof(cb_data)) == -1) {
		debug_error("[CallCB] %s write error\n", filename);
		goto fail;
	}
	/**************************************
	 *
	 * Close callback cmd pipe
	 *
	 **************************************/
	if (fd_FOCUS != -1) {
		close(fd_FOCUS);
		fd_FOCUS = -1;
	}
	g_free(filename);
	filename = NULL;

	pfd.fd = fd_FOCUS_R;
	pfd.events = POLLIN;

	/*********************************************
	 *
	 * Wait callback result msg
	 *
	 ********************************************/
	debug_error("[RETCB]wait callback(tid=%d, handle=%d, cmd=%d, timeout=%d)\n",cb_data.pid, cb_data.handle, command, pollingTimeout);
	pret = poll(&pfd, 1, pollingTimeout);
	if (pret < 0) {
		debug_error("[RETCB]poll failed (%d)\n", pret);
		goto fail;
	}
	if (pfd.revents & POLLIN) {
		if (read(fd_FOCUS_R, &ret, sizeof(ret)) == -1) {
			debug_error("read error\n");
			goto fail;
		}
	}
	g_free(filename2);
	filename2 = NULL;

	/* Calculate endtime and display*/
	gettimeofday(&time, NULL);
	endtime = time.tv_sec * 1000000 + time.tv_usec;
	debug_error("[RETCB] ASM_CB_END cbtimelab=%3.3f(second), timeout=%d(milli second) (reciever=%d) Return value = (handle_id=%d)\n", ((endtime-starttime)/1000000.), pollingTimeout, cb_data.pid, ret);

	/**************************************
	 *
	 * Close callback result pipe
	 *
	 **************************************/
	if (fd_FOCUS_R != -1) {
		close(fd_FOCUS_R);
		fd_FOCUS_R = -1;
	}
	//debug_log("[RETCB] Return value 0x%x\n", buf);

	/* update victim node */
	taken_pid = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->pid : 0;
	taken_hid = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->handle_id : 0;
	flag_for_taken_index = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->request_type & victim_node->status : assaulter_param->request_type;
	for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
		if (flag_for_taken_index & (i+1)) {
			if (command == FOCUS_COMMAND_ACQUIRE && (victim_node->taken_by_id[i].pid != assaulter_param->pid || victim_node->taken_by_id[i].handle_id != assaulter_param->handle_id)) {
				/* skip */
				continue;
			}
			victim_node->taken_by_id[i].pid = taken_pid;
			victim_node->taken_by_id[i].handle_id = taken_hid;
		}
	}
	if(ret == victim_node->handle_id) {
		/* return from client is success, ret will be its handle_id */
		victim_node->status = (command == FOCUS_COMMAND_RELEASE) ? (victim_node->status &= ~(cb_data.type)) : (victim_node->status |= cb_data.type);
	} else {
		victim_node->status = FOCUS_STATUS_DEACTIVATED;
	}

	if (strncmp(assaulter_stream_type, victim_node->stream_type, MAX_STREAM_TYPE_LEN))
		_mm_sound_mgr_focus_do_watch_callback((focus_type_e)assaulter_param->request_type, command, victim_node, assaulter_param);


	return MM_ERROR_NONE;

fail:
	if (filename) {
		g_free (filename);
		filename = NULL;
	}
	if (filename2) {
		g_free (filename2);
		filename2 = NULL;
	}
	if (fd_FOCUS != -1) {
		close(fd_FOCUS);
		fd_FOCUS = -1;
	}
	if (fd_FOCUS_R != -1) {
		close (fd_FOCUS_R);
		fd_FOCUS_R = -1;
	}

	return -1;
}

static int _mm_sound_mgr_focus_list_dump ()
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;

	debug_log("============================================ focus node list : start ===============================================\n");
	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && !node->is_for_watch) {
			debug_log("*** pid[%5d]/handle_id[%d]/[%15s]: priority[%2d], status[%s], taken_by[P(%5d/%2d)C(%5d/%2d)]\n",
					node->pid, node->handle_id, node->stream_type, node->priority, focus_status_str[node->status],
					node->taken_by_id[0].pid, node->taken_by_id[0].handle_id, node->taken_by_id[1].pid, node->taken_by_id[1].handle_id);
		}
	}
	debug_log("============================================ focus node list : end =================================================\n");

	return ret;
}

static int _mm_sound_mgr_focus_watch_list_dump ()
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;

	debug_log("========================================== focus watch node list : start =============================================\n");
	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && node->is_for_watch) {
			debug_log("*** pid[%5d]/watch on focus status[%s]\n", node->pid, focus_status_str[node->status]);
		}
	}
	debug_log("========================================== focus watch node list : end ===============================================\n");

	return ret;
}

static void _mm_sound_mgr_focus_fill_info_from_msg (focus_node_t *node, const _mm_sound_mgr_focus_param_t *msg)
{
	debug_fenter();
	node->pid = msg->pid;
	node->handle_id = msg->handle_id;
	node->callback = msg->callback;
	node->cbdata = msg->cbdata;
#ifdef SUPPORT_CONTAINER
	memset (&node->container, 0, sizeof (container_info_t));
	strcpy(node->container.name, "NONAME");
	node->container.pid = msg->pid;
#endif

	debug_fleave();
	return;
}

#ifdef SUPPORT_CONTAINER
void _mm_sound_mgr_focus_update_container_data(int pid,int handle, const char* container_name, int container_pid)
{
	__set_container_data(pid, handle, container_name, container_pid);
	//__temp_print_list(NULL);
}
#endif

int mm_sound_mgr_focus_create_node (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;
	int priority = 0;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && !node->is_for_watch && (node->pid == param->pid) && (node->handle_id == param->handle_id)) {
			debug_error("the node of pid[%d]/handle_id[%d] is already created\n", param->pid, param->handle_id);
			ret = MM_ERROR_INVALID_ARGUMENT;
			goto FINISH;
		}
	}

	/* get priority from stream type */
	ret = _mm_sound_mgr_focus_get_priority_from_stream_type(&priority, param->stream_type);
	if (ret) {
		goto FINISH;
	}
	node = g_malloc0(sizeof(focus_node_t));

	/* fill up information to the node */
	_mm_sound_mgr_focus_fill_info_from_msg(node, param);
	node->priority = priority;
	node->status = FOCUS_STATUS_DEACTIVATED;
	MMSOUND_STRNCPY(node->stream_type, param->stream_type, MAX_STREAM_TYPE_LEN);

	g_focus_node_list = g_list_append(g_focus_node_list, node);
	if (g_focus_node_list) {
		debug_log("new focus node is added\n");
	} else {
		debug_error("g_list_append failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
		g_free(node);
	}

	_mm_sound_mgr_focus_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int mm_sound_mgr_focus_destroy_node (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_SOUND_INTERNAL;
	GList *list = NULL;
	focus_node_t *node = NULL;
	int i = 0;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && !node->is_for_watch && (node->pid == param->pid) && (node->handle_id == param->handle_id)) {
			debug_log("found the node of pid[%d]/handle_id[%d]\n", param->pid, param->handle_id);
			g_focus_node_list = g_list_remove(g_focus_node_list, node);
			g_free(node);
			ret = MM_ERROR_NONE;
			break;
		}
	}
	if (list == NULL) {
		debug_error("could not find any node of pid[%d]/handle_id[%d]\n", param->pid, param->handle_id);
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto FINISH;
	}
	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
			if (node && (node->taken_by_id[i].pid == param->pid && node->taken_by_id[i].handle_id == param->handle_id)) {
				node->taken_by_id[i].pid = 0;
				node->taken_by_id[i].handle_id = 0;
			}
		}
	}
	_mm_sound_mgr_focus_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int mm_sound_mgr_focus_request_acquire (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;
	focus_node_t *my_node = NULL;
	bool need_to_trigger = false;
	bool need_to_trigger_watch_cb = true;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && !node->is_for_watch && (node->pid == param->pid) && (node->handle_id == param->handle_id)) {
			my_node = node;
			if ((my_node->status > FOCUS_STATUS_DEACTIVATED) && (my_node->status & param->request_type)) {
				ret = MM_ERROR_SOUND_INVALID_STATE;
				goto FINISH;
			}
		}
	}
	/* check if the priority of any node is higher than its based on io direction */
	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (my_node == node || node->is_for_watch) {
			/* skip */
		} else if (node && (param->request_type == FOCUS_TYPE_BOTH || node->status == FOCUS_STATUS_ACTIVATED_BOTH ||
					(node->status & param->request_type))) {
			if (node->status > FOCUS_STATUS_DEACTIVATED) {
				if ((my_node->priority < node->priority)) {
					ret = MM_ERROR_POLICY_BLOCKED;
					need_to_trigger = false;
					break;
				} else {
					need_to_trigger = true;
				}
			}
		}
	}

	if (need_to_trigger) {
		for (list = g_focus_node_list; list != NULL; list = list->next) {
			node = (focus_node_t *)list->data;
			if (node == my_node || node->is_for_watch) {
				/* skip */
			} else if (node && (param->request_type == FOCUS_TYPE_BOTH || node->status == FOCUS_STATUS_ACTIVATED_BOTH ||
					(node->status & param->request_type))) {
				if (node->status > FOCUS_STATUS_DEACTIVATED) {
					if (my_node->priority >= node->priority) {
						/* do callback for interruption */
						ret = _mm_sound_mgr_focus_do_callback(FOCUS_COMMAND_RELEASE, node, param, my_node->stream_type);
						if (ret) {
							debug_error("Fail to _focus_do_callback for COMMAND RELEASE to node[%x], ret[0x%x]\n", node, ret);
							/* but, keep going */
							ret = MM_ERROR_NONE;
						}
						if (!strncmp(my_node->stream_type, node->stream_type, MAX_STREAM_TYPE_LEN)) {
							need_to_trigger_watch_cb = false;
						}
					}
				}
			}
		}
	}

	if (ret != MM_ERROR_POLICY_BLOCKED) {
		/* update status */
		my_node->status |= param->request_type;
		/* do watch callback due to the status of mine */
		if (need_to_trigger_watch_cb)
			_mm_sound_mgr_focus_do_watch_callback((focus_type_e)param->request_type, FOCUS_COMMAND_ACQUIRE, my_node, param);
	}
	_mm_sound_mgr_focus_list_dump();
	_mm_sound_mgr_focus_watch_list_dump ();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int mm_sound_mgr_focus_request_release (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;
	focus_node_t *my_node = NULL;
	bool need_to_trigger_watch_cb = true;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && !node->is_for_watch && (node->pid == param->pid) && (node->handle_id == param->handle_id)) {
			my_node = node;
			if (my_node->status == FOCUS_STATUS_DEACTIVATED) {
				ret = MM_ERROR_SOUND_INVALID_STATE;
				goto FINISH;
			} else if ((my_node->status != FOCUS_STATUS_ACTIVATED_BOTH) && (my_node->status != param->request_type)) {
				ret = MM_ERROR_SOUND_INVALID_STATE;
				goto FINISH;
			}
		}
	}

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node == my_node || node->is_for_watch) {
			/* skip */
		} else {
			int i = 0;
			for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
				if (param->request_type & (i+1)) {
					if (node && (node->taken_by_id[i].pid == param->pid && node->taken_by_id[i].handle_id == param->handle_id)) {
						/* do callback for resumption */
						ret = _mm_sound_mgr_focus_do_callback(FOCUS_COMMAND_ACQUIRE, node, param, my_node->stream_type);
						if (ret) {
							debug_error("Fail to _focus_do_callback for COMMAND ACQUIRE to node[%x], ret[0x%x]\n", node, ret);
						}
						if (!strncmp(my_node->stream_type, node->stream_type, MAX_STREAM_TYPE_LEN)) {
							need_to_trigger_watch_cb = false;
						}
					}
				}
			}
		}
	}
	/* update status */
	my_node->status &= ~(param->request_type);
	/* do watch callback due to the status of mine */
	if (need_to_trigger_watch_cb)
		_mm_sound_mgr_focus_do_watch_callback((focus_type_e)param->request_type, FOCUS_COMMAND_RELEASE, my_node, param);

	_mm_sound_mgr_focus_list_dump();
	_mm_sound_mgr_focus_watch_list_dump ();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int mm_sound_mgr_focus_set_watch_cb (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && (node->pid == param->pid) && node->is_for_watch) {
			debug_error("the node of pid[%d] for watch focus is already created\n", param->pid);
			ret = MM_ERROR_INVALID_ARGUMENT;
			goto FINISH;
		}
	}

	node = g_malloc0(sizeof(focus_node_t));

	/* fill up information to the node */
	_mm_sound_mgr_focus_fill_info_from_msg(node, param);
	node->is_for_watch = true;
	node->status = param->request_type;

	g_focus_node_list = g_list_append(g_focus_node_list, node);
	if (g_focus_node_list) {
		debug_log("new focus node is added\n");
	} else {
		debug_error("g_list_append failed\n");
		ret = MM_ERROR_SOUND_INTERNAL;
		g_free(node);
	}

	_mm_sound_mgr_focus_watch_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int mm_sound_mgr_focus_unset_watch_cb (const _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_SOUND_INTERNAL;
	GList *list = NULL;
	focus_node_t *node = NULL;

	debug_fenter();

	MMSOUND_ENTER_CRITICAL_SECTION_WITH_RETURN(&g_focus_node_list_mutex, MM_ERROR_SOUND_INTERNAL);

	/* Update list for dead process */
	g_list_foreach (g_focus_node_list, (GFunc)_clear_focus_node_list_func, NULL);

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node && (node->pid == param->pid) && (node->is_for_watch)) {
			debug_log("found the node of pid[%d] for watch focus\n", param->pid);
			g_focus_node_list = g_list_remove(g_focus_node_list, node);
			g_free(node);
			ret = MM_ERROR_NONE;
			break;
		}
	}
	if (list == NULL) {
		debug_error("could not find any node of pid[%d] for watch focus\n", param->pid);
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto FINISH;
	}

	_mm_sound_mgr_focus_watch_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int MMSoundMgrFocusInit(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	ret = __mm_sound_mgr_ipc_dbus_get_stream_list(&g_stream_list);
	if (ret)
		debug_error("failed to __mm_sound_mgr_ipc_dbus_get_stream_list()\n");

	debug_fleave();
	return ret;
}

int MMSoundMgrFocusFini(void)
{
	int i = 0;
	debug_fenter();

	for (i = 0; i < AVAIL_STREAMS_MAX; i++) {
		if (g_stream_list.stream_types[i]) {
			free (g_stream_list.stream_types[i]);
		}
	}

	debug_fleave();
	return MM_ERROR_NONE;
}

