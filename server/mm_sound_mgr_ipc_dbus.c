
#include <string.h>

#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>

#ifdef USE_SECURITY
#include <security-server.h>
#define COOKIE_SIZE 20
#endif

#include "include/mm_sound_mgr_ipc_dbus.h"
#include "include/mm_sound_mgr_ipc.h"
#include "../include/mm_sound_dbus.h"


#define BUS_NAME_SOUND_SERVER "org.tizen.SoundServer"
#define OBJECT_SOUND_SERVER "/org/tizen/SoundServer1"
#define INTERFACE_SOUND_SERVER "org.tizen.SoundServer1"

/* workaround for AF volume gain tuning */
#define PROC_DBUS_OBJECT 	"/Org/Tizen/ResourceD/Process"
#define PROC_DBUS_INTERFACE 	"org.tizen.resourced.process"
#define PROC_DBUS_METHOD 	"ProcExclude"

/* Introspection data for the service we are exporting */
  static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.tizen.SoundServer1'>"
  "    <method name='MethodTest1'>"
  "      <arg type='i' name='num1' direction='in'/>"
  "      <arg type='i' name='num2' direction='in'/>"
  "      <arg type='i' name='multiple' direction='out'/>"
  "    </method>"
  "    <method name='GetBTA2DPStatus'>"
  "      <arg type='b' name='is_bt_on' direction='out'/>"
  "      <arg type='s' name='bt_name' direction='out'/>"
  "    </method>"
  "    <method name='PlayFileStart'>"
  "      <arg type='s' name='filename' direction='in'/>"
  "      <arg type='i' name='tone' direction='in'/>"
  "      <arg type='i' name='repeat' direction='in'/>"
  "      <arg type='i' name='volume' direction='in'/>"
  "      <arg type='i' name='vol_config' direction='in'/>"
  "      <arg type='i' name='priority' direction='in'/>"
  "      <arg type='i' name='session_type' direction='in'/>"
  "      <arg type='i' name='session_option' direction='in'/>"
  "      <arg type='i' name='client_pid' direction='in'/>"
  "      <arg type='i' name='handle_route' direction='in'/>"
  "      <arg type='b' name='enable_session' direction='in'/>"
  "      <arg type='s' name='stream_type' direction='in'/>"
  "      <arg type='i' name='stream_index' direction='in'/>"
  "      <arg type='i' name='handle' direction='out'/>"
  "    </method>"
  "	   <method name='PlayFileStartWithStreamInfo'>"
  "	     <arg type='s' name='filename' direction='in'/>"
  "	     <arg type='i' name='repeat' direction='in'/>"
  "	     <arg type='i' name='volume' direction='in'/>"
  "	     <arg type='i' name='priority' direction='in'/>"
  "	     <arg type='i' name='client_pid' direction='in'/>"
  "	     <arg type='i' name='handle_route' direction='in'/>"
  "	     <arg type='s' name='stream_type' direction='in'/>"
  "	     <arg type='i' name='stream_index' direction='in'/>"
  "	     <arg type='i' name='handle' direction='out'/>"
  "	   </method>"
  "    <method name='PlayFileStop'>"
  "      <arg type='i' name='handle' direction='in'/>"
  "    </method>"
  "    <method name='ClearFocus'>"
  "      <arg type='i' name='pid' direction='in'/>"
  "    </method>"
  "    <method name='PlayDTMF'>"
  "      <arg type='i' name='tone' direction='in'/>"
  "      <arg type='i' name='repeat' direction='in'/>"
  "      <arg type='i' name='volume' direction='in'/>"
  "      <arg type='i' name='vol_config' direction='in'/>"
  "      <arg type='i' name='session_type' direction='in'/>"
  "      <arg type='i' name='session_option' direction='in'/>"
  "      <arg type='i' name='client_pid' direction='in'/>"
  "      <arg type='b' name='enable_session' direction='in'/>"
  "	     <arg type='s' name='stream_type' direction='in'/>"
  "	     <arg type='i' name='stream_index' direction='in'/>"
  "      <arg type='i' name='handle' direction='out'/>"
  "    </method>"
  "	   <method name='PlayDTMFWithStreamInfo'>"
  "	     <arg type='i' name='tone' direction='in'/>"
  "	     <arg type='i' name='repeat' direction='in'/>"
  "	     <arg type='i' name='volume' direction='in'/>"
  "	     <arg type='i' name='client_pid' direction='in'/>"
  "	     <arg type='s' name='stream_type' direction='in'/>"
  "	     <arg type='i' name='stream_index' direction='in'/>"
  "	     <arg type='i' name='handle' direction='out'/>"
  "	   </method>"
  "    <method name='GetConnectedDeviceList'>"
  "      <arg type='i' name='device_mask' direction='in'/>"
  "      <arg type='a(iiiis)' name='device_list' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";
