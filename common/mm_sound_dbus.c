
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "../include/mm_sound_common.h"
#include "../include/mm_sound_dbus.h"
#include "../include/mm_sound_intf.h"

#define INTERFACE_DBUS			"org.freedesktop.DBus.Properties"
#define SIGNAL_PROP_CHANGED             "PropertiesChanged"
#define METHOD_GET			"Get"
#define METHOD_SET			"Set"

struct callback_data {
	mm_sound_dbus_callback user_cb;
	void *user_data;
	mm_sound_dbus_userdata_free free_func;
};

struct dbus_path {
	char *bus_name;
	char *object;
	char *interface;
};

const struct dbus_path g_paths[AUDIO_PROVIDER_MAX] = {
	[AUDIO_PROVIDER_SOUND_SERVER] = {
		.bus_name = "org.tizen.SoundServer",
		.object = "/org/tizen/SoundServer1",
		.interface ="org.tizen.SoundServer1"
	},
	[AUDIO_PROVIDER_FOCUS_SERVER] = {
		.bus_name = "org.tizen.FocusServer",
		.object = "/org/tizen/FocusServer1",
		.interface = "org.tizen.FocusServer1"
	},
	[AUDIO_PROVIDER_DEVICE_MANAGER] = {
		.bus_name = "org.pulseaudio.Server",
		.object = "/org/pulseaudio/DeviceManager",
		.interface = "org.pulseaudio.DeviceManager"
	},
	[AUDIO_PROVIDER_STREAM_MANAGER] = {
		.bus_name = "org.pulseaudio.Server",
		.object = "/org/pulseaudio/StreamManager",
		.interface = "org.pulseaudio.StreamManager"
	},
	[AUDIO_PROVIDER_AUDIO_CLIENT] = {
		.bus_name = "org.tizen.AudioClient",
		.object = "/org/tizen/AudioClient1",
		.interface = "org.tizen.AudioClient1"
	}
};

const mm_sound_dbus_method_info_t g_methods[AUDIO_METHOD_MAX] = {
	[AUDIO_METHOD_TEST] = {
		.name = "MethodTest1",
	},
	[AUDIO_METHOD_PLAY_FILE_START] = {
		.name = "PlayFileStart",
	},
	[AUDIO_METHOD_PLAY_FILE_START_WITH_STREAM_INFO] = {
		.name = "PlayFileStartWithStreamInfo",
	},
	[AUDIO_METHOD_PLAY_FILE_STOP] = {
		.name = "PlayFileStop",
	},
	[AUDIO_METHOD_PLAY_DTMF] = {
		.name = "PlayDTMF",
	},
	[AUDIO_METHOD_PLAY_DTMF_WITH_STREAM_INFO] = {
		.name = "PlayDTMFWithStreamInfo",
	},
	[AUDIO_METHOD_CLEAR_FOCUS] = {
		.name = "ClearFocus",
	},
	[AUDIO_METHOD_SET_VOLUME_LEVEL] = {
		.name = "SetVolumeLevel",
	},
	[AUDIO_METHOD_GET_CONNECTED_DEVICE_LIST] = {
		.name = "GetConnectedDeviceList",
	},
	[AUDIO_METHOD_GET_UNIQUE_ID] = {
		.name = "GetUniqueId",
	},
	[AUDIO_METHOD_REGISTER_FOCUS] = {
		.name = "RegisterFocus",
	},
	[AUDIO_METHOD_UNREGISTER_FOCUS] = {
		.name = "UnregisterFocus",
	},
	[AUDIO_METHOD_SET_FOCUS_REACQUISITION] = {
		.name = "SetFocusReacquisition",
	},
	[AUDIO_METHOD_GET_ACQUIRED_FOCUS_STREAM_TYPE] = {
		.name = "GetAcquiredFocusStreamType",
	},
	[AUDIO_METHOD_ACQUIRE_FOCUS] = {
		.name = "AcquireFocus",
	},
	[AUDIO_METHOD_RELEASE_FOCUS] = {
		.name = "ReleaseFocus",
	},
	[AUDIO_METHOD_WATCH_FOCUS] = {
		.name = "WatchFocus",
	},
	[AUDIO_METHOD_UNWATCH_FOCUS] = {
		.name = "UnwatchFocus",
	},
};

