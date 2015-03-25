#include <gio/gio.h>
#include <glib.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "include/mm_sound_client_dbus.h"
#include "include/mm_sound_device.h"
#include "include/mm_sound_msg.h"
#include "include/mm_sound_common.h"

#define BUS_NAME_PULSEAUDIO "org.pulseaudio.Server"
#define OBJECT_PULSEAUDIO "/org/pulseaudio/policy1"
#define INTERFACE_PULSEAUDIO "org.PulseAudio.Ext.Policy1"

#define BUS_NAME_SOUND_SERVER "org.tizen.SoundServer"
#define OBJECT_SOUND_SERVER "/org/tizen/SoundServer1"
#define INTERFACE_SOUND_SERVER "org.tizen.SoundServer1"

#define INTERFACE_DBUS			"org.freedesktop.DBus.Properties"
#define SIGNAL_PROP_CHANGED "PropertiesChanged"
#define METHOD_GET			"Get"
#define METHOD_SET			"Set"
#define DBUS_NAME_MAX                   32
#define DBUS_SIGNATURE_MAX              32

#define FOCUS_HANDLE_MAX 256
#define FOCUS_HANDLE_INIT_VAL -1

#define CONFIG_ENABLE_RETCB



struct user_callback {
	int sig_type;
	void *cb;
	void *userdata;
	int mask;
};

typedef gboolean (*focus_gLoopPollHandler_t)(gpointer d);

typedef struct
{
	int focus_tid;
	int handle;
	int focus_fd;
	GSourceFuncs* g_src_funcs;
	GPollFD* g_poll_fd;
	GSource* focus_src;
	bool is_used;
	GMutex* focus_lock;
	mm_sound_focus_changed_cb focus_callback;
	mm_sound_focus_changed_watch_cb watch_callback;
	void* user_data;
} focus_sound_info_t;

typedef struct {
	int pid;
	int handle;
	int type;
	int state;
	char stream_type [MAX_STREAM_TYPE_LEN];
	char name [MM_SOUND_NAME_NUM];
} focus_cb_data_lib;


GThread *g_focus_thread;
GMainLoop *g_focus_loop;
focus_sound_info_t g_focus_sound_handle[FOCUS_HANDLE_MAX];
guint g_dbus_subs_ids[SOUND_SERVER_SIGNAL_MAX];
guint g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MAX];
GQuark g_mm_sound_error_quark;

const struct mm_sound_dbus_method_info g_sound_server_methods[SOUND_SERVER_METHOD_MAX] = {
	[SOUND_SERVER_METHOD_TEST] = {
		.name = "MethodTest1",
	},
	[SOUND_SERVER_METHOD_PLAY_FILE_START] = {
		.name = "PlayFileStart",
	},
	[SOUND_SERVER_METHOD_PLAY_FILE_STOP] = {
		.name = "PlayFileStop",
	},
	[SOUND_SERVER_METHOD_PLAY_DTMF] = {
		.name = "PlayDTMF",
	},
	[SOUND_SERVER_METHOD_GET_BT_A2DP_STATUS] = {
		.name = "GetBTA2DPStatus",
	},
	[SOUND_SERVER_METHOD_SET_PATH_FOR_ACTIVE_DEVICE] = {
		.name = "SetPathForActiveDevice",
	},
	[SOUND_SERVER_METHOD_GET_AUDIO_PATH] = {
		.name = "GetAudioPath",
	},
	[SOUND_SERVER_METHOD_GET_CONNECTED_DEVICE_LIST] = {
		.name = "GetConnectedDeviceList",
	},
	[SOUND_SERVER_METHOD_REGISTER_FOCUS] = {
		.name = "RegisterFocus",
	},
	[SOUND_SERVER_METHOD_UNREGISTER_FOCUS] = {
		.name = "UnregisterFocus",
	},
	[SOUND_SERVER_METHOD_ACQUIRE_FOCUS] = {
		.name = "AcquireFocus",
	},
	[SOUND_SERVER_METHOD_RELEASE_FOCUS] = {
		.name = "ReleaseFocus",
	},
	[SOUND_SERVER_METHOD_WATCH_FOCUS] = {
		.name = "WatchFocus",
	},
	[SOUND_SERVER_METHOD_UNWATCH_FOCUS] = {
		.name = "UnwatchFocus",
	},
};

