
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mm_types.h>
#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>
#include <glib.h>

#ifdef USE_SECURITY
#include <security-server.h>
#define COOKIE_SIZE 20
#endif

#include "include/mm_sound_mgr_focus_dbus.h"
#include "include/mm_sound_mgr_focus_ipc.h"


#define BUS_NAME_FOCUS_SERVER "org.tizen.FocusServer"
#define OBJECT_FOCUS_SERVER "/org/tizen/FocusServer1"
#define INTERFACE_FOCUS_SERVER "org.tizen.FocusServer1"

/* Introspection data for the service we are exporting */
  static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.tizen.FocusServer1'>"
  "    <method name='GetUniqueId'>"
  "      <arg name='id' type='i' direction='out'/>"
  "    </method>"
  "    <method name='RegisterFocus'>"
#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
  "      <arg name='container' type='ay' direction='in'/>"
#else
  "      <arg name='container' type='s' direction='in'/>"
#endif
#endif
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='stream_type' type='s' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='UnregisterFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='SetFocusReacquisition'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='reacquisition' type='b' direction='in'/>"
  "    </method>"
  "    <method name='AcquireFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='focus_type' type='i' direction='in'/>"
  "      <arg name='name' type='s' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='ReleaseFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='focus_type' type='i' direction='in'/>"
  "      <arg name='name' type='s' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='WatchFocus'>"
#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	  " 	 <arg name='container' type='ay' direction='in'/>"
#else
	  " 	 <arg name='container' type='s' direction='in'/>"
#endif
#endif
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='focus_type' type='i' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='UnwatchFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='is_for_session' type='b' direction='in'/>"
  "    </method>"
  "    <method name='EmergentExitFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";
GDBusConnection* conn_g;

typedef void (*dbus_method_handler)(GDBusMethodInvocation *invocation);
typedef int (*dbus_signal_sender)(GDBusConnection *conn, GVariant *parameter);

struct mm_sound_mgr_focus_dbus_method{
	struct mm_sound_dbus_method_info info;
        dbus_method_handler handler;
};

struct mm_sound_mgr_focus_dbus_signal{
	struct mm_sound_dbus_signal_info info;
	dbus_signal_sender sender;
};

static void handle_method_get_unique_id(GDBusMethodInvocation* invocation);
static void handle_method_register_focus(GDBusMethodInvocation* invocation);
static void handle_method_unregister_focus(GDBusMethodInvocation* invocation);
static void handle_method_set_focus_reacquisition(GDBusMethodInvocation* invocation);
static void handle_method_acquire_focus(GDBusMethodInvocation* invocation);
static void handle_method_release_focus(GDBusMethodInvocation* invocation);
static void handle_method_watch_focus(GDBusMethodInvocation* invocation);
static void handle_method_unwatch_focus(GDBusMethodInvocation* invocation);
static void handle_method_emergent_exit_focus (GDBusMethodInvocation* invocation);

/* Currently , Just using method's name and handler */
/* TODO : generate introspection xml automatically, with these value include argument and reply */
/* TODO : argument check with these information */
/* TODO : divide object and interface with features (ex. play, path, device, focus, asm) */
struct mm_sound_mgr_focus_dbus_method methods[METHOD_CALL_MAX] = {
	[METHOD_CALL_GET_UNIQUE_ID] = {
		.info = {
			.name = "GetUniqueId",
		},
		.handler = handle_method_get_unique_id
	},
	[METHOD_CALL_REGISTER_FOCUS] = {
		.info = {
			.name = "RegisterFocus",
		},
		.handler = handle_method_register_focus
	},
	[METHOD_CALL_UNREGISTER_FOCUS] = {
		.info = {
			.name = "UnregisterFocus",
		},
		.handler = handle_method_unregister_focus
	},
	[METHOD_CALL_SET_FOCUS_REACQUISITION] = {
		.info = {
			.name = "SetFocusReacquisition",
		},
		.handler = handle_method_set_focus_reacquisition
	},
	[METHOD_CALL_ACQUIRE_FOCUS] = {
		.info = {
			.name = "AcquireFocus",
		},
		.handler = handle_method_acquire_focus
	},
	[METHOD_CALL_RELEASE_FOCUS] = {
		.info = {
			.name = "ReleaseFocus",
		},
		.handler = handle_method_release_focus
	},
	[METHOD_CALL_WATCH_FOCUS] = {
		.info = {
			.name = "WatchFocus",
		},
		.handler = handle_method_watch_focus
	},
	[METHOD_CALL_UNWATCH_FOCUS] = {
		.info = {
			.name = "UnwatchFocus",
		},
		.handler = handle_method_unwatch_focus
	},
	[METHOD_CALL_EMERGENT_EXIT_FOCUS] = {
		.info = {
			.name = "EmergentExitFocus",
		},
		.handler = handle_method_emergent_exit_focus
	},
};


