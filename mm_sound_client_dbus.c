#include <gio/gio.h>
#include <glib.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "include/mm_sound_client_dbus.h"
#include "include/mm_sound_device.h"
#include "include/mm_sound_msg.h"
#include "include/mm_sound_common.h"


#define BUS_NAME_SOUND_SERVER "org.tizen.SoundServer"
#define OBJECT_SOUND_SERVER "/org/tizen/SoundServer1"
#define INTERFACE_SOUND_SERVER "org.tizen.SoundServer1"

#define INTERFACE_DBUS			"org.freedesktop.DBus.Properties"
#define METHOD_GET			"Get"
#define DBUS_NAME_MAX                   32
#define DBUS_SIGNATURE_MAX              32

struct user_callback {
	int sig_type;
	void *cb;
	void *userdata;
	int mask;
};

guint dbus_subs_ids[SOUND_SERVER_SIGNAL_MAX];
GQuark mm_sound_error_quark;

struct mm_sound_dbus_method_info sound_server_methods[SOUND_SERVER_METHOD_MAX] = {
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

struct mm_sound_dbus_signal_info sound_server_signals[SOUND_SERVER_SIGNAL_MAX] = {
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
	{MM_ERROR_SOUND_UNSUPPORTED_FORMAT, "org.tizen.multimedia.audio.UnsupportedFormat"},
	{MM_ERROR_SOUND_INVALID_POINTER, "org.tizen.multimedia.audio.InvalidPointer"},
	{MM_ERROR_SOUND_INVALID_FILE, "org.tizen.multimedia.audio.InvalidFile"},
	{MM_ERROR_SOUND_FILE_NOT_FOUND, "org.tizen.multimedia.audio.FileNotFound"},
	{MM_ERROR_SOUND_NO_DATA, "org.tizen.multimedia.audio.NoData"},
	{MM_ERROR_SOUND_INVALID_PATH, "org.tizen.multimedia.audio.InvalidPath"},
};


/******************************************************************************************
		Wrapper Functions of GDbus
******************************************************************************************/


static GQuark __dbus_register_error_domain (void)
{
	static volatile gsize quark_volatile = 0;
	g_dbus_error_register_error_domain("mm-sound-error-quark",
					   &quark_volatile,
					   mm_sound_error_entries,
					   G_N_ELEMENTS(mm_sound_error_entries));
	return (GQuark) quark_volatile;
}

static int __dbus_convert_error(GError *err)
{
	if (g_dbus_error_is_remote_error(err)) {
		if (err->domain == mm_sound_error_quark) {
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


static int __dbus_method_call(GDBusConnection* conn, const char* bus_name, const char* object, const char* intf, const char* method, GVariant* args, GVariant** result)
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
		ret = __dbus_convert_error(err);
		g_error_free(err);
	}

	return ret;
}


static int __dbus_get_property(GDBusConnection *conn, const char* bus_name, const char* object_name, const char* intf_name, const char* prop, GVariant** result)
{
	int ret = MM_ERROR_NONE;
	if (!conn || !object_name || !intf_name || !prop) {
		debug_error("Invalid Argument");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_log("Dbus get property with obj'%s' intf'%s' prop'%s'", object_name, intf_name, prop);

	if ((ret = __dbus_method_call(conn,
				       bus_name, object_name, INTERFACE_DBUS, METHOD_GET,
				       g_variant_new("(ss)", intf_name, prop), result)) != MM_ERROR_NONE) {
		debug_error("Dbus call for get property failed");
	}

	return ret;
}

static int __dbus_subscribe_signal(GDBusConnection *conn, const char* object_name, const char* intf_name, const char* signal_name,GDBusSignalCallback signal_cb, guint *subscribe_id, void* userdata)
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

static void __dbus_unsubscribe_signal(GDBusConnection *conn, guint subs_id)
{
	if (!conn || !subs_id) {
		debug_error("Invalid Argument");
	}

	g_dbus_connection_signal_unsubscribe(conn, subs_id);
}

static GDBusConnection* __dbus_get_connection(GBusType bustype)
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

static void __dbus_disconnect(GDBusConnection* conn)
{
	debug_fenter ();
	g_object_unref(conn);
	debug_fleave ();
}

/******************************************************************************************
		Simple Functions For Communicate with Sound-Server
******************************************************************************************/

/* Hardcoded method call Just for communicating with Sound-server*/
static int __MMDbusCall(int method_type, GVariant* args, GVariant **result)
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

	if ((conn = __dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if((ret = __dbus_method_call(conn, BUS_NAME_SOUND_SERVER,
					      OBJECT_SOUND_SERVER,
					      INTERFACE_SOUND_SERVER,
					      sound_server_methods[method_type].name,
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

static void __MMDbusSignalCB (GDBusConnection  *connection,
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
		debug_log("Call usercallback for %s", sound_server_signals[user_cb->sig_type].name);
		((mm_sound_volume_changed_cb)(user_cb->cb))(vol_type, volume, user_cb->userdata);
	} else if (user_cb->sig_type == SOUND_SERVER_SIGNAL_DEVICE_CONNECTED) {
		mm_sound_device_t device_h;
		const char* name = NULL;
		bool is_connected = FALSE;

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

static int __MMDbusSubscribe(int signaltype, void *cb, void *userdata, int mask)
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

	if ((conn = __dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		if(__dbus_subscribe_signal(conn, OBJECT_SOUND_SERVER, INTERFACE_SOUND_SERVER, sound_server_signals[signaltype].name,
								__MMDbusSignalCB, &subs_id, user_cb) != MM_ERROR_NONE){
			debug_error("Dbus Subscribe on Client Error");
			return MM_ERROR_SOUND_INTERNAL;
		} else {
			dbus_subs_ids[signaltype] = subs_id;
		}
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return MM_ERROR_NONE;
}

static int __MMDbusUnsubscribe(int signaltype)
{
	GDBusConnection *conn = NULL;

	if (signaltype < 0 || signaltype >= SOUND_SERVER_SIGNAL_MAX) {
		debug_error("Wrong Signal Type");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if ((conn = __dbus_get_connection(G_BUS_TYPE_SYSTEM))) {
		__dbus_unsubscribe_signal(conn, dbus_subs_ids[signaltype]);
	} else {
		debug_error("Get Dbus Connection Error");
		return MM_ERROR_SOUND_INTERNAL;
	}
}


/******************************************************************************************
		Implementation of each dbus client code (Construct Params,..)
******************************************************************************************/

int _mm_sound_client_dbus_add_test_callback(mm_sound_test_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = __MMDbusSubscribe(SOUND_SERVER_SIGNAL_TEST, func, user_data, 0)) != MM_ERROR_NONE) {
		debug_error("add test callback failed");
	}

	debug_fleave();
        return ret;
}

int _mm_sound_client_dbus_remove_test_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = __MMDbusUnsubscribe(SOUND_SERVER_SIGNAL_TEST)) != MM_ERROR_NONE) {
		debug_error("remove test callback failed");
	}

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_test(int a, int b, int *get)
{
	int ret = MM_ERROR_NONE;
	int reply = 0;
	GVariant *params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(ii)", a, b);
	if (params) {
		if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_TEST, params, &result)) != MM_ERROR_NONE) {
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


//int _mm_sound_client_dbus_get_current_connected_device_list(int device_flags, mm_sound_device_list_t *device_list)
int _mm_sound_client_dbus_get_current_connected_device_list(int device_flags, GList** device_list)
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
		if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_GET_CONNECTED_DEVICE_LIST, params, &result)) != MM_ERROR_NONE) {
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

int _mm_sound_client_dbus_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data)
{

	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = __MMDbusSubscribe(SOUND_SERVER_SIGNAL_DEVICE_CONNECTED, func, user_data, device_flags)) != MM_ERROR_NONE) {
		debug_error("add device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_remove_device_connected_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = __MMDbusUnsubscribe(SOUND_SERVER_SIGNAL_DEVICE_CONNECTED)) != MM_ERROR_NONE) {
		debug_error("remove device connected callback failed");
	}

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = __MMDbusSubscribe(SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED, func, user_data, device_flags)) != MM_ERROR_NONE) {
		debug_error("Add device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_remove_device_info_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = __MMDbusUnsubscribe(SOUND_SERVER_SIGNAL_DEVICE_INFO_CHANGED)) != MM_ERROR_NONE) {
		debug_error("remove device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int MMSoundClientDbusIsBtA2dpOn (bool *connected, char** bt_name)
{
	int ret = MM_ERROR_NONE;
	GVariant* result = NULL;
	bool _connected;
	char* _bt_name = NULL;

	debug_fenter();

	if((ret = __MMDbusCall(SOUND_SERVER_METHOD_GET_BT_A2DP_STATUS, NULL, &result)) != MM_ERROR_NONE) {
		goto cleanup;
	}
	g_variant_get(result, "(bs)", &_connected, &_bt_name);

	*connected = _connected;
	if (_connected)
		bt_name = _bt_name;
	else
		bt_name = NULL;

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}


int _mm_sound_client_dbus_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = __MMDbusSubscribe(SOUND_SERVER_SIGNAL_VOLUME_CHANGED, func, user_data, 0)) != MM_ERROR_NONE) {
		debug_error("Add Volume changed callback failed");
	}

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_remove_volume_changed_callback(void)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = __MMDbusUnsubscribe(SOUND_SERVER_SIGNAL_VOLUME_CHANGED)) != MM_ERROR_NONE) {
		debug_error("Remove Volume changed callback failed");
	}