const struct mm_sound_dbus_signal_info g_sound_server_signals[SOUND_SERVER_SIGNAL_MAX] = {
    [SOUND_SERVER_SIGNAL_TEST] = {
		.name = "SignalTest1",
	},
	[SOUND_SERVER_SIGNAL_PLAY_FILE_END] = {
		.name = "PlayFileEnd",
	},
	[SOUND_SERVER_SIGNAL_VOLUME_CHANGED] = {
		.name = "VolumeChanged",
	},
	[SOUND_SERVER_SIGNAL_DEVICE_CONNECTED] = {
		.name = "DeviceConnected",
	},
	[SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED] = {
		.name = "DeviceInfoChanged",
	},
	[SOUND_SERVER_SIGNAL_FOCUS_CHANGED] = {
		.name = "FocusChanged",
	},
	[SOUND_SERVER_SIGNAL_FOCUS_WATCH] = {
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


static GQuark _dbus_register_error_domain (void)
{
	static volatile gsize quark_volatile = 0;
	g_dbus_error_register_error_domain("mm-sound-error-quark",
					   &quark_volatile,
					   mm_sound_error_entries,
					   G_N_ELEMENTS(mm_sound_error_entries));
	return (GQuark) quark_volatile;
}

static int _dbus_convert_error(GError *err)
{
	if (g_dbus_error_is_remote_error(err)) {
		if (err->domain == g_mm_sound_error_quark) {
			return err->code;
		} else{
			debug_error("Unknown Error domain");
			return MM_ERROR_SOUND_INTERNAL;
		}
	} else {
		debug_log("Dbus error");
		return MM_ERROR_SOUND_INTERNAL;
	}
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
		debug_error ("g_dbus_connection_call_sync() error (%s)(%s) ",  g_quark_to_string(err->domain), err->message);
		ret = _dbus_convert_error(err);
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

	if (!conn || !object_name || !intf_name || !signal_name || !signal_cb || !subscribe_id) {
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

/* Hardcoded method call Just for communicating with Sound-server*/
static int _sound_server_dbus_method_call(int method_type, GVariant* args, GVariant **result)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;

	if (method_type < 0 || method_type > SOUND_SERVER_METHOD_MAX) {
		debug_error("Invalid method type");
		return MM_ERROR_INVALID_ARGUMENT;
	}

/*
	if (sound_server_methods[method_type].argument && !args) {
		debug_error("Argument for Method Null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (sound_server_methods[method_type].argument && !g_variant_is_of_type(args, G_VARIANT_TYPE(sound_server_methods[method_type].argument))) {
		debug_error("Wrong Argument for Method");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	*/

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if((ret = _dbus_method_call(conn, BUS_NAME_SOUND_SERVER,
					      OBJECT_SOUND_SERVER,
					      INTERFACE_SOUND_SERVER,
					      g_sound_server_methods[method_type].name,
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

static void _sound_server_dbus_signal_callback (GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *params,
                                     gpointer          user_data)
{
	struct user_callback* user_cb = NULL;
	GVariantIter *iter = NULL;

	user_cb = (struct user_callback*) user_data;

	if (!user_cb || !user_cb->cb) {
		debug_error("User callback data Null");
		return;
	}

	if (user_cb->sig_type < 0 || user_cb->sig_type >= SOUND_SERVER_SIGNAL_MAX) {
		debug_error("Wrong Signal Type");
		return;
	}

	if (!params) {
		debug_error("Parameter Null");
		return;
	}

	debug_log("Signal(%s.%s) Received , Let's call real User-Callback", interface_name, signal_name);

	if (user_cb->sig_type == SOUND_SERVER_SIGNAL_VOLUME_CHANGED) {
		volume_type_t vol_type;
		unsigned int volume;

		g_variant_get(params, "(uu)", &vol_type, &volume);
		debug_log("Call usercallback for %s", g_sound_server_signals[user_cb->sig_type].name);
		((mm_sound_volume_changed_cb)(user_cb->cb))(vol_type, volume, user_cb->userdata);
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_DEVICE_CONNECTED) {
		mm_sound_device_t device_h;
		const char* name = NULL;
		gboolean is_connected = FALSE;

		g_variant_get(params, "((iiii&s)b)", &device_h.id, &device_h.type, &device_h.io_direction,
					&device_h.state, &name, &is_connected);
		MMSOUND_STRNCPY(device_h.name, name, MAX_DEVICE_NAME_NUM);
		((mm_sound_device_connected_cb)(user_cb->cb))(&device_h, is_connected, user_cb->userdata);

	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED) {
		mm_sound_device_t device_h;
		const char* name = NULL;
		int changed_device_info_type = 0;

		g_variant_get(params, "((iiii&s)i)", &device_h.id, &device_h.type, &device_h.io_direction,
					&device_h.state, &name, &changed_device_info_type);
		MMSOUND_STRNCPY(device_h.name, name, MAX_DEVICE_NAME_NUM);
		((mm_sound_device_info_changed_cb)(user_cb->cb))(&device_h, changed_device_info_type, user_cb->userdata);
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_FOCUS_CHANGED) {
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_FOCUS_WATCH) {
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_TEST) {
		int test_var = 0;
		g_variant_get(params, "(i)", &test_var);
		((mm_sound_test_cb)(user_cb->cb))(test_var, user_cb->userdata);
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_PLAY_FILE_END) {
		int ended_handle = 0;
		g_variant_get(params, "(i)", &ended_handle);
		if (ended_handle == user_cb->mask) {
			debug_log("Interested playing handle end : %d", ended_handle);
			((mm_sound_stop_callback_func)(user_cb->cb))(user_cb->userdata, ended_handle);
		} else {
			debug_log("Not interested playing handle : %d", ended_handle);
		}

	}
}

static int _sound_server_dbus_signal_subscribe(sound_server_signal_t signaltype, void *cb, void *userdata, int mask)
{
	GDBusConnection *conn = NULL;
	guint subs_id = 0;
	int instance = 0;
	struct user_callback *user_cb  = NULL;

	if (!cb) {
		debug_error("Callback data Null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (signaltype < 0 || signaltype >= SOUND_SERVER_SIGNAL_MAX) {
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

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if(_dbus_subscribe_signal(conn, OBJECT_SOUND_SERVER, INTERFACE_SOUND_SERVER, g_sound_server_signals[signaltype].name,
								_sound_server_dbus_signal_callback, &subs_id, user_cb) != MM_ERROR_NONE){
			debug_error("Dbus Subscribe on Client Error");
			return MM_ERROR_SOUND_INTERNAL;
		} else {
			g_dbus_subs_ids[signaltype] = subs_id;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return MM_ERROR_NONE;
}

static int _sound_server_dbus_signal_unsubscribe(sound_server_signal_t signaltype)
{
	GDBusConnection *conn = NULL;

	if (signaltype < 0 || signaltype >= SOUND_SERVER_SIGNAL_MAX) {
		debug_error("Wrong Signal Type");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		_dbus_unsubscribe_signal(conn, g_dbus_subs_ids[signaltype]);
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}
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
		if((ret = _dbus_set_property(conn, BUS_NAME_PULSEAUDIO, OBJECT_PULSEAUDIO, INTERFACE_PULSEAUDIO,
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
		if((ret = _dbus_get_property(conn, BUS_NAME_PULSEAUDIO, OBJECT_PULSEAUDIO, INTERFACE_PULSEAUDIO,
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

int mm_sound_client_dbus_add_test_callback(mm_sound_test_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _sound_server_dbus_signal_subscribe(SOUND_SERVER_SIGNAL_TEST, func, user_data, 0)) != MM_ERROR_NONE) {
		debug_error("add test callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_test_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _sound_server_dbus_signal_unsubscribe(SOUND_SERVER_SIGNAL_TEST)) != MM_ERROR_NONE) {
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
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_TEST, params, &result)) != MM_ERROR_NONE) {
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
	GList* g_device_list = NULL;
	const gchar* device_name_tmp = NULL;

	debug_fenter();

	if (!device_list) {
		debug_error("Invalid Parameter, device_list null");
		ret = MM_ERROR_INVALID_ARGUMENT;
		goto cleanup;
	}

	params = g_variant_new("(i)", device_flags);

	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_GET_CONNECTED_DEVICE_LIST, params, &result)) != MM_ERROR_NONE) {
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
		if (device_item && g_variant_iter_loop(&iter, "(iiii&s)", &device_item->id, &device_item->type, &device_item->io_direction, &device_item->state, &device_name_tmp)) {
			MMSOUND_STRNCPY(device_item->name, device_name_tmp, MAX_DEVICE_NAME_NUM);
			*device_list = g_list_append(*device_list, device_item);
			debug_log("Added device id(%d) type(%d) direction(%d) state(%d) name(%s)", device_item->id, device_item->type,device_item->io_direction, device_item->state, device_item->name);
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

int mm_sound_client_dbus_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _sound_server_dbus_signal_subscribe(SOUND_SERVER_SIGNAL_DEVICE_CONNECTED, func, user_data, device_flags)) != MM_ERROR_NONE) {
		debug_error("add device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_device_connected_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _sound_server_dbus_signal_unsubscribe(SOUND_SERVER_SIGNAL_DEVICE_CONNECTED)) != MM_ERROR_NONE) {
		debug_error("remove device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _sound_server_dbus_signal_subscribe(SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED, func, user_data, device_flags)) != MM_ERROR_NONE) {
		debug_error("Add device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_device_info_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _sound_server_dbus_signal_unsubscribe(SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED)) != MM_ERROR_NONE) {
		debug_error("remove device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_is_bt_a2dp_on (bool *connected, char** bt_name)
{
	int ret = MM_ERROR_NONE;
	GVariant* result = NULL;
	gboolean _connected;
	gchar* _bt_name = NULL;

	debug_fenter();

	if((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_GET_BT_A2DP_STATUS, NULL, &result)) != MM_ERROR_NONE) {
		goto cleanup;
	}
	g_variant_get(result, "(bs)", &_connected, &_bt_name);

	*connected = _connected;
	*bt_name = (_connected)? _bt_name : NULL;

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}


int mm_sound_client_dbus_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _sound_server_dbus_signal_subscribe(SOUND_SERVER_SIGNAL_VOLUME_CHANGED, func, user_data, 0)) != MM_ERROR_NONE) {
		debug_error("Add Volume changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_remove_volume_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = _sound_server_dbus_signal_unsubscribe(SOUND_SERVER_SIGNAL_VOLUME_CHANGED)) != MM_ERROR_NONE) {
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

	if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_GET_AUDIO_PATH, NULL, &result)) != MM_ERROR_NONE) {
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
			   bool enable_session, int *codechandle)
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

	params = g_variant_new("(iiiiiiib)", tone, repeat, volume,
		      volume_config, session_type, session_options, client_pid , _enable_session);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_PLAY_DTMF, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_client_dbus_play_sound(char* filename, int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int keytone,  int handle_route,
			   bool enable_session, int *codechandle)
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

	params = g_variant_new("(siiiiiiiiiib)", filename, tone, repeat, volume,
		      volume_config, priority, session_type, session_options, client_pid, keytone, handle_route, _enable_session);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_PLAY_FILE_START, params, &result)) != MM_ERROR_NONE) {
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

	if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_PLAY_FILE_STOP, g_variant_new("(i)", handle), &result)) != MM_ERROR_NONE) {
		debug_error("dbus stop file playing failed");
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_is_route_available(mm_sound_route route, bool *is_available)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_foreach_available_route_cb(mm_sound_available_route_cb avail_cb, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_set_active_route(mm_sound_route route, bool need_broadcast)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_add_play_sound_end_callback(int handle, mm_sound_stop_callback_func stop_cb, void* userdata)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = _sound_server_dbus_signal_subscribe(SOUND_SERVER_SIGNAL_PLAY_FILE_END, stop_cb, userdata, handle)) != MM_ERROR_NONE) {
		debug_error("add play sound end callback failed");
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_remove_active_device_changed_callback(const char *name)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_remove_available_route_changed_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_set_active_route_auto(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_set_audio_balance(double audio_balance)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();
	debug_msg("audio_balance = %f", audio_balance);

	if ((ret = _pulseaudio_dbus_set_property(PULSEAUDIO_PROP_AUDIO_BALANCE,
											g_variant_new("d", audio_balance), &result)) != MM_ERROR_NONE) {
		debug_error("dbus set audio balance property failed [%d]", ret);
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_get_audio_balance(double *audio_balance)
{
	double balance;
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if (!audio_balance) {
		debug_error("audio_balance is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((ret = _pulseaudio_dbus_get_property(PULSEAUDIO_PROP_AUDIO_BALANCE, &result)) != MM_ERROR_NONE) {
		debug_error("dbus set audio balance property failed [%d]", ret);
		goto cleanup;
	}

	if (result) {
		g_variant_get(result, "(d)", &balance);
		debug_log("Got audio balance : %f", balance);
		*audio_balance = balance;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_enable_mono_audio(bool enable)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();
	debug_msg("enable = %d", enable);

	if ((ret = _pulseaudio_dbus_set_property(PULSEAUDIO_PROP_MONO_AUDIO,
											g_variant_new("b", enable), &result)) != MM_ERROR_NONE) {
		debug_error("dbus set mono audio property failed [%d]", ret);
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_is_mono_audio_enabled(bool *is_enabled)
{
	gboolean enabled;
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if (!is_enabled) {
		debug_error("is_enabled is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((ret = _pulseaudio_dbus_get_property(PULSEAUDIO_PROP_MONO_AUDIO, &result)) != MM_ERROR_NONE) {
		debug_error("dbus get mono audio property failed [%d]", ret);
		goto cleanup;
	}

	if (result) {
		g_variant_get(result, "(b)", &enabled);
		debug_log("Got mono audio enabled : %d", enabled);
		*is_enabled = (bool)enabled;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_enable_mute_all(bool enable)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();
	debug_msg("enable = %d", enable);

	if ((ret = _pulseaudio_dbus_set_property(PULSEAUDIO_PROP_MUTE_ALL,
											g_variant_new("b", enable), &result)) != MM_ERROR_NONE) {
		debug_error("dbus set mute-all property failed [%d]", ret);
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_is_mute_all_enabled(bool *is_enabled)
{
	gboolean enabled;
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if (!is_enabled) {
		debug_error("is_enabled is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((ret = _pulseaudio_dbus_get_property(PULSEAUDIO_PROP_MUTE_ALL, &result)) != MM_ERROR_NONE) {
		debug_error("dbus get mute-all property failed [%d]", ret);
		goto cleanup;
	}

	if (result) {
		g_variant_get(result, "(b)", &enabled);
		debug_log("Got mute-all enabled : %d", enabled);
		*is_enabled = (bool)enabled;
	} else {
		debug_error("reply null");
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

static void _mute_all_signal_changed_callback (GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *params,
                                     gpointer          user_data)
{
	GVariantIter *iter = NULL;
	const gchar *interface_name_for_signal;
	GVariantIter *invalidated_iter;
	const gchar *key;
	GVariant *value;
	gboolean muteall_value;
	struct user_callback *user_cb  = (struct user_callback *)user_data;

	if (!user_cb) {
		debug_error("User callback data Null");
		return;
	}

	if (!params) {
		debug_error("Parameter Null");
		return;
	}

	g_variant_get(params,"(sa{sv}as)", &interface_name_for_signal, &iter, &invalidated_iter);

	debug_log("Signal(%s.%s.%s) Received , Let's call real User-Callback",
			interface_name, signal_name, interface_name_for_signal);

	while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
		if (g_strcmp0(key, "MuteAll") == 0) {
			g_variant_get(value, "b", &muteall_value);
			if (user_cb->sig_type == PULSEAUDIO_PROP_MUTE_ALL && user_cb->cb) {
				((mm_sound_mute_all_changed_cb)(user_cb->cb))(muteall_value, user_cb->userdata);
			}
		}
	}

	g_variant_iter_free (iter);

	/* FIXME : what about invalidated_iter?? */
}

int mm_sound_client_dbus_add_mute_all_changed_callback(mm_sound_mute_all_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;
	guint subs_id = 0;
	struct user_callback *user_cb  = NULL;

	debug_fenter();

	if (g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MUTE_ALL] > 0) {
		debug_error("Already callback exists, can't overwrite before remove callback....");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if (!func) {
		debug_error("Callback data Null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM)) == NULL) {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if (!(user_cb = g_malloc0(sizeof(struct user_callback)))) {
		debug_error("Allocate Memory for User CB data Failed");
		return MM_ERROR_SOUND_INTERNAL;
	}
	memset (user_cb, 0, sizeof(struct user_callback));
	user_cb->sig_type = PULSEAUDIO_PROP_MUTE_ALL;
	user_cb->cb = func;
	user_cb->userdata = user_data;

	if((ret = _dbus_subscribe_signal(conn, OBJECT_PULSEAUDIO, INTERFACE_DBUS, SIGNAL_PROP_CHANGED,
									_mute_all_signal_changed_callback, &subs_id, user_cb)) == MM_ERROR_NONE) {
		g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MUTE_ALL] = subs_id;
	} else {
		debug_error("Dbus Subscribe on Client Error");
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_remove_mute_all_changed_callback(void)
{
	GDBusConnection *conn = NULL;

	debug_fenter();

	if ((conn = _dbus_get_connection(G_BUS_TYPE_SYSTEM)) == NULL) {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}

	_dbus_unsubscribe_signal(conn, g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MUTE_ALL]);
	g_dbus_prop_subs_ids[PULSEAUDIO_PROP_MUTE_ALL] = 0;

	debug_fleave();

	return MM_ERROR_NONE;
}

/*------------------------------------------ FOCUS --------------------------------------------------*/
#ifdef USE_FOCUS

static gpointer _focus_thread_func(gpointer data)
{
	debug_log(">>> thread func..ID of this thread(%u)\n", (unsigned int)pthread_self());
	g_main_loop_run(g_focus_loop);
	debug_log("<<< quit thread func..\n");
	return NULL;
}

static gboolean _focus_fd_check(GSource * source)
{
	GSList *fd_list;
	GPollFD *temp;

	if (!source) {
		debug_error("GSource is null");
		return FALSE;
	}
	fd_list = source->poll_fds;
	if (!fd_list) {
		debug_error("fd_list is null");
		return FALSE;
	}
	do {
		temp = (GPollFD*)fd_list->data;
		if (!temp) {
			debug_error("fd_list->data is null");
			return FALSE;
		}
		if (temp->revents & (POLLIN | POLLPRI)) {
			return TRUE;
		}
		fd_list = fd_list->next;
	} while (fd_list);

	return FALSE; /* there is no change in any fd state */
}

static gboolean _focus_fd_prepare(GSource *source, gint *timeout)
{
	return FALSE;
}

static gboolean _focus_fd_dispatch(GSource *source,	GSourceFunc callback, gpointer user_data)
{
	callback(user_data);
	return TRUE;
}


static int _focus_find_index_by_handle(int handle)
{
	int i = 0;
	for(i = 0; i< FOCUS_HANDLE_MAX; i++) {
		if (handle == g_focus_sound_handle[i].handle) {
			//debug_msg("found index(%d) for handle(%d)", i, handle);
			if (handle == FOCUS_HANDLE_INIT_VAL) {
				return -1;
			}
			return i;
		}
	}
	return -1;
}

static int _focus_find_index_by_watch(int pid)
{
	int i = 0;
	for(i = 0; i< FOCUS_HANDLE_MAX; i++) {
		if (pid == g_focus_sound_handle[i].focus_tid && g_focus_sound_handle[i].watch_callback) {
			//debug_msg("found index(%d) for handle(%d)", i, handle);
			return i;
		}
	}
	debug_msg("can not find watch callback index with given pid[%d]", pid);
	return -1;
}

static bool _focus_callback_handler(gpointer d)
{
	GPollFD *data = (GPollFD*)d;
	int count;
	int tid = 0;
	int focus_index = 0;
	focus_cb_data_lib cb_data;
	debug_log(">>> focus_callback_handler()..ID of this thread(%u)\n", (unsigned int)pthread_self());

	memset(&cb_data, 0, sizeof(focus_cb_data_lib));

	if (!data) {
		debug_error("GPollFd is null");
		return FALSE;
	}
	if (data->revents & (POLLIN | POLLPRI)) {
		int changed_state = -1;

		count = read(data->fd, &cb_data, sizeof(cb_data));
		if (count < 0){
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("GpollFD read fail, errno=%d(%s)",errno, str_error);
			return FALSE;
		}
		changed_state = cb_data.state;
		focus_index = _focus_find_index_by_handle(cb_data.handle);
		if (focus_index == -1) {
			debug_error("Can not find index");
			return FALSE;
		}

		if (g_focus_sound_handle[focus_index].focus_lock) {
			g_mutex_lock(g_focus_sound_handle[focus_index].focus_lock);
		}

		tid = g_focus_sound_handle[focus_index].focus_tid;

		if (changed_state != -1) {
			debug_error("Got and start CB : TID(%d), handle(%d), type(%d), state(%d,(DEACTIVATED(0)/ACTIVATED(1)), trigger(%s)", tid, cb_data.handle, cb_data.type, cb_data.state, cb_data.stream_type);
			if (g_focus_sound_handle[focus_index].focus_callback== NULL) {
					debug_error("callback is null..");
			}
			debug_error("[CALLBACK(%p) START]",g_focus_sound_handle[focus_index].focus_callback);
			(g_focus_sound_handle[focus_index].focus_callback)(cb_data.handle, cb_data.type, cb_data.state, cb_data.stream_type, cb_data.name, g_focus_sound_handle[focus_index].user_data);
			debug_error("[CALLBACK END]");
		}
#ifdef CONFIG_ENABLE_RETCB

				int rett = 0;
				int tmpfd = -1;
				int buf = 0;
				char *filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[focus_index].focus_tid, cb_data.handle);
				tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
				if (tmpfd < 0) {
					char str_error[256];
					strerror_r(errno, str_error, sizeof(str_error));
					debug_error("[RETCB][Failed(May Server Close First)]tid(%d) fd(%d) %s errno=%d(%s)\n", tid, tmpfd, filename2, errno, str_error);
					g_free(filename2);
					if (g_focus_sound_handle[focus_index].focus_lock) {
						g_mutex_unlock(g_focus_sound_handle[focus_index].focus_lock);
					}
					return FALSE;
				}
				buf = cb_data.handle;
				rett = write(tmpfd, &buf, sizeof(buf));
				close(tmpfd);
				g_free(filename2);
				debug_msg("[RETCB] tid(%d) finishing CB (write=%d)\n", tid, rett);
#endif
	}


	if (g_focus_sound_handle[focus_index].focus_lock) {
		g_mutex_unlock(g_focus_sound_handle[focus_index].focus_lock);
	}

	return TRUE;
}

static gboolean _focus_watch_callback_handler( gpointer d)
{
	GPollFD *data = (GPollFD*)d;
	int count;
	int tid = 0;
	int focus_index = 0;
	focus_cb_data_lib cb_data;

	debug_fenter();

	memset(&cb_data, 0, sizeof(focus_cb_data_lib));

	if (!data) {
		debug_error("GPollFd is null");
		return FALSE;
	}
	if (data->revents & (POLLIN | POLLPRI)) {
		count = read(data->fd, &cb_data, sizeof(cb_data));
		if (count < 0){
			char str_error[256];
			strerror_r(errno, str_error, sizeof(str_error));
			debug_error("GpollFD read fail, errno=%d(%s)",errno, str_error);
			return FALSE;
		}

		focus_index = _focus_find_index_by_handle(cb_data.handle);
		if (focus_index == -1) {
			debug_error("Can not find index");
			return FALSE;
		}

		if (g_focus_sound_handle[focus_index].focus_lock) {
			g_mutex_lock(g_focus_sound_handle[focus_index].focus_lock);
		}

		tid = g_focus_sound_handle[focus_index].focus_tid;

		debug_error("Got and start CB : TID(%d), handle(%d), type(%d), state(%d,(DEACTIVATED(0)/ACTIVATED(1)), trigger(%s)", tid, cb_data.handle,  cb_data.type, cb_data.state, cb_data.stream_type);

		if (g_focus_sound_handle[focus_index].watch_callback == NULL) {
			debug_msg("callback is null..");
			if (g_focus_sound_handle[focus_index].focus_lock) {
				g_mutex_unlock(g_focus_sound_handle[focus_index].focus_lock);
			}
			return FALSE;
		}

		debug_msg("[CALLBACK(%p) START]",g_focus_sound_handle[focus_index].watch_callback);
		(g_focus_sound_handle[focus_index].watch_callback)(cb_data.type, cb_data.state, cb_data.stream_type, cb_data.name, g_focus_sound_handle[focus_index].user_data);
		debug_msg("[CALLBACK END]");

#ifdef CONFIG_ENABLE_RETCB

			int rett = 0;
			int tmpfd = -1;
			int buf = -1;
			char *filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[focus_index].focus_tid);
			tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
			if (tmpfd < 0) {
				char str_error[256];
				strerror_r(errno, str_error, sizeof(str_error));
				debug_error("[RETCB][Failed(May Server Close First)]tid(%d) fd(%d) %s errno=%d(%s)\n", tid, tmpfd, filename2, errno, str_error);
				g_free(filename2);
				if (g_focus_sound_handle[focus_index].focus_lock) {
					g_mutex_unlock(g_focus_sound_handle[focus_index].focus_lock);
				}
				return FALSE;
			}
			buf = cb_data.handle;
			rett = write(tmpfd, &buf, sizeof(buf));
			close(tmpfd);
			g_free(filename2);
			debug_msg("[RETCB] tid(%d) finishing CB (fprintf=%d)\n", tid, rett);

#endif

	}
	debug_fleave();

	if (g_focus_sound_handle[focus_index].focus_lock) {
		g_mutex_unlock(g_focus_sound_handle[focus_index].focus_lock);
	}

	return TRUE;
}

static void _focus_open_callback(int index, bool is_for_watching)
{
	mode_t pre_mask;
	char *filename;

	debug_fenter();

	if (is_for_watching) {
		filename = g_strdup_printf("/tmp/FOCUS.%d.wch", g_focus_sound_handle[index].focus_tid);
	} else {
		filename = g_strdup_printf("/tmp/FOCUS.%d.%d", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}
	pre_mask = umask(0);
	if (mknod(filename, S_IFIFO|0666, 0)) {
		debug_error("mknod() failure, errno(%d)", errno);
	}
	umask(pre_mask);
	g_focus_sound_handle[index].focus_fd = open( filename, O_RDWR|O_NONBLOCK);
	if (g_focus_sound_handle[index].focus_fd == -1) {
		debug_error("Open fail : index(%d), file open error(%d)", index, errno);
	} else {
		debug_log("Open sucess : index(%d), filename(%s), fd(%d)", index, filename, g_focus_sound_handle[index].focus_fd);
	}
	g_free(filename);
	filename = NULL;

#ifdef CONFIG_ENABLE_RETCB
	char *filename2;

	if (is_for_watching) {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[index].focus_tid);
	} else {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}
	pre_mask = umask(0);
	if (mknod(filename2, S_IFIFO | 0666, 0)) {
		debug_error("mknod() failure, errno(%d)", errno);
	}
	umask(pre_mask);
	g_free(filename2);
	filename2 = NULL;
#endif
	debug_fleave();

}

void _focus_close_callback(int index, bool is_for_watching)
{
	debug_fenter();

	if (g_focus_sound_handle[index].focus_fd < 0) {
		debug_error("%Close fail : fd error.");
	} else {
		char *filename;
		if (is_for_watching) {
			filename = g_strdup_printf("/tmp/FOCUS.%d.wch", g_focus_sound_handle[index].focus_tid);
		} else {
			filename = g_strdup_printf("/tmp/FOCUS.%d.%d", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
		}
		close(g_focus_sound_handle[index].focus_fd);
		if (remove(filename)) {
			debug_error("remove() failure, filename(%s), errno(%d)", filename, errno);
		}
		debug_log("%Close Sucess : index(%d), filename(%s)", index, filename);
		g_free(filename);
		filename = NULL;
	}

#ifdef CONFIG_ENABLE_RETCB
	char *filename2;

	if (is_for_watching) {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.wchr", g_focus_sound_handle[index].focus_tid);
	} else {
		filename2 = g_strdup_printf("/tmp/FOCUS.%d.%dr", g_focus_sound_handle[index].focus_tid, g_focus_sound_handle[index].handle);
	}

	/* Defensive code - wait until callback timeout although callback is removed */
	int buf = MM_ERROR_NONE; //no need to specify cb result to server, just notice if the client got the callback properly or not
	int tmpfd = -1;

	tmpfd = open(filename2, O_WRONLY | O_NONBLOCK);
	if (tmpfd < 0) {
		char str_error[256];
		strerror_r(errno, str_error, sizeof(str_error));
		debug_warning("could not open file(%s) (may server close it first), tid(%d) fd(%d) %s errno=%d(%s)",
			filename2, g_focus_sound_handle[index].focus_tid, tmpfd, filename2, errno, str_error);
	} else {
		debug_msg("write MM_ERROR_NONE(tid:%d) for waiting server", g_focus_sound_handle[index].focus_tid);
		write(tmpfd, &buf, sizeof(buf));
		close(tmpfd);
	}

	if (remove(filename2)) {
		debug_error("remove() failure, filename(%s), errno(%d)", filename2, errno);
	}
	g_free(filename2);
	filename2 = NULL;
#endif

}

static bool _focus_add_sound_callback(int index, int fd, gushort events, focus_gLoopPollHandler_t p_gloop_poll_handler )
{
	GSource* g_src = NULL;
	GSourceFuncs *g_src_funcs = NULL;		/* handler function */
	guint gsource_handle;
	GPollFD *g_poll_fd = NULL;			/* file descriptor */

	debug_fenter();

	g_focus_sound_handle[index].focus_lock = g_mutex_new();
	if (!g_focus_sound_handle[index].focus_lock) {
		debug_error("failed to g_mutex_new() for index(%d)", index);
		return false;
	}

	/* 1. make GSource Object */
	g_src_funcs = (GSourceFuncs *)g_malloc(sizeof(GSourceFuncs));
	if (!g_src_funcs) {
		debug_error("g_malloc failed on g_src_funcs");
		return false;
	}

	g_src_funcs->prepare = _focus_fd_prepare;
	g_src_funcs->check = _focus_fd_check;
	g_src_funcs->dispatch = _focus_fd_dispatch;
	g_src_funcs->finalize = NULL;
	g_src = g_source_new(g_src_funcs, sizeof(GSource));
	if (!g_src) {
		debug_error("g_malloc failed on m_readfd");
		return false;
	}
	g_focus_sound_handle[index].focus_src = g_src;
	g_focus_sound_handle[index].g_src_funcs = g_src_funcs;

	/* 2. add file description which used in g_loop() */
	g_poll_fd = (GPollFD *)g_malloc(sizeof(GPollFD));
	if (!g_poll_fd) {
		debug_error("g_malloc failed on g_poll_fd");
		return false;
	}
	g_poll_fd->fd = fd;
	g_poll_fd->events = events;
	g_focus_sound_handle[index].g_poll_fd = g_poll_fd;

	/* 3. combine g_source object and file descriptor */
	g_source_add_poll(g_src, g_poll_fd);
	gsource_handle = g_source_attach(g_src, g_main_loop_get_context(g_focus_loop));
	if (!gsource_handle) {
		debug_error(" Failed to attach the source to context");
		return false;
	}
	//g_source_unref(g_src);

	/* 4. set callback */
	g_source_set_callback(g_src, p_gloop_poll_handler,(gpointer)g_poll_fd, NULL);

	debug_log(" g_malloc:g_src_funcs(%#X),g_poll_fd(%#X)  g_source_add_poll:g_src_id(%d)  g_source_set_callback:errno(%d)",
				g_src_funcs, g_poll_fd, gsource_handle, errno);

	debug_fleave();
	return true;


}

static bool _focus_remove_sound_callback(int index, gushort events)
{
	bool ret = true;
	gboolean gret = TRUE;

	debug_fenter();

	if (g_focus_sound_handle[index].focus_lock) {
		g_mutex_free(g_focus_sound_handle[index].focus_lock);
		g_focus_sound_handle[index].focus_lock = NULL;
	}

	GSourceFunc *g_src_funcs = g_focus_sound_handle[index].g_src_funcs;
	GPollFD *g_poll_fd = g_focus_sound_handle[index].g_poll_fd;	/* store file descriptor */
	if (!g_poll_fd) {
		debug_error("g_poll_fd is null..");
		ret = false;
		goto init_handle;
	}
	g_poll_fd->fd = g_focus_sound_handle[index].focus_fd;
	g_poll_fd->events = events;

	if (!g_focus_sound_handle[index].focus_src) {
		debug_error("FOCUS_sound_handle[%d].focus_src is null..", index);
		goto init_handle;
	}
	debug_log(" g_source_remove_poll : fd(%d), event(%x), errno(%d)", g_poll_fd->fd, g_poll_fd->events, errno);
	g_source_remove_poll(g_focus_sound_handle[index].focus_src, g_poll_fd);

init_handle:

	if (g_focus_sound_handle[index].focus_src) {
		g_source_destroy(g_focus_sound_handle[index].focus_src);
		if (!g_source_is_destroyed (g_focus_sound_handle[index].focus_src)) {
			debug_warning(" failed to g_source_destroy(), asm_src(0x%p)", g_focus_sound_handle[index].focus_src);
		}
	}
	debug_log(" g_free : g_src_funcs(%#X), g_poll_fd(%#X)", g_src_funcs, g_poll_fd);

	if (g_src_funcs) {
		g_free(g_src_funcs);
		g_src_funcs = NULL;
	}
	if (g_poll_fd) {
		g_free(g_poll_fd);
		g_poll_fd = NULL;
	}

	g_focus_sound_handle[index].g_src_funcs = NULL;
	g_focus_sound_handle[index].g_poll_fd = NULL;
	g_focus_sound_handle[index].focus_src = NULL;
	g_focus_sound_handle[index].focus_callback = NULL;
	g_focus_sound_handle[index].watch_callback = NULL;

	debug_fleave();
	return ret;
}


static void _focus_add_callback(int index, bool is_for_watching)
{
	debug_fenter();
	if (!is_for_watching) {
		if (!_focus_add_sound_callback(index, g_focus_sound_handle[index].focus_fd, (gushort)POLLIN | POLLPRI, _focus_callback_handler)) {
			debug_error("failed to __ASM_add_sound_callback(asm_callback_handler)");
			//return false;
		}
	} else { // need to check if it's necessary
		if (!_focus_add_sound_callback(index, g_focus_sound_handle[index].focus_fd, (gushort)POLLIN | POLLPRI, _focus_watch_callback_handler)) {
			debug_error("failed to __ASM_add_sound_callback(watch_callback_handler)");
			//return false;
		}
	}
	debug_fleave();
}

static void _focus_remove_callback(int index)
{
	debug_fenter();
	if (!_focus_remove_sound_callback(index, (gushort)POLLIN | POLLPRI)) {
		debug_error("failed to __focus_remove_sound_callback()");
		//return false;
	}
	debug_fleave();
}

static void _focus_init_callback(int index, bool is_for_watching)
{
	debug_fenter();
	_focus_open_callback(index, is_for_watching);
	_focus_add_callback(index, is_for_watching);
	debug_fleave();
}

static void _focus_destroy_callback(int index, bool is_for_watching)
{
	debug_fenter();
	_focus_remove_callback(index);
	_focus_close_callback(index, is_for_watching);
	debug_fleave();
}

int mm_sound_client_dbus_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = 0;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

//	pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();

	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == false) {
			g_focus_sound_handle[index].is_used = true;
			break;
		}
	}

	g_focus_sound_handle[index].focus_tid = instance;
	g_focus_sound_handle[index].handle = id;
	g_focus_sound_handle[index].focus_callback = callback;
	g_focus_sound_handle[index].user_data = user_data;

	params = g_variant_new("(iis)", instance, id, stream_type);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_REGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus register focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to register focus\n");
		if (!g_focus_thread) {
			GMainContext* focus_context = g_main_context_new ();
			g_focus_loop = g_main_loop_new (focus_context, FALSE);
			g_main_context_unref(focus_context);
			g_focus_thread = g_thread_create(_focus_thread_func, NULL, TRUE, NULL);
			if (g_focus_thread == NULL) {
				debug_error ("could not create thread..");
				g_main_loop_unref(g_focus_loop);
				g_focus_sound_handle[index].is_used = false;
				ret = MM_ERROR_SOUND_INTERNAL;
				goto cleanup;
			}
		}
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d \n",ret);
		g_focus_sound_handle[index].is_used = false;
		goto cleanup;
	}

	_focus_init_callback(index, false);

cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);
	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();

	return ret;

}

int mm_sound_client_dbus_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = -1;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();
	index = _focus_find_index_by_handle(id);

	if (g_focus_sound_handle[index].focus_lock) {
		if (!g_mutex_trylock(g_focus_sound_handle[index].focus_lock)) {
			debug_warning("maybe focus_callback is being called, try one more time..");
			usleep(2500000); // 2.5 sec
			if (g_mutex_trylock(g_focus_sound_handle[index].focus_lock)) {
				debug_msg("finally got asm_lock");
			}
		}
	}

	params = g_variant_new("(ii)", instance, id);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_UNREGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus unregister focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to unregister focus\n");
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d \n",ret);
		goto cleanup;
	}


cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);
	if (g_focus_sound_handle[index].focus_lock) {
		g_mutex_unlock(g_focus_sound_handle[index].focus_lock);
	}

	_focus_destroy_callback(index, false);
	g_focus_sound_handle[index].focus_fd = 0;
	g_focus_sound_handle[index].focus_tid = 0;
	g_focus_sound_handle[index].handle = 0;
	g_focus_sound_handle[index].is_used = false;

	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_acquire_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	int instance;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();

	params = g_variant_new("(iiis)", instance, id, type, option);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_ACQUIRE_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus acquire focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to acquire focus\n");
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d \n",ret);
	}

cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);

	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_release_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;
	int instance;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();;

	params = g_variant_new("(iiis)", instance, id, type, option);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_RELEASE_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus release focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to release focus\n");
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d \n",ret);
	}

cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);

	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_set_focus_watch_callback(mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = 0;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();

	for (index = 0; index < FOCUS_HANDLE_MAX; index++) {
		if (g_focus_sound_handle[index].is_used == false) {
			g_focus_sound_handle[index].is_used = true;
			break;
		}
	}

	g_focus_sound_handle[index].focus_tid = instance;
	g_focus_sound_handle[index].handle = index;
	g_focus_sound_handle[index].watch_callback = callback;
	g_focus_sound_handle[index].user_data = user_data;

	params = g_variant_new("(ii)", instance, type);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_WATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus set watch focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to watch focus");
		if (!g_focus_thread) {
			GMainContext* focus_context = g_main_context_new ();
			g_focus_loop = g_main_loop_new (focus_context, FALSE);
			g_main_context_unref(focus_context);
			g_focus_thread = g_thread_create(_focus_thread_func, NULL, TRUE, NULL);
			if (g_focus_thread == NULL) {
				debug_error ("could not create thread..");
				g_main_loop_unref(g_focus_loop);
				g_focus_sound_handle[index].is_used = false;
				ret = MM_ERROR_SOUND_INTERNAL;
				goto cleanup;
			}
		}
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d",ret);
		goto cleanup;
	}

	_focus_init_callback(index, true);

cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);

	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();

	return ret;
}

int mm_sound_client_dbus_unset_focus_watch_callback(void)
{
	int ret = MM_ERROR_NONE;
	int instance;
	int index = -1;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	//pthread_mutex_lock(&g_thread_mutex2);

	instance = getpid();
	index = _focus_find_index_by_watch(instance);

	if (g_focus_sound_handle[index].focus_lock) {
		g_mutex_lock(g_focus_sound_handle[index].focus_lock);
	}

	params = g_variant_new("(i)", instance);
	if (params) {
		if ((ret = _sound_server_dbus_method_call(SOUND_SERVER_METHOD_UNWATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus unset watch focus failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for method call failed");
	}

	if (ret == MM_ERROR_NONE) {
		debug_msg("[Client] Success to unwatch focus\n");
	} else {
		g_variant_get(result, "(i)",  &ret);
		debug_error("[Client] Error occurred : %d \n",ret);
	}

cleanup:
	//pthread_mutex_unlock(&g_thread_mutex2);

	if (g_focus_sound_handle[index].focus_lock) {
		g_mutex_unlock(g_focus_sound_handle[index].focus_lock);
	}
	_focus_destroy_callback(index, true);
	g_focus_sound_handle[index].focus_fd = 0;
	g_focus_sound_handle[index].focus_tid = 0;
	g_focus_sound_handle[index].handle = 0;
	g_focus_sound_handle[index].is_used = false;

	if (result) {
		g_variant_unref(result);
	}

	debug_fleave();

	return ret;
}

#endif /* USE_FOCUS */
/*------------------------------------------ FOCUS --------------------------------------------------*/

int mm_sound_client_dbus_initialize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	g_mm_sound_error_quark = _dbus_register_error_domain();
	debug_fleave();
	return ret;
}

int mm_sound_client_dbus_finalize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if (g_focus_thread) {
		g_main_loop_quit(g_focus_loop);
		g_thread_join(g_focus_thread);
		debug_log("after thread join");
		g_main_loop_unref(g_focus_loop);
		g_focus_thread = NULL;
	}

	debug_fleave();

	return ret;
}
