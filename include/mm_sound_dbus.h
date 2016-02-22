
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
