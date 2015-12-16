#include <gio/gio.h>
#include <glib.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "include/mm_sound_client_dbus.h"
#include "include/mm_sound_device.h"
#include "include/mm_sound_msg.h"
#include "include/mm_sound_common.h"

#ifdef USE_SECURITY
#include <security-server.h>
#define COOKIE_SIZE 20
#endif


#define BUS_NAME_PULSEAUDIO "org.pulseaudio.Server"
#define OBJECT_PULSE_MODULE_POLICY "/org/pulseaudio/policy1"
#define INTERFACE_PULSE_MODULE_POLICY "org.PulseAudio.Ext.Policy1"
#define OBJECT_PULSE_MODULE_DEVICE_MANAGER "/org/pulseaudio/DeviceManager"
#define INTERFACE_PULSE_MODULE_DEVICE_MANAGER "org.pulseaudio.DeviceManager"
#define OBJECT_PULSE_MODULE_STREAM_MANAGER "/org/pulseaudio/StreamManager"
#define INTERFACE_PULSE_MODULE_STREAM_MANAGER "org.pulseaudio.StreamManager"

#define BUS_NAME_SOUND_SERVER "org.tizen.SoundServer"
#define OBJECT_SOUND_SERVER "/org/tizen/SoundServer1"
#define INTERFACE_SOUND_SERVER "org.tizen.SoundServer1"

#define BUS_NAME_FOCUS_SERVER "org.tizen.FocusServer"
#define OBJECT_FOCUS_SERVER "/org/tizen/FocusServer1"
#define INTERFACE_FOCUS_SERVER "org.tizen.FocusServer1"

#define INTERFACE_DBUS			"org.freedesktop.DBus.Properties"
#define SIGNAL_PROP_CHANGED "PropertiesChanged"
#define METHOD_GET			"Get"
#define METHOD_SET			"Set"
#define DBUS_NAME_MAX                   32
#define DBUS_SIGNATURE_MAX              32
#define ERR_MSG_MAX                     100

#define CODEC_HANDLE_MAX 256
enum {
	DBUS_TO_SOUND_SERVER,
	DBUS_TO_PULSE_MODULE_DEVICE_MANAGER,
	DBUS_TO_PULSE_MODULE_STREAM_MANAGER,
	DBUS_TO_PULSE_MODULE_POLICY,
	DBUS_TO_FOCUS_SERVER,
};

struct user_callback {
	int sig_type;
	void *cb;
	void *userdata;
	int mask;
};

guint g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MAX];

const struct mm_sound_dbus_method_info g_methods[METHOD_CALL_MAX] = {
	[METHOD_CALL_TEST] = {
		.name = "MethodTest1",
	},
	[METHOD_CALL_PLAY_FILE_START] = {
		.name = "PlayFileStart",
	},
	[METHOD_CALL_PLAY_FILE_START_WITH_STREAM_INFO] = {
		.name = "PlayFileStartWithStreamInfo",
	},
	[METHOD_CALL_PLAY_FILE_STOP] = {
		.name = "PlayFileStop",
	},
	[METHOD_CALL_PLAY_DTMF] = {
		.name = "PlayDTMF",
	},
	[METHOD_CALL_PLAY_DTMF_WITH_STREAM_INFO] = {
		.name = "PlayDTMFWithStreamInfo",
	},
	[METHOD_CALL_CLEAR_FOCUS] = {
		.name = "ClearFocus",
	},
	[METHOD_CALL_GET_BT_A2DP_STATUS] = {
		.name = "GetBTA2DPStatus",
	},
	[METHOD_CALL_SET_PATH_FOR_ACTIVE_DEVICE] = {
		.name = "SetPathForActiveDevice",
	},
	[METHOD_CALL_GET_AUDIO_PATH] = {
		.name = "GetAudioPath",
	},
	[METHOD_CALL_SET_VOLUME_LEVEL] = {
		.name = "SetVolumeLevel",
	},
	[METHOD_CALL_GET_CONNECTED_DEVICE_LIST] = {
		.name = "GetConnectedDeviceList",
	},
	[METHOD_CALL_GET_UNIQUE_ID] = {
		.name = "GetUniqueId",
	},
	[METHOD_CALL_REGISTER_FOCUS] = {
		.name = "RegisterFocus",
	},
	[METHOD_CALL_UNREGISTER_FOCUS] = {
		.name = "UnregisterFocus",
	},
	[METHOD_CALL_SET_FOCUS_REACQUISITION] = {
		.name = "SetFocusReacquisition",
	},
	[METHOD_CALL_GET_ACQUIRED_FOCUS_STREAM_TYPE] = {
		.name = "GetAcquiredFocusStreamType",
	},
	[METHOD_CALL_ACQUIRE_FOCUS] = {
		.name = "AcquireFocus",
	},
	[METHOD_CALL_RELEASE_FOCUS] = {
		.name = "ReleaseFocus",
	},
	[METHOD_CALL_WATCH_FOCUS] = {
		.name = "WatchFocus",
	},
	[METHOD_CALL_UNWATCH_FOCUS] = {
		.name = "UnwatchFocus",
	},
	[METHOD_CALL_EMERGENT_EXIT_FOCUS] = {
		.name = "EmergentExitFocus",
	},
};