	debug_fleave();
	return ret;
}


int _mm_sound_client_dbus_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_GET_AUDIO_PATH, NULL, &result)) != MM_ERROR_NONE) {
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


//int MMSoundClientDbusPlayTone(int number, int volume_config, double volume, int time, int *handle, bool enable_session)
int MMSoundClientDbusPlayTone(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   bool enable_session, int *codechandle)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;

	if (!codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fenter();

	params = g_variant_new("(iiiiiiib)", tone, repeat, volume,
		      volume_config, session_type, session_options, client_pid , enable_session);
	if (params) {
		if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_PLAY_DTMF, params, &result)) != MM_ERROR_NONE) {
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

//int MMSoundClientDbusPlaySound(MMSoundPlayParam *param, int tone, int keytone, int *handle)
int MMSoundClientDbusPlaySound(char* filename, int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int keytone,  int handle_route,
			   bool enable_session, int *codechandle)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant* params = NULL, *result = NULL;

	if (!filename || !codechandle) {
		debug_error("Param for play is null");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	debug_fenter();

	params = g_variant_new("(siiiiiiiiiib)", filename, tone, repeat, volume,
		      volume_config, priority, session_type, session_options, client_pid, keytone, handle_route, enable_session);
	if (params) {
		if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_PLAY_FILE_START, params, &result)) != MM_ERROR_NONE) {
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

int MMSoundClientDbusStopSound(int handle)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = __MMDbusCall(SOUND_SERVER_METHOD_PLAY_FILE_STOP, g_variant_new("(i)", handle), &result)) != MM_ERROR_NONE) {
		debug_error("dbus stop file playing failed");
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int _mm_sound_client_dbus_is_route_available(mm_sound_route route, bool *is_available)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_foreach_available_route_cb(mm_sound_available_route_cb avail_cb, void *user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_set_active_route(mm_sound_route route, bool need_broadcast)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_add_play_sound_end_callback(int handle, mm_sound_stop_callback_func stop_cb, void* userdata)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();

	if ((ret = __MMDbusSubscribe(SOUND_SERVER_SIGNAL_PLAY_FILE_END, stop_cb, userdata, handle)) != MM_ERROR_NONE) {
		debug_error("add play sound end callback failed");
	}

	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_remove_active_device_changed_callback(const char *name)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_remove_available_route_changed_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

#ifdef USE_FOCUS
int _mm_sound_client_dbus_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_unregister_focus(int id)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_acquire_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_release_focus(int id, mm_sound_focus_type_e type, const char *option)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_set_focus_watch_callback(mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, void* user_data)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

int _mm_sound_client_dbus_unset_focus_watch_callback(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}

#endif

int _mm_sound_client_dbus_set_active_route_auto(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	ret = MM_ERROR_SOUND_NOT_SUPPORTED_OPERATION;
	debug_fleave();

	return ret;
}


int MMSoundClientDbusInit(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	mm_sound_error_quark = __dbus_register_error_domain();
	debug_fleave();
	return ret;
}

int MMSoundClientDbusFini(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_fleave();

	return ret;
}