const mm_sound_dbus_signal_info_t g_events[AUDIO_EVENT_MAX] = {
	[AUDIO_EVENT_TEST] = {
		.name = "SignalTest1",
	},
	[AUDIO_EVENT_PLAY_FILE_END] = {
		.name = "PlayFileEnd",
	},
	[AUDIO_EVENT_VOLUME_CHANGED] = {
		.name = "VolumeChanged",
	},
	[AUDIO_EVENT_DEVICE_CONNECTED] = {
		.name = "DeviceConnected",
	},
	[AUDIO_EVENT_DEVICE_INFO_CHANGED] = {
		.name = "DeviceInfoChanged",
	},
	[AUDIO_EVENT_FOCUS_CHANGED] = {
		.name = "FocusChanged",
	},
	[AUDIO_EVENT_FOCUS_WATCH] = {
		.name = "FocusWatch",
	},
	[AUDIO_EVENT_EMERGENT_EXIT] = {
		.name = "EmergentExit",
	}
};

/* Only For error types which is currently being used in server-side */
static const GDBusErrorEntry mm_sound_error_entries[] =
{
	{MM_ERROR_OUT_OF_MEMORY, "org.tizen.multimedia.OutOfMemory"},
	{MM_ERROR_OUT_OF_STORAGE, "org.tizen.multimedia.OutOfStorage"},
	{MM_ERROR_INVALID_ARGUMENT, "org.tizen.multimedia.InvalidArgument"},
	{MM_ERROR_POLICY_INTERNAL, "org.tizen.multimedia.PolicyInternal"},
	{MM_ERROR_NOT_SUPPORT_API, "org.tizen.multimedia.NotSupportAPI"},
	{MM_ERROR_POLICY_BLOCKED, "org.tizen.multimedia.PolicyBlocked"},
	{MM_ERROR_END_OF_FILE, "org.tizen.multimedia.EndOfFile"},
	{MM_ERROR_COMMON_OUT_OF_RANGE, "org.tizen.multimedia.common.OutOfRange"},
	{MM_ERROR_COMMON_UNKNOWN, "org.tizen.multimedia.common.Unknown"},
	{MM_ERROR_COMMON_NO_FREE_SPACE, "org.tizen.multimedia.common.NoFreeSpace"},
	{MM_ERROR_SOUND_INTERNAL, "org.tizen.multimedia.audio.Internal"},
	{MM_ERROR_SOUND_INVALID_STATE, "org.tizen.multimedia.audio.InvalidState"},
	{MM_ERROR_SOUND_NO_FREE_SPACE, "org.tizen.multimedia.audio.NoFreeSpace"},
	{MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE, "org.tizen.multimedia.audio.UnsupportedMediaType"},
	{MM_ERROR_SOUND_INVALID_POINTER, "org.tizen.multimedia.audio.InvalidPointer"},
	{MM_ERROR_SOUND_INVALID_FILE, "org.tizen.multimedia.audio.InvalidFile"},
	{MM_ERROR_SOUND_FILE_NOT_FOUND, "org.tizen.multimedia.audio.FileNotFound"},
	{MM_ERROR_SOUND_NO_DATA, "org.tizen.multimedia.audio.NoData"},
	{MM_ERROR_SOUND_INVALID_PATH, "org.tizen.multimedia.audio.InvalidPath"},
};


/******************************************************************************************
		Wrapper Functions of GDbus
******************************************************************************************/

static int _parse_error_msg(char *full_err_msg, char **err_name, char **err_msg)
{
	char *save_p, *domain, *_err_name, *_err_msg;

	if (!(domain = strtok_r(full_err_msg, ":", &save_p))) {
		debug_error("get domain failed");
		return -1;
	}
	if (!(_err_name = strtok_r(NULL, ":", &save_p))) {
		debug_error("get err name failed");
		return -1;
	}
	if (!(_err_msg = strtok_r(NULL, ":", &save_p))) {
		debug_error("get err msg failed");
		return -1;
	}

	*err_name = _err_name;
	*err_msg = _err_msg;

	return 0;
}

static int _convert_error_name(const char *err_name)
{
	int i = 0;

	for (i = 0; i < G_N_ELEMENTS(mm_sound_error_entries); i++) {
		if (!strcmp(mm_sound_error_entries[i].dbus_error_name, err_name)) {
			return mm_sound_error_entries[i].error_code;
		}
	}

	return MM_ERROR_COMMON_UNKNOWN;
}