struct mm_sound_mgr_focus_dbus_signal signals[SIGNAL_MAX] = {
	[SIGNAL_TEST] = {
		.info = {
			.name = "SignalTest1",
		},
	},
	[SIGNAL_FOCUS_CHANGED] = {
		.info = {
			.name = "FocusChanged",
		},
	},
	[SIGNAL_FOCUS_WATCH] = {
		.info = {
			.name = "FocusWatch",
		},
	}
};


static GDBusNodeInfo *introspection_data = NULL;
guint focus_server_owner_id ;

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

static int _get_sender_pid(GDBusMethodInvocation* invocation)
{
	GVariant* value;
	guint pid;
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

static void handle_method_get_unique_id(GDBusMethodInvocation* invocation)
{
	static int unique_id = 0;

	debug_fenter();

	_method_call_return_value(invocation, g_variant_new("(i)", ++unique_id));

	debug_fleave();
}

static void handle_method_register_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int handle_id = 0;
	const char* stream_type = NULL;
	gboolean is_for_session;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#else
	int pid = 0;
#endif /* SUPPORT_CONTAINER */


	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiisb)", &cookie_data, &container_pid, &handle_id, &stream_type, &is_for_session);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_focus_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type, is_for_session, container, container_pid);

	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siisb)", &container, &container_pid, &handle_id, &stream_type, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type, is_for_session, container, container_pid);

#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iisb)", &pid, &handle_id, &stream_type, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type, is_for_session);

#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_unregister_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0;
	gboolean is_for_session;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iib)", &pid, &handle_id, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_unregister_focus((is_for_session) ? pid : _get_sender_pid(invocation), handle_id);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_set_focus_reacquisition(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0;
	gboolean reacquisition;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iib)", &pid, &handle_id, &reacquisition);
	ret = __mm_sound_mgr_focus_ipc_set_focus_reacquisition(_get_sender_pid(invocation), handle_id, reacquisition);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_acquire_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0, focus_type = 0;
	const char* name = NULL;
	gboolean is_for_session;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiisb)", &pid, &handle_id, &focus_type, &name, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_acquire_focus((is_for_session) ? pid : _get_sender_pid(invocation), handle_id, focus_type, name);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_release_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0, focus_type = 0;
	const char* name = NULL;
	gboolean is_for_session;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiisb)", &pid, &handle_id, &focus_type, &name, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_release_focus((is_for_session) ? pid : _get_sender_pid(invocation), handle_id, focus_type, name);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_watch_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int handle_id = 0, focus_type = 0;
	gboolean is_for_session;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#else
	int pid = 0;
#endif /* SUPPORT_CONTAINER */

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiiib)", &cookie_data, &container_pid, &handle_id, &focus_type, &is_for_session);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_focus_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type, is_for_session, container, container_pid);

	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siiib)", &container, &container_pid, &handle_id, &focus_type, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type, is_for_session, container, container_pid);

#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iiib)", &pid, &handle_id, &focus_type, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type, is_for_session);