GDBusConnection* conn_g;

static void handle_method_play_file_start(GDBusMethodInvocation* invocation);
static void handle_method_play_file_start_with_stream_info(GDBusMethodInvocation* invocation);
static void handle_method_play_file_stop(GDBusMethodInvocation* invocation);
static void handle_method_play_dtmf(GDBusMethodInvocation* invocation);
static void handle_method_play_dtmf_with_stream_info(GDBusMethodInvocation* invocation);
static void handle_method_clear_focus(GDBusMethodInvocation* invocation);
static void handle_method_test(GDBusMethodInvocation* invocation);
static void handle_method_get_connected_device_list(GDBusMethodInvocation* invocation);

/* Currently , Just using method's name and handler */
/* TODO : generate introspection xml automatically, with these value include argument and reply */
/* TODO : argument check with these information */
/* TODO : divide object and interface with features (ex. play, path, device) */
mm_sound_dbus_method_intf_t methods[AUDIO_METHOD_MAX] = {
	[AUDIO_METHOD_TEST] = {
		.info = {
			.name = "MethodTest1",
		},
		.handler = handle_method_test
	},
	[AUDIO_METHOD_PLAY_FILE_START] = {
		.info = {
			.name = "PlayFileStart",
		},
		.handler = handle_method_play_file_start
	},
	[AUDIO_METHOD_PLAY_FILE_START_WITH_STREAM_INFO] = {
		.info = {
			.name = "PlayFileStartWithStreamInfo",
		},
		.handler = handle_method_play_file_start_with_stream_info
	},
	[AUDIO_METHOD_PLAY_FILE_STOP] = {
		.info = {
			.name = "PlayFileStop",
		},
		.handler = handle_method_play_file_stop
	},
	[AUDIO_METHOD_CLEAR_FOCUS] = {
		.info = {
			.name = "ClearFocus",
		},
		.handler = handle_method_clear_focus
	},
	[AUDIO_METHOD_PLAY_DTMF] = {
		.info = {
			.name = "PlayDTMF",
		},
		.handler = handle_method_play_dtmf
	},
	[AUDIO_METHOD_PLAY_DTMF_WITH_STREAM_INFO] = {
		.info = {
			.name = "PlayDTMFWithStreamInfo",
		},
		.handler = handle_method_play_dtmf_with_stream_info
	},
	[AUDIO_METHOD_GET_CONNECTED_DEVICE_LIST] = {
		.info = {
			.name = "GetConnectedDeviceList",
		},
		.handler = handle_method_get_connected_device_list
	},
};

static GDBusNodeInfo *introspection_data = NULL;
guint sound_server_owner_id ;