const struct mm_sound_dbus_signal_info g_signals[SIGNAL_MAX] = {
    [SIGNAL_TEST] = {
		.name = "SignalTest1",
	},
	[SIGNAL_PLAY_FILE_END] = {
		.name = "PlayFileEnd",
	},
	[SIGNAL_VOLUME_CHANGED] = {
		.name = "VolumeChanged",
	},
	[SIGNAL_DEVICE_CONNECTED] = {
		.name = "DeviceConnected",
	},
	[SIGNAL_DEVICE_INFO_CHANGED] = {
		.name = "DeviceInfoChanged",
	},
	[SIGNAL_FOCUS_CHANGED] = {
		.name = "FocusChanged",
	},
	[SIGNAL_FOCUS_WATCH] = {
		.name = "FocusWatch",
	}
};

const struct pulseaudio_dbus_property_info g_pulseaudio_properties[PULSEAUDIO_PROP_MAX] = {
	[PULSEAUDIO_PROP_AUDIO_BALANCE] = {
		.name = "AudioBalance",
	},
	[PULSEAUDIO_PROP_MONO_AUDIO] = {
		.name = "MonoAudio",
	},
	[PULSEAUDIO_PROP_MUTE_ALL] = {
		.name = "MuteAll",
	},
};

/*
static const GDBusErrorEntry mm_sound_client_error_entries[] =
{
	{MM_ERROR_SOUND_INTERNAL, "org.tizen.mm.sound.Error.Internal"},
	{MM_ERROR_INVALID_ARGUMENT, "org.tizen.mm.sound.Error.InvalidArgument"},
};
*/


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
		else if (!object)
			debug_error("object null");
		else if (!intf)
			debug_error("intf null");
		else if (!method)
			debug_error("method null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus call with obj'%s' intf'%s' method'%s'", object, intf, method);

	dbus_reply = g_dbus_connection_call_sync(conn, bus_name, object , intf, \
			     method, args ,\
			     NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err );
	if (dbus_reply && !err) {
		debug_log("Method Call '%s.%s' Success", intf, method);
		*result = dbus_reply;
	} else {
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

	return ret;
}

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

static int _dbus_subscribe_signal(GDBusConnection *conn, const char* object_name, const char* intf_name,
					const char* signal_name, GDBusSignalCallback signal_cb, guint *subscribe_id, void* userdata)
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
	} else {
		if (subscribe_id)
			*subscribe_id = subs_id;
	}

	return MM_ERROR_NONE;
}