#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_unwatch_focus (GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0;
	int handle_id = 0;
	gboolean is_for_session;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iib)", &pid, &handle_id, &is_for_session);
	ret = __mm_sound_mgr_focus_ipc_unwatch_focus((is_for_session) ? pid : _get_sender_pid(invocation), handle_id);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

static void handle_method_emergent_exit_focus (GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		goto send_reply;
	}

	g_variant_get(params, "(i)", &pid);
	ret = __mm_sound_mgr_focus_ipc_emergent_exit(_get_sender_pid(invocation));
	if(ret)
		debug_error("__mm_sound_mgr_focus_ipc_emergent_exit faild : 0x%x", ret);

send_reply:
	if (ret == MM_ERROR_NONE) {
		_method_call_return_value(invocation, g_variant_new("()"));
	} else {
		_method_call_return_error(invocation, ret);
	}

	debug_fleave();
}

/**********************************************************************************/
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

	for (method_idx = METHOD_CALL_GET_UNIQUE_ID; method_idx < METHOD_CALL_MAX; method_idx++) {
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

#if 0
static void handle_signal(GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *params,
                                     gpointer          user_data)
{
	if (!object_path || !interface_name || !signal_name) {
		debug_error("Invalid Parameters");
		return;
	}

	debug_log("Got Signal : Object '%s, Interface '%s', Signal '%s'", object_path, interface_name, signal_name);

	if (!g_strcmp0(object_path, OBJECT_ASM)) {
		if (!g_strcmp0(interface_name, INTERFACE_ASM) && !g_strcmp0(signal_name, "EmergentExit")) {
			debug_log("handle signal '%s.%s'", interface_name, signal_name);
			handle_signal_asm_emergent_exit(params, user_data);
		} else {
			debug_log("Unknown Signal '%s.%s'", interface_name, signal_name);
		}
	} else {
		debug_log("Unknown Object '%s'", object_path);
	}

}
#endif

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	guint reg_id;
	debug_log("Bus Acquired (%s)", name);

	conn_g = connection;
	reg_id = g_dbus_connection_register_object(connection,
					  OBJECT_FOCUS_SERVER,
					  introspection_data->interfaces[0],
					  &interface_vtable,
					  NULL,
					  NULL,
					  NULL);
	if (!reg_id) {
		debug_error("Register object(%s) failed", OBJECT_FOCUS_SERVER);
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

static int _mm_sound_mgr_focus_dbus_own_name(GBusType bus_type, const char* wellknown_name, guint* owner_id)
{
	guint oid;

	debug_log("Own name (%s) for focus-server", wellknown_name);

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

static void _mm_sound_mgr_focus_dbus_unown_name(guint oid)
{
	debug_log("Unown name for focus-server");
	if (oid > 0) {
		g_bus_unown_name(oid);
	}
}

#define PA_BUS_NAME                                    "org.pulseaudio.Server"
#define PA_STREAM_MANAGER_OBJECT_PATH                  "/org/pulseaudio/StreamManager"
#define PA_STREAM_MANAGER_INTERFACE                    "org.pulseaudio.StreamManager"
#define PA_STREAM_MANAGER_METHOD_NAME_GET_STREAM_LIST  "GetStreamList"
int __mm_sound_mgr_focus_dbus_get_stream_list(stream_list_t* stream_list)
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


int MMSoundMgrFocusDbusInit(void)
{
	debug_enter();

	introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	if (!introspection_data)
		return MM_ERROR_SOUND_INTERNAL;

	if (_mm_sound_mgr_focus_dbus_own_name(G_BUS_TYPE_SYSTEM, BUS_NAME_FOCUS_SERVER, &focus_server_owner_id) != MM_ERROR_NONE) {
		debug_error ("dbus own name for focus-server error\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_leave();

	return MM_ERROR_NONE;
}

void MMSoundMgrFocusDbusFini(void)
{
	debug_enter("\n");

	_mm_sound_mgr_focus_dbus_unown_name(focus_server_owner_id);
	g_dbus_node_info_unref (introspection_data);

	debug_leave("\n");
}