static int _dbus_method_call(GDBusConnection* conn, const char* bus_name, const char* object, const char* intf,
							const char* method, GVariant* args, GVariant** result)
{
	int ret = MM_ERROR_NONE;
	GError *err = NULL;
	GVariant* dbus_reply = NULL;

	if (!conn || !object || !intf || !method) {
		debug_error("Invalid Argument");
		if (!conn)
			debug_error("conn null");
		if (!object)
			debug_error("object null");
		if (!intf)
			debug_error("intf null");
		if (!method)
			debug_error("method null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus call with obj'%s' intf'%s' method'%s'", object, intf, method);

	dbus_reply = g_dbus_connection_call_sync(conn, bus_name, object , intf,
			     method, args ,
			     NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err );

	if (!dbus_reply) {
		char *err_name = NULL, *err_msg = NULL;
		debug_log("Method Call '%s.%s' Failed, %s", intf, method, err->message);

		if (_parse_error_msg(err->message,  &err_name, &err_msg) < 0) {
			debug_error("failed to parse error message");
			g_error_free(err);
			return MM_ERROR_SOUND_INTERNAL;
		}
		ret = _convert_error_name(err_name);
		g_error_free(err);
	}

	debug_log("Method Call '%s.%s' Success", intf, method);
	*result = dbus_reply;

	return ret;
}

#if 0
static int _dbus_set_property(GDBusConnection* conn, const char* bus_name, const char* object, const char* intf,
							const char* prop, GVariant* args, GVariant** result)
{
	int ret = MM_ERROR_NONE;

	if (!conn || !object || !intf || !prop) {
		debug_error("Invalid Argument");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus set property with obj'%s' intf'%s' prop'%s'", object, intf, prop);

	if ((ret = _dbus_method_call(conn, bus_name, object, INTERFACE_DBUS, METHOD_SET,
								g_variant_new("(ssv)", intf, prop, args), result)) != MM_ERROR_NONE) {
		debug_error("Dbus call for set property failed");
	}

	return ret;
}

static int _dbus_get_property(GDBusConnection *conn, const char* bus_name, const char* object_name,
							const char* intf_name, const char* prop, GVariant** result)
{
	int ret = MM_ERROR_NONE;

	if (!conn || !object_name || !intf_name || !prop) {
		debug_error("Invalid Argument");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus get property with obj'%s' intf'%s' prop'%s'", object_name, intf_name, prop);

	if ((ret = _dbus_method_call(conn,
				       bus_name, object_name, INTERFACE_DBUS, METHOD_GET,
				       g_variant_new("(ss)", intf_name, prop), result)) != MM_ERROR_NONE) {
		debug_error("Dbus call for get property failed");
	}

	return ret;
}
#endif

static int _dbus_subscribe_signal(GDBusConnection *conn, const char* object_name, const char* intf_name,
					const char* signal_name, GDBusSignalCallback signal_cb, guint *subscribe_id, void* userdata, GDestroyNotify freefunc)
{
	guint subs_id = 0;

	if (!conn || !object_name || !intf_name || !signal_name || !signal_cb) {
		debug_error("Invalid Argument");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus subscirbe signal with Obj'%s' Intf'%s' sig'%s'", object_name, intf_name, signal_name);

	subs_id = g_dbus_connection_signal_subscribe(conn, NULL, intf_name, signal_name, object_name, \
			 NULL , G_DBUS_SIGNAL_FLAGS_NONE , signal_cb, userdata, NULL );

	if (!subs_id) {
		debug_error ("g_dbus_connection_signal_subscribe() failed ");
		return MM_ERROR_SOUND_INTERNAL;
	}
	if (subscribe_id)
		*subscribe_id = subs_id;

	return MM_ERROR_NONE;
}

static void _dbus_unsubscribe_signal(GDBusConnection *conn, guint subs_id)
{
	if (!conn || !subs_id) {
		debug_error("Invalid Argument");
		return;
	}

	g_dbus_connection_signal_unsubscribe(conn, subs_id);
}

static GDBusConnection* _dbus_get_connection(GBusType bustype)
{
	static GDBusConnection *conn_system = NULL;
	static GDBusConnection *conn_session = NULL;

	if (bustype == G_BUS_TYPE_SYSTEM) {
		if (conn_system) {
			debug_log("Already connected to system bus");
		} else {
			debug_log("Get new connection on system bus");
			conn_system = g_bus_get_sync(bustype, NULL, NULL);
		}
		return conn_system;
	} else if (bustype == G_BUS_TYPE_SESSION) {
		if (conn_session) {
			debug_log("Already connected to session bus");
		} else {
			debug_log("Get new connection on session bus");
			conn_session = g_bus_get_sync(bustype, NULL, NULL);
		}
		return conn_session;
	} else {
		debug_error("Invalid bus type");
		return NULL;
	}

}

#if 0
static void _dbus_disconnect(GDBusConnection* conn)
{
	debug_fenter ();
	g_object_unref(conn);
	debug_fleave ();
}
#endif

/******************************************************************************************
		Simple Functions For Communicate with Sound-Server
******************************************************************************************/

EXPORT_API
int mm_sound_dbus_method_call_to(audio_provider_t provider, audio_method_t method_type, GVariant *args, GVariant **result)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;

	if (method_type < 0 || method_type >= AUDIO_METHOD_MAX) {
		debug_error("Invalid method type : %d", method_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (provider < 0 || provider >= AUDIO_PROVIDER_MAX) {
		debug_error("Invalid provider : %d", provider);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!(conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if((ret = _dbus_method_call(conn, g_paths[provider].bus_name,
				      g_paths[provider].object,
				      g_paths[provider].interface,
				      g_methods[method_type].name,
				      args, result)) != MM_ERROR_NONE) {
		debug_error("Dbus Call on Client Error");
	}

	return ret;
}

/* This callback only transform signal-callback to dbus non-related form (mm_sound_dbus_callback) */
static void _dbus_signal_callback(GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *params,
                                     gpointer          userdata)
{
	struct callback_data *cb_data = (struct callback_data*) userdata;

	debug_log("Signal(%s.%s) Received , Let's call Wrapper-Callback", interface_name, signal_name);

	if (!strcmp(signal_name, g_events[AUDIO_EVENT_VOLUME_CHANGED].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_VOLUME_CHANGED, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_DEVICE_CONNECTED].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_DEVICE_CONNECTED, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_DEVICE_INFO_CHANGED].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_DEVICE_INFO_CHANGED, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_FOCUS_CHANGED].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_FOCUS_CHANGED, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_FOCUS_WATCH].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_FOCUS_WATCH, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_PLAY_FILE_END].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_PLAY_FILE_END, params, cb_data->user_data);
	} else if (!strcmp(signal_name, g_events[AUDIO_EVENT_EMERGENT_EXIT].name)) {
		(cb_data->user_cb)(AUDIO_EVENT_EMERGENT_EXIT, params, cb_data->user_data);
	}
}

static void callback_data_free_func(gpointer data)
{
	struct callback_data *cb_data = (struct callback_data *) data;

	if (cb_data->free_func)
		cb_data->free_func(cb_data->user_data);
	g_free(cb_data);
}

EXPORT_API
int mm_sound_dbus_signal_subscribe_to(audio_provider_t provider, audio_event_t event, mm_sound_dbus_callback callback, void *userdata, mm_sound_dbus_userdata_free freefunc, unsigned *subs_id)
{
	GDBusConnection *conn = NULL;
	guint _subs_id = 0;
	struct callback_data *cb_data = NULL;

	if (!callback) {
		debug_error("Callback is Null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (event < 0 || event >= AUDIO_EVENT_MAX) {
		debug_error("Wrong event Type : %d", event);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (provider < 0 || provider >= AUDIO_PROVIDER_MAX) {
		debug_error("Invalid provider : %d", provider);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!(conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	cb_data = (struct callback_data*) g_malloc0(sizeof(struct callback_data));
	cb_data->user_cb = callback;
	cb_data->user_data = userdata;
	cb_data->free_func = freefunc;

	if(_dbus_subscribe_signal(conn, g_paths[provider].object, g_paths[provider].interface, g_events[event].name,
							_dbus_signal_callback, &_subs_id, cb_data, callback_data_free_func) != MM_ERROR_NONE) {
		debug_error("Dbus Subscribe on Client Error");
		goto fail;
	}
	if (subs_id)
		*subs_id = (unsigned int)_subs_id;

	return MM_ERROR_NONE;

fail:
	if (cb_data)
		free(cb_data);
	return MM_ERROR_SOUND_INTERNAL;
}

EXPORT_API
int mm_sound_dbus_signal_unsubscribe(unsigned int subs_id)
{
	GDBusConnection *conn = NULL;

	if (!(conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	_dbus_unsubscribe_signal(conn, (guint) subs_id);

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_sound_dbus_emit_signal(audio_provider_t provider, audio_event_t event, GVariant *param)
{
	GDBusConnection *conn;
	GError *err = NULL;
	gboolean dbus_ret;
	int ret = MM_ERROR_NONE;

	if (event < 0 || event >= AUDIO_EVENT_MAX) {
		debug_error ("invalid argument, event_type(%d)", event);
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto end;
	}

	if (!(conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		debug_error("Get Dbus Connection Error");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto end;
	}

	dbus_ret = g_dbus_connection_emit_signal (conn,
						  NULL, g_paths[provider].object,
						  g_paths[provider].interface, g_events[event].name,
						  param, &err);
	if (!dbus_ret) {
		debug_error ("g_dbus_connection_emit_signal() error (%s)", err->message);
		ret = MM_ERROR_SOUND_INTERNAL;
	}

end:
	debug_msg ("emit signal for [%s]  %s", g_events[event].name, (ret == MM_ERROR_NONE ? "success" : "failed") );
	return ret;
}

