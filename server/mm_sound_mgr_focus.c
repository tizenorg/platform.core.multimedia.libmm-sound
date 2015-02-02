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

#include "include/mm_sound_mgr_ipc.h"
#include "../include/mm_sound_utils.h"
#include <sys/time.h>

static GList *g_focus_node_list = NULL;
static pthread_mutex_t g_focus_node_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char* focus_status_str[] =
{
	"DEACTIVATED",
	"P ACTIVATED",
	"C ACTIVATED",
	"B ACTIVATED",
};

#define CLEAR_DEAD_NODE_LIST(x)  do { \
	debug_warning ("list = %p, node = %p, pid=[%d]", x, node, (node)? node->pid : -1); \
	if (x && node && (__is_pid_exist(node->pid) == FALSE)) { \
		debug_warning("PID:%d does not exist now! remove from device cb list\n", node->pid); \
		g_free (node); \
		x = g_list_remove (x, node); \
	} \
}while(0)

static void _clear_focus_node_list_func (focus_node_t *node, gpointer user_data)
{
	CLEAR_DEAD_NODE_LIST(g_focus_node_list);
}

int __mm_sound_mgr_focus_get_priority_from_stream_type(int *priority, const char *stream_type)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (priority == NULL || stream_type == NULL) {
		ret = MM_ERROR_INVALID_ARGUMENT;
		debug_error("invalid argument, priority[0x%x], stream_type[%s], ret[0x%x]\n", priority, stream_type, ret);
	} else {
		if (!strncmp(STREAM_TYPE_CALL, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 10;
		} else if (!strncmp(STREAM_TYPE_VOIP, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 9;
		} else if (!strncmp(STREAM_TYPE_RINGTONE, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 8;
		} else if (!strncmp(STREAM_TYPE_EMERGENCY, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 7;
		} else if (!strncmp(STREAM_TYPE_ALARM, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 6;
		} else if (!strncmp(STREAM_TYPE_NOTIFICATION, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 5;
		} else if (!strncmp(STREAM_TYPE_MEDIA_PLAYBACK, stream_type, MAX_STREAM_TYPE_LEN) ||
			!strncmp(STREAM_TYPE_MEDIA_RECORDING, stream_type, MAX_STREAM_TYPE_LEN) ||
			!strncmp(STREAM_TYPE_RADIO, stream_type, MAX_STREAM_TYPE_LEN) ||
			!strncmp(STREAM_TYPE_TTS, stream_type, MAX_STREAM_TYPE_LEN) ||
			!strncmp(STREAM_TYPE_VOICE_RECOGNITION, stream_type, MAX_STREAM_TYPE_LEN)) {
			*priority = 1;
		} else {
			ret = MM_ERROR_INVALID_ARGUMENT;
			debug_error("not supported stream_type[%s], ret[0x%x]\n", stream_type, ret);
		}
		if (!(*priority)) {
			debug_log("[%s] has priority of [%d]\n", stream_type, *priority);
		}
	}

	debug_fleave();
	return ret;
}

int ___mm_sound_mgr_focus_do_watch_callback(focus_type_e focus_type, focus_command_e command, focus_node_t *my_node, _mm_sound_mgr_focus_param_t *param)
{
	int ret = MM_ERROR_NONE;
	GList *list = NULL;
	focus_node_t *node = NULL;
	mm_ipc_msg_t msg;
	mm_ipc_msg_t rcv_msg;
	struct timeval time;
	int starttime = 0;
	int endtime = 0;
	int i = 0;

	debug_fenter();

	for (list = g_focus_node_list; list != NULL; list = list->next) {
		node = (focus_node_t *)list->data;
		if (node == my_node) {
			/* skip my own */
		} else {
			if (node->is_for_watch && (node->status & focus_type)) {
				memset(&msg, 0, sizeof(mm_ipc_msg_t));
				memset(&rcv_msg, 0, sizeof(mm_ipc_msg_t));
				SOUND_MSG_SET(msg.sound_msg, MM_SOUND_MSG_INF_FOCUS_WATCH_CB, 0, MM_ERROR_NONE, node->pid);
				msg.sound_msg.callback = node->callback;
				msg.sound_msg.cbdata = node->cbdata;
				msg.sound_msg.focus_type = focus_type & node->status;
				msg.sound_msg.changed_state = (command == FOCUS_COMMAND_ACQUIRE) ? !FOCUS_STATUS_DEACTIVATED : FOCUS_STATUS_DEACTIVATED;
				MMSOUND_STRNCPY(msg.sound_msg.stream_type, my_node->stream_type, MAX_STREAM_TYPE_LEN);
				MMSOUND_STRNCPY(msg.sound_msg.name, param->option, MM_SOUND_NAME_NUM);
				msg.wait_for_reply = true;

				/* Set start time */
				gettimeofday(&time, NULL);
				starttime = time.tv_sec * 1000000 + time.tv_usec;
				debug_msg("Before sending msg for [pid(%d),cb(0x%x)], StartTime:%.3f\n", node->pid, node->callback, starttime/1000000.);
				ret = _MMIpcCBSndMsg(&msg);
				if (ret != MM_ERROR_NONE) {
					debug_error("Fail to send callback message for pid[%d], ret[0x%x]\n", node->pid, ret);
					goto FINISH;
				}
				/* wait for return msg */
				/* if rev msg(pid) is not valid, enqueue again */
				do {
					ret = _MMIpcCBRecvMsg(&rcv_msg);
					if (ret != MM_ERROR_NONE) {
						debug_warning("Failed to receive callback message, ret[0x%x]. Need to check the pid[%d] why it got blocked!!!\n", ret, node->pid);
						break;
					} else {
						if (rcv_msg.sound_msg.msgtype == MM_SOUND_MSG_INF_FOCUS_WATCH_CB &&	rcv_msg.sound_msg.msgid == node->pid ) {
							/* Set end time */
							gettimeofday(&time, NULL);
							endtime = time.tv_sec * 1000000 + time.tv_usec;
							debug_msg("Success to receiving callback message for pid[%d], msgtype[%d], EndTime:%.3f\n", rcv_msg.sound_msg.msgid, rcv_msg.sound_msg.msgtype, endtime/1000000.);
							break;
						} else {
							/* enqueue again */
							debug_error("It is not a reply message for pid[%d], msgtype[%d], enqueue this message again\n", rcv_msg.sound_msg.msgid, rcv_msg.sound_msg.msgtype);
							ret = _MMIpcCBMsgEnQueueAgain(&rcv_msg);
							if (ret != MM_ERROR_NONE) {
								debug_error("Fail to enqueue the reply message for pid[%d], ret[0x%x]\n", rcv_msg.sound_msg.msgid, ret);
								goto FINISH;
							}
						}
					}
				} while(1);
			}
		}
	}

FINISH:
	debug_fleave();
	return ret;
}

int __mm_sound_mgr_focus_do_callback(focus_command_e command, focus_node_t *victim_node, _mm_sound_mgr_focus_param_t *assaulter_param, const char *assaulter_stream_type)
{
	int ret = MM_ERROR_NONE;
	mm_ipc_msg_t msg;
	mm_ipc_msg_t rcv_msg;
	struct timeval time;
	int starttime = 0;
	int endtime = 0;
	int i = 0;
	int flag_for_focus_type = 0;
	int flag_for_taken_index = 0;
	int taken_pid = 0;
	int taken_hid = 0;

	debug_fenter();

	memset(&msg, 0, sizeof(mm_ipc_msg_t));
	memset(&rcv_msg, 0, sizeof(mm_ipc_msg_t));
	SOUND_MSG_SET(msg.sound_msg, MM_SOUND_MSG_INF_FOCUS_CHANGED_CB, 0, MM_ERROR_NONE, victim_node->pid);
	msg.sound_msg.callback = victim_node->callback;
	msg.sound_msg.cbdata = victim_node->cbdata;
	msg.sound_msg.handle_id = victim_node->handle_id;
	if (command == FOCUS_COMMAND_RELEASE) {
		/* client will lost the acquired focus */
		msg.sound_msg.focus_type = assaulter_param->request_type & victim_node->status;
		msg.sound_msg.changed_state = FOCUS_STATUS_DEACTIVATED;
	} else {
		/* client will gain the lost focus */
		for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
			if ((victim_node->taken_by_id[i].pid == assaulter_param->pid) && (victim_node->taken_by_id[i].handle_id == assaulter_param->handle_id)) {
				flag_for_focus_type |= i+1; /* playback:1, capture:2 */
			}
		}
		msg.sound_msg.focus_type = flag_for_focus_type;
		msg.sound_msg.changed_state = !FOCUS_STATUS_DEACTIVATED;
	}
	MMSOUND_STRNCPY(msg.sound_msg.stream_type, assaulter_stream_type, MAX_STREAM_TYPE_LEN);
	MMSOUND_STRNCPY(msg.sound_msg.name, assaulter_param->option, MM_SOUND_NAME_NUM);
	msg.wait_for_reply = true;

	/* Set start time */
	gettimeofday(&time, NULL);
	starttime = time.tv_sec * 1000000 + time.tv_usec;
	debug_msg("Before sending msg for [pid(%d),hid(%d),cb(0x%x)], StartTime:%.3f\n", victim_node->pid, victim_node->handle_id, victim_node->callback, starttime/1000000.);
	ret = _MMIpcCBSndMsg(&msg);
	if (ret != MM_ERROR_NONE) {
		debug_error("Fail to send callback message for pid[%d], handle_id[%d], ret[0x%x]\n", victim_node->pid, victim_node->handle_id, ret);
		goto FINISH;
	}
	/* wait for return msg */
	/* if rev msg(pid/hid) is not valid, enqueue again */
	do {
		ret = _MMIpcCBRecvMsg(&rcv_msg);
		if (ret != MM_ERROR_NONE) {
			debug_warning("Failed to receive callback message, ret[0x%x]. Need to check the pid[%d]/handle_id[%d] why it got blocked!!!\n", ret, victim_node->pid, victim_node->handle_id);
			break;
		} else {
			if (rcv_msg.sound_msg.msgtype == MM_SOUND_MSG_INF_FOCUS_CHANGED_CB &&
				rcv_msg.sound_msg.msgid == victim_node->pid && rcv_msg.sound_msg.handle_id == victim_node->handle_id) {
				/* Set end time */
				gettimeofday(&time, NULL);
				endtime = time.tv_sec * 1000000 + time.tv_usec;
				debug_msg("Success to receiving callback message for pid[%d], handle_id[%d], msgtype[%d], EndTime:%.3f\n",
						rcv_msg.sound_msg.msgid, rcv_msg.sound_msg.handle_id, rcv_msg.sound_msg.msgtype, endtime/1000000.);
				break;
			} else {
				/* enqueue again */
				debug_error("It is not a reply message for pid[%d], handle_id[%d], msgtype[%d], enqueue this message again\n",
						rcv_msg.sound_msg.msgid, rcv_msg.sound_msg.handle_id, rcv_msg.sound_msg.msgtype);
				ret = _MMIpcCBMsgEnQueueAgain(&rcv_msg);
				if (ret != MM_ERROR_NONE) {
					debug_error("Fail to enqueue the reply message for pid[%d], handle_id[%d], ret[0x%x]\n", rcv_msg.sound_msg.msgid, rcv_msg.sound_msg.handle_id, ret);
					goto FINISH;
				}
			}
		}
	} while(1);

	/* update victim node */
	taken_pid = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->pid : 0;
	taken_hid = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->handle_id : 0;
	flag_for_taken_index = (command == FOCUS_COMMAND_RELEASE) ? assaulter_param->request_type & victim_node->status : assaulter_param->request_type;
	for (i = 0; i < NUM_OF_STREAM_IO_TYPE; i++) {
		if (flag_for_taken_index & (i+1)) {
			victim_node->taken_by_id[i].pid = taken_pid;
			victim_node->taken_by_id[i].handle_id = taken_hid;
		}
	}
	if(ret == MM_ERROR_NONE) {
		victim_node->status = (command == FOCUS_COMMAND_RELEASE) ? (victim_node->status &= ~(assaulter_param->request_type)) : (victim_node->status |= flag_for_focus_type);
	} else {
		victim_node->status = FOCUS_STATUS_DEACTIVATED;
	}

	if (strncmp(assaulter_stream_type, victim_node->stream_type, MAX_STREAM_TYPE_LEN))
		___mm_sound_mgr_focus_do_watch_callback((focus_type_e)assaulter_param->request_type, command, victim_node, assaulter_param);

FINISH:
	debug_fleave();
	return ret;
}

int __mm_sound_mgr_focus_list_dump ()
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

int __mm_sound_mgr_focus_watch_list_dump ()
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

gboolean __is_pid_exist(int pid)
{
	if (pid > 999999 || pid < 2)
		return FALSE;
	gchar *tmp = g_malloc0(25);
	g_sprintf(tmp, "/proc/%d", pid);
	if (access(tmp, R_OK)==0) {
		g_free(tmp);
		return TRUE;
	}
	g_free(tmp);
	return FALSE;
}

void __mm_sound_mgr_focus_fill_info_from_msg (focus_node_t *node, const _mm_sound_mgr_focus_param_t *msg)
{
	node->pid = msg->pid;
	node->handle_id = msg->handle_id;
	node->callback = msg->callback;
	node->cbdata = msg->cbdata;
	return;
}

int _mm_sound_mgr_focus_create_node (const _mm_sound_mgr_focus_param_t *param)
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
	ret = __mm_sound_mgr_focus_get_priority_from_stream_type(&priority, param->stream_type);
	if (ret) {
		goto FINISH;
	}
	node = g_malloc0(sizeof(focus_node_t));

	/* fill up information to the node */
	__mm_sound_mgr_focus_fill_info_from_msg(node, param);
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

	__mm_sound_mgr_focus_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int _mm_sound_mgr_focus_destroy_node (const _mm_sound_mgr_focus_param_t *param)
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
	__mm_sound_mgr_focus_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int _mm_sound_mgr_focus_request_acquire (const _mm_sound_mgr_focus_param_t *param)
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
		} else if (node && (param->request_type == FOCUS_TYPE_BOTH || node->status == FOCUS_STATUS_ACTIVATED_BOTH || (node->status & param->request_type))) {
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
			} else if (node && (param->request_type == FOCUS_TYPE_BOTH || node->status == FOCUS_STATUS_ACTIVATED_BOTH || (node->status & param->request_type))) {
				if (node->status > FOCUS_STATUS_DEACTIVATED) {
					if (my_node->priority >= node->priority) {
						/* do callback for interruption */
						ret = __mm_sound_mgr_focus_do_callback(FOCUS_COMMAND_RELEASE, node, param, my_node->stream_type);
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
			___mm_sound_mgr_focus_do_watch_callback((focus_type_e)param->request_type, FOCUS_COMMAND_ACQUIRE, my_node, param);
	}
	__mm_sound_mgr_focus_list_dump();
	__mm_sound_mgr_focus_watch_list_dump ();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int _mm_sound_mgr_focus_request_release (const _mm_sound_mgr_focus_param_t *param)
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
						ret = __mm_sound_mgr_focus_do_callback(FOCUS_COMMAND_ACQUIRE, node, param, my_node->stream_type);
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
		___mm_sound_mgr_focus_do_watch_callback((focus_type_e)param->request_type, FOCUS_COMMAND_RELEASE, my_node, param);

	__mm_sound_mgr_focus_list_dump();
	__mm_sound_mgr_focus_watch_list_dump ();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int _mm_sound_mgr_focus_set_watch_cb (const _mm_sound_mgr_focus_param_t *param)
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
	__mm_sound_mgr_focus_fill_info_from_msg(node, param);
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

	__mm_sound_mgr_focus_watch_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int _mm_sound_mgr_focus_unset_watch_cb (const _mm_sound_mgr_focus_param_t *param)
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

	__mm_sound_mgr_focus_watch_list_dump();
FINISH:
	MMSOUND_LEAVE_CRITICAL_SECTION(&g_focus_node_list_mutex);

	debug_fleave();
	return ret;
}

int MMSoundMgrFocusInit(void)
{
	debug_fenter();


	debug_fleave();
	return MM_ERROR_NONE;
}

int MMSoundMgrFocusFini(void)
{
	debug_fenter();

	debug_fleave();
	return MM_ERROR_NONE;
}