/*
        For pass error code with 'g_dbus_method_invocation_return_error'
        We have to use some glib features like GError, GQuark
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

static const char* _convert_error_code(int err_code)
{
	int i = 0;

	for (i = 0; i < G_N_ELEMENTS(mm_sound_error_entries); i++) {
		if (err_code == mm_sound_error_entries[i].error_code) {
			return mm_sound_error_entries[i].dbus_error_name;
		}
	}

	return "org.tizen.multimedia.common.Unknown";
}

static int mm_sound_mgr_ipc_dbus_send_signal(audio_event_t event, GVariant *parameter)
{
	if(mm_sound_dbus_emit_signal(AUDIO_PROVIDER_SOUND_SERVER, event, parameter) != MM_ERROR_NONE) {
		debug_error("Sound Server Emit signal failed");
		return MM_ERROR_SOUND_INTERNAL;
	}
	return MM_ERROR_NONE;
}

static int _get_sender_pid(GDBusMethodInvocation* invocation)
{
	GVariant* value;
	guint pid = 0;
	const gchar* sender;
	GDBusConnection * connection = NULL;
	GError* err = NULL;

	connection = g_dbus_method_invocation_get_connection(invocation);
	sender = g_dbus_method_invocation_get_sender(invocation);

	debug_error ("connection = %p, sender = %s", connection, sender);

	value = g_dbus_connection_call_sync (connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
										"org.freedesktop.DBus", "GetConnectionUnixProcessID",
										g_variant_new("(s)", sender, NULL), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
	if (value) {
		g_variant_get(value, "(u)", &pid);
		debug_error ("Sender PID = [%d]", pid);
	} else {
		debug_error ("err code = %d, err msg = %s", err->code, err->message);
	}
	return pid;
}

static void _method_call_return_value(GDBusMethodInvocation *invocation, GVariant *params)
{
	const char *method_name;
	method_name = g_dbus_method_invocation_get_method_name(invocation);
	debug_error("Method Call '%s' success", method_name);
	g_dbus_method_invocation_return_value(invocation, params);
}
static void _method_call_return_error(GDBusMethodInvocation *invocation, int ret)
{
	const char *err_name, *method_name;
	err_name = _convert_error_code(ret);
	method_name = g_dbus_method_invocation_get_method_name(invocation);
	debug_error("Method Call '%s' failed, err '%s(%X)'", method_name, err_name, ret);
	g_dbus_method_invocation_return_dbus_error(invocation, err_name, "failed");
}

static void handle_method_test(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int val = 0, val2 = 0;
	GVariant* params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(ii)", &val, &val2);
	debug_log("Got value : %d , %d", val, val2);

	if ((ret = mm_sound_mgr_ipc_dbus_send_signal(AUDIO_EVENT_TEST, g_variant_new("(i)", val+val2))) != MM_ERROR_NONE) {
		debug_error("signal send error : %X", ret);
	} else {
		debug_error("signal send success");
	}

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("(i)", val * val2));
	} else {
		ret = MM_ERROR_INVALID_ARGUMENT;
		_method_call_return_error(invocation,  ret);
	}

	debug_fleave();
}

static void handle_method_play_file_start(GDBusMethodInvocation* invocation)
{
	gchar* filename = NULL;
	char *stream_type = NULL;
	gint32 ret = MM_ERROR_NONE, slotid = 0;
	gint32 tone = 0, repeat = 0, volume = 0, vol_config = 0, priority = 0;
	gint32 session_type = 0, session_option = 0, pid = 0, handle_route =0, stream_index = 0;
	gboolean enable_session = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(siiiiiiiiibsi)", &filename, &tone, &repeat, &volume,
		      &vol_config, &priority, &session_type, &session_option, &pid, &handle_route, &enable_session, &stream_type, &stream_index);
	if (!filename) {
	    debug_error("filename null");
	    ret = MM_ERROR_SOUND_INTERNAL;
	    goto send_reply;
	}
	ret = _MMSoundMgrIpcPlayFile(filename, tone, repeat, volume, vol_config, priority,
				session_type, session_option, _get_sender_pid(invocation), handle_route, enable_session, &slotid, stream_type, stream_index);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_play_file_start_with_stream_info(GDBusMethodInvocation* invocation)
{
	gchar* filename = NULL;
	char *stream_type = NULL;
	gint32 ret = MM_ERROR_NONE, slotid = 0;
	gint32 repeat = 0, volume = 0, priority = 0, pid = 0, handle_route =0, stream_index = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(siiiiisi)", &filename, &repeat, &volume,
		      &priority, &pid, &handle_route, &stream_type, &stream_index);
	if (!filename) {
	    debug_error("filename null");
	    ret = MM_ERROR_SOUND_INTERNAL;
	    goto send_reply;
	}
	ret = _MMSoundMgrIpcPlayFileWithStreamInfo(filename, repeat, volume, priority,
				_get_sender_pid(invocation), handle_route, &slotid, stream_type, stream_index);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_play_dtmf(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE, slotid = 0;
	int tone = 0, repeat = 0, volume = 0, vol_config = 0, session_type = 0, session_option = 0, pid = 0, stream_index = 0;
	char* stream_type = NULL;
	gboolean enable_session = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiiiibsi)", &tone, &repeat, &volume,
		      &vol_config, &session_type, &session_option, &pid, &enable_session, &stream_type, &stream_index);
	debug_error("volume - %d", volume);
	ret = _MMSoundMgrIpcPlayDTMF(tone, repeat, volume, vol_config,
				     session_type, session_option, _get_sender_pid(invocation), enable_session, &slotid, stream_type, stream_index);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		_method_call_return_error(invocation, ret);
	}


	debug_fleave();
}

static void handle_method_play_dtmf_with_stream_info(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE, slotid = 0;
	int tone = 0, repeat = 0, volume = 0, pid = 0, stream_index = 0;
	char* stream_type = NULL;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiisi)", &tone, &repeat, &volume, &pid, &stream_type, &stream_index);
	ret = _MMSoundMgrIpcPlayDTMFWithStreamInfo(tone, repeat, volume, _get_sender_pid(invocation), &slotid, stream_type, stream_index);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_play_file_stop(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int handle = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(i)", &handle);
	ret = _MMSoundMgrIpcStop(handle);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_clear_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(i)", &pid);
	ret = _MMSoundMgrIpcClearFocus(pid);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_get_connected_device_list(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	GVariant *params = NULL;
	GVariantBuilder reply_builder;
	int mask_flags = 0;
	int devices_num = 0, device_idx = 0;
	mm_sound_device_t *device_list = NULL, *device_entry = NULL;

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(i)", &mask_flags);
	debug_log("Get device list with device_flag %X", mask_flags);
	if ((ret = __mm_sound_mgr_ipc_get_current_connected_device_list(mask_flags, &device_list, &devices_num))==MM_ERROR_NONE) {
		g_variant_builder_init(&reply_builder, G_VARIANT_TYPE("(a(iiiis))"));
		g_variant_builder_open(&reply_builder, G_VARIANT_TYPE("a(iiiis)"));
		for (device_idx = 0; device_idx < devices_num; device_idx++) {
			device_entry = &device_list[device_idx];
//			debug_log("device(%d): id(%d), type(%d), io(%d), state(%d), name(%s) ", device_idx, device_entry->id, device_entry->type, device_entry->io_direction, device_entry->state, device_entry->name);
			g_variant_builder_add(&reply_builder, "(iiiis)", device_entry->id, device_entry->type, device_entry->io_direction, device_entry->state, device_entry->name);
		}
		g_variant_builder_close(&reply_builder);
	}

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_builder_end(&reply_builder));
		debug_log("Reply Sent");
	} else {
		_method_call_return_error(invocation, ret);
	}
}

static void handle_method_call(GDBusConnection *connection,
							const gchar *sender,
							const gchar *object_path,
							const gchar *interface_name,
							const gchar *method_name,
							GVariant *parameters,
							GDBusMethodInvocation *invocation,
							gpointer userdata)
{
	int method_idx = 0;

	if (!parameters) {
		debug_error("Parameter Null");
		return;
	}
	debug_log("Method Call, obj : %s, intf : %s, method : %s", object_path, interface_name, method_name);

	for (method_idx = 0; method_idx < AUDIO_METHOD_MAX; method_idx++) {
		if (!g_strcmp0(method_name, methods[method_idx].info.name)) {
			methods[method_idx].handler(invocation);
		}
	}
}


static GVariant* handle_get_property(GDBusConnection *connection,
									const gchar *sender,
									const gchar *object_path,
									const gchar *interface_name,
									const gchar *property_name,
									GError **error,
									gpointer userdata)
{
	debug_log("Get Property, obj : %s, intf : %s, prop : %s", object_path, interface_name, property_name);
	return NULL;
}

static gboolean handle_set_property(GDBusConnection *connection,
									const gchar *sender,
									const gchar *object_path,
									const gchar *interface_name,
									const gchar *property_name,
									GVariant *value,
									GError **error,
									gpointer userdata)
{
	debug_log("Set Property, obj : %s, intf : %s, prop : %s", object_path, interface_name, property_name);
	return TRUE;
}

static const GDBusInterfaceVTable interface_vtable =
{
	handle_method_call,
	handle_get_property,
	handle_set_property
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	guint reg_id;
	debug_log("Bus Acquired (%s)", name);

	conn_g = connection;
	reg_id = g_dbus_connection_register_object(connection,
					  OBJECT_SOUND_SERVER,
					  introspection_data->interfaces[0],
					  &interface_vtable,
					  NULL,
					  NULL,
					  NULL);
	if (!reg_id) {
		debug_error("Register object(%s) failed", OBJECT_SOUND_SERVER);
		return ;
	}

}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	debug_log("Name Acquired (%s)", name);
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	debug_log("Name Lost (%s)", name);
}

static int _mm_sound_mgr_dbus_own_name(GBusType bus_type, const char* wellknown_name, guint* owner_id)
{
	guint oid;

	debug_log("Own name (%s) for sound-server", wellknown_name);

	oid = g_bus_own_name(bus_type, wellknown_name , G_BUS_NAME_OWNER_FLAGS_NONE,
			on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
	if (oid <= 0) {
		debug_error("Dbus own name failed");
		return MM_ERROR_SOUND_INTERNAL;
	} else {
		*owner_id = oid;
	}

	return MM_ERROR_NONE;
}

static void _mm_sound_mgr_dbus_unown_name(guint oid)
{
	debug_log("Unown name for Sound-Server");
	if (oid > 0) {
		g_bus_unown_name(oid);
	}
}

int __mm_sound_mgr_ipc_dbus_notify_device_connected (mm_sound_device_t *device, gboolean is_connected)
{
	int ret = MM_ERROR_NONE;
	GVariantBuilder builder;
	GVariant* param = NULL;

	debug_log("Send device connected signal");

	g_variant_builder_init(&builder, G_VARIANT_TYPE("((iiiis)b)"));
	g_variant_builder_open(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add(&builder, "(iiiis)", device->id, device->type, device->io_direction, device->state, device->name);
	g_variant_builder_close(&builder);
	g_variant_builder_add(&builder, "b", is_connected);
	param = g_variant_builder_end(&builder);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(AUDIO_EVENT_DEVICE_CONNECTED, param))!= MM_ERROR_NONE) {
			debug_error("Send device connected signal failed");
		}
	} else {
		debug_error("Build variant for dbus param failed");
	}

	return ret;
}

int __mm_sound_mgr_ipc_dbus_notify_device_info_changed (mm_sound_device_t *device, int changed_device_info_type)
{
	int ret = MM_ERROR_NONE;
	GVariantBuilder builder;
	GVariant* param = NULL;

	debug_log("Send device info changed signal");

	g_variant_builder_init(&builder, G_VARIANT_TYPE("((iiiis)i)"));
	g_variant_builder_open(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add(&builder, "(iiiis)", device->id, device->type, device->io_direction, device->state, device->name);
	g_variant_builder_close(&builder);
	g_variant_builder_add(&builder, "i", changed_device_info_type);
	param = g_variant_builder_end(&builder);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(AUDIO_EVENT_DEVICE_INFO_CHANGED, param)) != MM_ERROR_NONE) {
			debug_error("Send device info changed signal failed");
		}
	} else {
		debug_error("Build variant for dbus param failed");
	}

	return ret;
}

int __mm_sound_mgr_ipc_dbus_notify_volume_changed(unsigned int vol_type, unsigned int value)
{
	int ret = MM_ERROR_NONE;
	GVariant* param = NULL;

	debug_log("Send Signal volume changed signal");

	param = g_variant_new("(uu)", vol_type, value);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(AUDIO_EVENT_VOLUME_CHANGED, param)) != MM_ERROR_NONE) {
			debug_error("Send device connected signal failed");
		}
	} else {
		debug_error("Build variant for dbus param failed");
	}

	return ret;
}

int __mm_sound_mgr_ipc_dbus_notify_play_file_end(int handle)
{
	int ret = MM_ERROR_NONE;
	GVariant* param = NULL;

	debug_log("Send play file ended signal");

	param = g_variant_new("(i)", handle);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(AUDIO_EVENT_PLAY_FILE_END, param)) != MM_ERROR_NONE) {
			debug_error("Send play file end for '%d' failed", handle);
		}
	} else {
		debug_error("Build variant for dbus param failed");
	}

	return ret;
}

int __mm_sound_mgr_ipc_dbus_notify_active_device_changed(int device_in, int device_out)
{
	return MM_ERROR_SOUND_INTERNAL;
}

int __mm_sound_mgr_ipc_dbus_notify_available_device_changed(int device_in, int device_out, int available)
{
	return MM_ERROR_SOUND_INTERNAL;
}

#define PA_BUS_NAME                                    "org.pulseaudio.Server"
#define PA_STREAM_MANAGER_OBJECT_PATH                  "/org/pulseaudio/StreamManager"
#define PA_STREAM_MANAGER_INTERFACE                    "org.pulseaudio.StreamManager"
#define PA_STREAM_MANAGER_METHOD_NAME_GET_STREAM_LIST  "GetStreamList"
int __mm_sound_mgr_ipc_dbus_get_stream_list(stream_list_t* stream_list)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;
	GVariant *child = NULL;
	GDBusConnection *conn = NULL;
	GError *err = NULL;
	int i = 0;

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!conn && err) {
		LOGE("g_bus_get_sync() error (%s)", err->message);
		g_error_free (err);
		ret = MM_ERROR_SOUND_INTERNAL;
		return ret;
	}
	result = g_dbus_connection_call_sync (conn,
							PA_BUS_NAME,
							PA_STREAM_MANAGER_OBJECT_PATH,
							PA_STREAM_MANAGER_INTERFACE,
							PA_STREAM_MANAGER_METHOD_NAME_GET_STREAM_LIST,
							NULL,
							G_VARIANT_TYPE("(vv)"),
							G_DBUS_CALL_FLAGS_NONE,
							2000,
							NULL,
							&err);
	if (!result && err) {
		debug_error("g_dbus_connection_call_sync() error (%s)", err->message);
		ret = MM_ERROR_SOUND_INTERNAL;
	} else {
		GVariantIter iter;
		GVariant *item = NULL;
		child = g_variant_get_child_value(result, 0);
		item = g_variant_get_variant(child);
		gchar *name;
		i = 0;
		g_variant_iter_init(&iter, item);
		while ((i < AVAIL_STREAMS_MAX) && g_variant_iter_loop(&iter, "&s", &name)) {
			debug_log ("name : %s", name);
			stream_list->stream_types[i++] = strdup(name);
		}
		g_variant_iter_free (&iter);
		g_variant_unref (item);
		g_variant_unref (child);

		child = g_variant_get_child_value(result, 1);
		item = g_variant_get_variant(child);
		gint32 priority;
		i = 0;
		g_variant_iter_init(&iter, item);
		while ((i < AVAIL_STREAMS_MAX) && g_variant_iter_loop(&iter, "i", &priority)) {
			debug_log ("priority : %d", priority);
			stream_list->priorities[i++] = priority;
		}
		g_variant_iter_free (&iter);
		g_variant_unref (item);
		g_variant_unref (child);

		g_variant_unref(result);
	}
	g_object_unref(conn);

	return ret;
}

int MMSoundMgrDbusInit(void)
{
	debug_enter();

	introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	if (!introspection_data)
		return MM_ERROR_SOUND_INTERNAL;

	if (_mm_sound_mgr_dbus_own_name(G_BUS_TYPE_SYSTEM, BUS_NAME_SOUND_SERVER, &sound_server_owner_id) != MM_ERROR_NONE) {
		debug_error ("dbus own name for sound-server error\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_leave();

	return MM_ERROR_NONE;
}

void MMSoundMgrDbusFini(void)
{
	debug_enter("\n");

	_mm_sound_mgr_dbus_unown_name(sound_server_owner_id);
	g_dbus_node_info_unref (introspection_data);

	debug_leave("\n");
}