static void _dbus_unsubscribe_signal(GDBusConnection *conn, guint subs_id)
{
	if (!conn || !subs_id) {
		debug_error("Invalid Argument");
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

static void _dbus_disconnect(GDBusConnection* conn)
{
	debug_fenter ();
	g_object_unref(conn);
	debug_fleave ();
}

/******************************************************************************************
		Simple Functions For Communicate with Sound-Server
******************************************************************************************/

static int _dbus_method_call_to(int dbus_to, int method_type, GVariant *args, GVariant **result)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;
	const char *bus_name, *object, *interface;

	if (method_type < 0 || method_type > METHOD_CALL_MAX) {
		debug_error("Invalid method type");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (dbus_to == DBUS_TO_SOUND_SERVER) {
		bus_name = BUS_NAME_SOUND_SERVER;
		object = OBJECT_SOUND_SERVER;
		interface = INTERFACE_SOUND_SERVER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_DEVICE_MANAGER) {
		bus_name = BUS_NAME_PULSEAUDIO;
		object = OBJECT_PULSE_MODULE_DEVICE_MANAGER;
		interface = INTERFACE_PULSE_MODULE_DEVICE_MANAGER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_STREAM_MANAGER) {
		bus_name = BUS_NAME_PULSEAUDIO;
		object = OBJECT_PULSE_MODULE_STREAM_MANAGER;
		interface = INTERFACE_PULSE_MODULE_STREAM_MANAGER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_POLICY) {
		bus_name = BUS_NAME_PULSEAUDIO;
		object = OBJECT_PULSE_MODULE_POLICY;
		interface = INTERFACE_PULSE_MODULE_POLICY;
	} else if (dbus_to == DBUS_TO_FOCUS_SERVER) {
		bus_name = BUS_NAME_FOCUS_SERVER;
		object = OBJECT_FOCUS_SERVER;
		interface = INTERFACE_FOCUS_SERVER;
	} else {
		debug_error("Invalid case, dbus_to %d", dbus_to);
		return MM_ERROR_SOUND_INTERNAL;
	}

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if((ret = _dbus_method_call(conn, bus_name,
					      object,
					      interface,
					      g_methods[method_type].name,
					      args, result)) != MM_ERROR_NONE) {
			debug_error("Dbus Call on Client Error");
			return ret;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

static void _dbus_signal_callback (GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *params,
                                     gpointer          user_data)
{
	struct user_callback* user_cb = (struct user_callback*) user_data;

	if (!user_cb || !user_cb->cb) {
		debug_error("User callback data Null");
		return;
	}

	if (user_cb->sig_type < 0 || user_cb->sig_type >= SIGNAL_MAX) {
		debug_error("Wrong Signal Type");
		return;
	}

	if (!params) {
		debug_error("Parameter Null");
		return;
	}

	debug_log("Signal(%s.%s) Received , Let's call Wrapper-Callback", interface_name, signal_name);

	if (user_cb->sig_type == SIGNAL_VOLUME_CHANGED) {
		char *volume_type_str = NULL, *direction = NULL;
		unsigned int volume_level;

		g_variant_get(params, "(&s&su)", &direction, &volume_type_str, &volume_level);
		((mm_sound_volume_changed_wrapper_cb)(user_cb->cb))(direction, volume_type_str, volume_level, user_cb->userdata);
	} else if (user_cb->sig_type == SIGNAL_DEVICE_CONNECTED) {
		const char *name = NULL, *device_type = NULL;
		gboolean is_connected = FALSE;
		int device_id, io_direction, state;

		g_variant_get(params, "((i&sii&s)b)", &device_id, &device_type, &io_direction,
					&state, &name, &is_connected);
		((mm_sound_device_connected_wrapper_cb)(user_cb->cb))(device_id, device_type, io_direction, state, name, is_connected, user_cb->userdata);
	} else if (user_cb->sig_type == SIGNAL_DEVICE_INFO_CHANGED) {
		const char *name = NULL, *device_type = NULL;
		int changed_device_info_type = 0;
		int device_id, io_direction, state;

		g_variant_get(params, "((i&sii&s)i)", &device_id, &device_type, &io_direction,
					&state, &name, &changed_device_info_type);
		((mm_sound_device_info_changed_wrapper_cb)(user_cb->cb))(device_id, device_type, io_direction, state, name, changed_device_info_type, user_cb->userdata);
	} else if (user_cb->sig_type == SIGNAL_FOCUS_CHANGED) {
	} else if (user_cb->sig_type == SIGNAL_FOCUS_WATCH) {
	} else if (user_cb->sig_type == SIGNAL_TEST) {
		int test_var = 0;
		g_variant_get(params, "(i)", &test_var);
		((mm_sound_test_cb)(user_cb->cb))(test_var, user_cb->userdata);
	} else if (user_cb->sig_type == SIGNAL_PLAY_FILE_END) {
		int ended_handle = 0;
		g_variant_get(params, "(i)", &ended_handle);
		((mm_sound_stop_callback_wrapper_func)(user_cb->cb))(ended_handle, user_cb->userdata);
	}
}

static int _dbus_signal_subscribe_to(int dbus_to, sound_server_signal_t signaltype, void *cb, void *userdata, int mask, unsigned int *subs_id)
{
	GDBusConnection *conn = NULL;
	guint _subs_id = 0;
	struct user_callback *user_cb  = NULL;
	const char *object, *interface;

	if (!cb) {
		debug_error("Callback data Null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (signaltype < 0 || signaltype >= SIGNAL_MAX) {
		debug_error("Wrong Signal Type");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!(user_cb = g_malloc0(sizeof(struct user_callback)))) {
		debug_error("Allocate Memory for User CB data Failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	user_cb->sig_type = signaltype;
	user_cb->cb = cb;
	user_cb->userdata = userdata;
	user_cb->mask = mask;


	if (dbus_to == DBUS_TO_SOUND_SERVER) {
		object = OBJECT_SOUND_SERVER;
		interface = INTERFACE_SOUND_SERVER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_DEVICE_MANAGER) {
		object = OBJECT_PULSE_MODULE_DEVICE_MANAGER;
		interface = INTERFACE_PULSE_MODULE_DEVICE_MANAGER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_STREAM_MANAGER) {
		object = OBJECT_PULSE_MODULE_STREAM_MANAGER;
		interface = INTERFACE_PULSE_MODULE_STREAM_MANAGER;
	} else if (dbus_to == DBUS_TO_PULSE_MODULE_POLICY) {
		object = OBJECT_PULSE_MODULE_POLICY;
		interface = INTERFACE_PULSE_MODULE_POLICY;
	} else {
		debug_error("Invalid case, dbus_to %d", dbus_to);
		return MM_ERROR_SOUND_INTERNAL;
	}


	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if(_dbus_subscribe_signal(conn, object, interface, g_signals[signaltype].name,
								_dbus_signal_callback, &_subs_id, user_cb) != MM_ERROR_NONE) {
			debug_error("Dbus Subscribe on Client Error");
			return MM_ERROR_SOUND_INTERNAL;
		} else {
			if (subs_id)
				*subs_id = (unsigned int)_subs_id;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return MM_ERROR_NONE;
}

static int _dbus_signal_unsubscribe(unsigned int subs_id)
{
	GDBusConnection *conn = NULL;

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		_dbus_unsubscribe_signal(conn, (guint) subs_id);
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

static int _pulseaudio_dbus_set_property(pulseaudio_property_t property, GVariant* args, GVariant **result)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;

	if (property < 0 || property > PULSEAUDIO_PROP_MAX) {
		debug_error("Invalid property [%d]", property);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (args == NULL) {
		debug_error("Invalid args");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if((ret = _dbus_set_property(conn, BUS_NAME_PULSEAUDIO, OBJECT_PULSE_MODULE_POLICY, INTERFACE_PULSE_MODULE_POLICY,
									g_pulseaudio_properties[property].name, args, result)) != MM_ERROR_NONE) {
			debug_error("Dbus Call on Client Error");
			return ret;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

static int _pulseaudio_dbus_get_property(pulseaudio_property_t property, GVariant **result)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;

	if (property < 0 || property > PULSEAUDIO_PROP_MAX) {
		debug_error("Invalid property [%d]", property);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if((ret = _dbus_get_property(conn, BUS_NAME_PULSEAUDIO, OBJECT_PULSE_MODULE_POLICY, INTERFACE_PULSE_MODULE_POLICY,
									g_pulseaudio_properties[property].name, result)) != MM_ERROR_NONE) {
			debug_error("Dbus Call on Client Error");
			return ret;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	return MM_ERROR_NONE;
}

/******************************************************************************************
		Implementation of each dbus client code (Construct Params,..)
******************************************************************************************/

int mm_sound_client_dbus_add_test_callback(mm_sound_test_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _dbus_signal_subscribe_to(DBUS_TO_SOUND_SERVER, SIGNAL_TEST, func, user_data, 0, subs_id)) != MM_ERROR_NONE) {
		debug_error("add test callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_test_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove test callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_test(int a, int b, int *get)
{
	int ret = MM_ERROR_NONE;
	int reply = 0;
	GVariant *params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(ii)", a, b);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_TEST, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus test call failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if (result) {
		g_variant_get(result, "(i)",  &reply);
		debug_log("reply : %d", reply);
		*get = reply;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_get_current_connected_device_list(int device_flags, GList** device_list)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL, *child = NULL;
	GVariant *params;
	GVariantIter iter;
	mm_sound_device_t* device_item;
	const gchar *device_name_tmp = NULL, *device_type_tmp = NULL;

	debug_fenter();

	if (!device_list) {
		debug_error("Invalid Parameter, device_list null");
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto cleanup;
	}

	params = g_variant_new("(i)", device_flags);

	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_PULSE_MODULE_DEVICE_MANAGER, METHOD_CALL_GET_CONNECTED_DEVICE_LIST, params, &result)) != MM_ERROR_NONE) {
			debug_error("Get current connected device list failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for get current connected device failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	child = g_variant_get_child_value(result, 0);
	g_variant_iter_init(&iter, child);
	while (1) {
		device_item = g_malloc0(sizeof(mm_sound_device_t));
		if (device_item && g_variant_iter_loop(&iter, "(i&sii&s)", &device_item->id, &device_type_tmp, &device_item->io_direction, &device_item->state, &device_name_tmp)) {
			MMSOUND_STRNCPY(device_item->name, device_name_tmp, MAX_DEVICE_NAME_NUM);
			MMSOUND_STRNCPY(device_item->type, device_type_tmp, MAX_DEVICE_TYPE_STR_LEN);
			*device_list = g_list_append(*device_list, device_item);
			debug_log("Added device id(%d) type(%17s) direction(%d) state(%d) name(%s)", device_item->id, device_item->type,device_item->io_direction, device_item->state, device_item->name);
		} else {
			if (device_item)
				g_free(device_item);
			break;
		}
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_add_device_connected_callback(int device_flags, mm_sound_device_connected_wrapper_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _dbus_signal_subscribe_to(DBUS_TO_PULSE_MODULE_DEVICE_MANAGER, SIGNAL_DEVICE_CONNECTED, func, user_data, device_flags, subs_id)) != MM_ERROR_NONE) {
		debug_error("add device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_device_connected_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_wrapper_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_subscribe_to(DBUS_TO_PULSE_MODULE_DEVICE_MANAGER, SIGNAL_DEVICE_INFO_CHANGED, func, user_data, device_flags, subs_id)) != MM_ERROR_NONE) {
		debug_error("Add device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_device_info_changed_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_set_volume_by_type(const char *volume_type, const unsigned int volume_level)
{
	int ret = MM_ERROR_NONE;
	char *reply = NULL, *direction = "out";
	GVariant *params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(ssu)", direction, volume_type, volume_level);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_PULSE_MODULE_STREAM_MANAGER, METHOD_CALL_SET_VOLUME_LEVEL, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus test call failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if (result) {
		g_variant_get(result, "(&s)",  &reply);
		debug_log("reply : %s", reply);
		if (!strcmp(reply, "STREAM_MANAGER_RETURN_ERROR"))
			ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_add_volume_changed_callback(mm_sound_volume_changed_wrapper_cb func, void* user_data, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _dbus_signal_subscribe_to(DBUS_TO_PULSE_MODULE_STREAM_MANAGER, SIGNAL_VOLUME_CHANGED, func, user_data, 0, subs_id)) != MM_ERROR_NONE) {
		debug_error("Add Volume changed callback failed");
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_remove_volume_changed_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("Remove Volume changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_GET_AUDIO_PATH, NULL, &result)) != MM_ERROR_NONE) {
		debug_error("add device connected callback failed");
		goto cleanup;
	}

	g_variant_get(result, "(ii)", device_in, device_out );

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_play_tone(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   bool enable_session, int *codechandle, char *stream_type, int stream_index)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;
	gboolean _enable_session = enable_session;

	if (!codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fenter();

	params = g_variant_new("(iiiiiiibsi)", tone, repeat, volume,
		      volume_config, session_type, session_options, client_pid , _enable_session, stream_type, stream_index);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_PLAY_DTMF, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus play tone failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (result) {
		g_variant_get(result, "(i)",  &handle);
		debug_log("handle : %d", handle);
		*codechandle = handle;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;


}

int mm_sound_client_dbus_play_tone_with_stream_info(int client_pid, int tone, char *stream_type, int stream_index, int volume, int repeat, int *codechandle)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	if (!codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	params = g_variant_new("(iiiisi)", tone, repeat, volume, client_pid, stream_type, stream_index);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_PLAY_DTMF_WITH_STREAM_INFO, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus play tone failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (result) {
		g_variant_get(result, "(i)",  &handle);
		debug_log("handle : %d", handle);
		*codechandle = handle;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;


}

int mm_sound_client_dbus_play_sound(const char* filename, int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int handle_route,
			   bool enable_session, int *codechandle, char *stream_type, int stream_index)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;
	gboolean _enable_session = enable_session;

	if (!filename || !codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fenter();

	params = g_variant_new("(siiiiiiiiibsi)", filename, tone, repeat, volume,
		      volume_config, priority, session_type, session_options, client_pid, handle_route, _enable_session, stream_type, stream_index);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_PLAY_FILE_START, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus play file failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (result) {
		g_variant_get(result, "(i)",  &handle);
		debug_log("handle : %d", handle);
		*codechandle = handle;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_play_sound_with_stream_info(const char* filename, int repeat, int volume,
				int priority, int client_pid, int handle_route, int *codechandle, char *stream_type, int stream_index)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;

	if (!filename || !codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fenter();

	params = g_variant_new("(siiiiisi)", filename, repeat, volume,
			priority, client_pid, handle_route, stream_type, stream_index);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_PLAY_FILE_START_WITH_STREAM_INFO, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus play file failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (result) {
		g_variant_get(result, "(i)",  &handle);
		debug_log("handle : %d", handle);
		*codechandle = handle;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;


}


int mm_sound_client_dbus_stop_sound(int handle)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_PLAY_FILE_STOP, g_variant_new("(i)", handle), &result)) != MM_ERROR_NONE) {
		debug_error("dbus stop file playing failed");
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_clear_focus(int pid)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = _dbus_method_call_to(DBUS_TO_SOUND_SERVER, METHOD_CALL_CLEAR_FOCUS, g_variant_new("(i)", pid), &result)) != MM_ERROR_NONE) {
		debug_error("dbus clear focus failed");
	}

	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_is_route_available(mm_sound_route route, bool *is_available)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_foreach_available_route_cb(mm_sound_available_route_cb avail_cb, void *user_data)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_set_active_route(mm_sound_route route, bool need_broadcast)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_add_play_sound_end_callback(mm_sound_stop_callback_wrapper_func stop_cb, void* userdata, unsigned int *subs_id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _dbus_signal_subscribe_to(DBUS_TO_SOUND_SERVER, SIGNAL_PLAY_FILE_END, stop_cb, userdata, 0, subs_id)) != MM_ERROR_NONE) {
		debug_error("add play sound end callback failed");
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_remove_play_sound_end_callback(unsigned int subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("Remove Play File End callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_remove_active_device_changed_callback(const char *name)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_remove_available_route_changed_callback(void)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

int mm_sound_client_dbus_set_active_route_auto(void)
{
	return MM_ERROR_NOT_SUPPORT_API;
}

/*------------------------------------------ FOCUS --------------------------------------------------*/
#ifdef USE_FOCUS

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
char* _get_cookie(int cookie_size)
{
	int retval = -1;
	char* cookie = NULL;

	if (security_server_get_cookie_size() != cookie_size) {
		debug_error ("[Security] security_server_get_cookie_size() != COOKIE_SIZE(%d)\n", cookie_size);
		return false;
	}

	cookie = (char*)malloc (cookie_size);

	retval = security_server_request_cookie (cookie, cookie_size);
	if (retval == SECURITY_SERVER_API_SUCCESS) {
		debug_msg ("[Security] security_server_request_cookie() returns [%d]\n", retval);
	} else {
		debug_error ("[Security] security_server_request_cookie() returns [%d]\n", retval);
	}

	return cookie;
}

static GVariant* _get_cookie_variant ()
{
	int i;
	GVariantBuilder builder;
	char* cookie = NULL;

	cookie = _get_cookie(COOKIE_SIZE);

	if (cookie == NULL)
		return NULL;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	for (i = 0; i < COOKIE_SIZE; i++)
		g_variant_builder_add(&builder, "y", cookie[i]);

	free (cookie);
	return g_variant_builder_end(&builder);
}

#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */

int mm_sound_client_dbus_get_unique_id(int *id)
{
	int ret = MM_ERROR_NONE;
	int res = 0;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_GET_UNIQUE_ID, NULL, &result)) != MM_ERROR_NONE) {
		debug_error("dbus get unique id failed");
	}

	if (result) {
		g_variant_get(result, "(i)", &res);
		*id = res;
		debug_msg("got unique id(%d)", *id);
		g_variant_unref(result);
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_register_focus(int id, int instance, const char *stream_type, mm_sound_focus_changed_cb callback, bool is_for_session, void* user_data)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;
#ifdef SUPPORT_CONTAINER
	char container[128];
#endif

	debug_fenter();

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	params = g_variant_new("(@ayiisb)", _get_cookie_variant(), instance, id, stream_type, is_for_session);
#else /* USE_SECURITY */
	gethostname(container, sizeof(container));
	debug_error("container = %s", container);
	params = g_variant_new("(siisb)", container, instance, id, stream_type, is_for_session);
#endif /* USE_SECURITY */

#else /* SUPPORT_CONTAINER */
	params = g_variant_new("(iisb)", instance, id, stream_type, is_for_session);

#endif /* SUPPORT_CONTAINER */

	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_REGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus register focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if(ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();

	return ret;

}

int mm_sound_client_dbus_unregister_focus(int instance, int id, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", instance, id, is_for_session);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_UNREGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus unregister focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_set_foucs_reacquisition(int instance, int id, bool reacquisition)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", instance, id, reacquisition);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_SET_FOCUS_REACQUISITION, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus set focus reacquisition failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_get_acquired_focus_stream_type(int focus_type, char **stream_type, char **additional_info)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(i)", focus_type);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_GET_ACQUIRED_FOCUS_STREAM_TYPE, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus get stream type of acquired focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE && result)
		g_variant_get(result, "(ss)", stream_type, additional_info);
	else
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_acquire_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iiisb)", instance, id, type, option, is_for_session);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_ACQUIRE_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus acquire focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_release_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iiisb)", instance, id, type, option, is_for_session);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_RELEASE_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus release focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_set_focus_watch_callback(int instance, int handle, mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, bool is_for_session, void* user_data)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;
#ifdef SUPPORT_CONTAINER
	char container[128];
#endif

	debug_fenter();
#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	params = g_variant_new("(@ayiiib)", _get_cookie_variant(), instance, handle, type, is_for_session);
#else /* USE_SECURITY */
	gethostname(container, sizeof(container));
	debug_error("container = %s", container);
	params = g_variant_new("(siiib)", container, instance, handle, type, is_for_session);
#endif /* USE_SECURITY */

#else /* SUPPORT_CONTAINER */
	params = g_variant_new("(iiib)", instance, handle, type, is_for_session);
#endif /* SUPPORT_CONTAINER */

	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_WATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus set watch focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);
	debug_fleave();

	return ret;

}

int mm_sound_client_dbus_unset_focus_watch_callback(int focus_tid, int handle, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", focus_tid, handle, is_for_session);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_UNWATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus unset watch focus failed");
		}
	} else {
		debug_error("Construct Param for method call failed");
	}
	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_emergent_exit_focus(int exit_pid)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(i)", exit_pid);
	if (params) {
		if ((ret = _dbus_method_call_to(DBUS_TO_FOCUS_SERVER, METHOD_CALL_EMERGENT_EXIT_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus emergent exit focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

cleanup:
	if (ret != MM_ERROR_NONE)
		g_variant_get(result, "(i)",  &ret);
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

#endif /* USE_FOCUS */
/*------------------------------------------ FOCUS --------------------------------------------------*/

int mm_sound_client_dbus_initialize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_finalize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_fleave();

	return ret;
}
