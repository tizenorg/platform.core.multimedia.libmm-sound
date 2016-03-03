/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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
 * @file		mm_sound_dbus.h
 * @brief		Internal dbus utility library for audio module.
 * @date
 * @version		Release
 *
 * Internal dbus utility library for audio module.
 * Audio modules can use dbus simply without using dbus library directly.
 */

#ifndef __MM_SOUND_DBUS_H__
#define __MM_SOUND_DBUS_H__

#include <gio/gio.h>
#include "include/mm_sound_intf.h"

typedef void (*mm_sound_dbus_callback)(audio_event_t event, GVariant *param, void *userdata);
typedef void (*mm_sound_dbus_userdata_free) (void *data);

int mm_sound_dbus_method_call_to(audio_provider_t provider, audio_method_t method_type, GVariant *args, GVariant **result);
int mm_sound_dbus_signal_subscribe_to(audio_provider_t provider, audio_event_t event, mm_sound_dbus_callback callback, void *userdata, mm_sound_dbus_userdata_free freefunc, unsigned *subs_id);
int mm_sound_dbus_signal_unsubscribe(unsigned subs_id);
int mm_sound_dbus_emit_signal(audio_provider_t provider, audio_event_t event, GVariant *param);

int mm_sound_dbus_get_event_name(audio_event_t event, const char **event_name);
int mm_sound_dbus_get_method_name(audio_method_t method, const char **method_name);

typedef void (*dbus_method_handler)(GDBusMethodInvocation *invocation);
typedef int (*dbus_signal_sender)(GDBusConnection *conn, GVariant *parameter);

typedef struct mm_sound_dbus_method_info{
	const char* name;
	/*
	const char* argument;
	const char* reply;
	*/
} mm_sound_dbus_method_info_t;

typedef struct mm_sound_dbus_signal_info{
	const char* name;
	const char* argument;
} mm_sound_dbus_signal_info_t;

typedef struct mm_sound_dbus_method_intf {
	struct mm_sound_dbus_method_info info;
	dbus_method_handler handler;
} mm_sound_dbus_method_intf_t;

typedef struct mm_sound_dbus_signal_intf {
	struct mm_sound_dbus_signal_info info;
	dbus_signal_sender sender;
} mm_sound_dbus_signal_intf_t;

#endif /* __MM_SOUND_DBUS_H__  */
