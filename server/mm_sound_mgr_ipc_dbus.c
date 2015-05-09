
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

#include "include/mm_sound_mgr_ipc_dbus.h"
#include "include/mm_sound_mgr_ipc.h"


#define BUS_NAME_SOUND_SERVER "org.tizen.SoundServer"
#define OBJECT_SOUND_SERVER "/org/tizen/SoundServer1"
#define INTERFACE_SOUND_SERVER "org.tizen.SoundServer1"

/* workaround for AF volume gain tuning */
#define PROC_DBUS_OBJECT 	"/Org/Tizen/ResourceD/Process"
#define PROC_DBUS_INTERFACE 	"org.tizen.resourced.process"
#define PROC_DBUS_METHOD 	"ProcExclude"

#define OBJECT_ASM "/org/tizen/asm"
#define INTERFACE_ASM "org.tizen.asm"

#define MM_SOUND_ERROR mm_sound_error_quark()

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
  "      <arg type='i' name='keytone' direction='in'/>"
  "      <arg type='i' name='handle_route' direction='in'/>"
  "      <arg type='b' name='enable_session' direction='in'/>"
  "      <arg type='i' name='handle' direction='out'/>"
  "    </method>"
  "    <method name='PlayFileStop'>"
  "      <arg type='i' name='handle' direction='in'/>"
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
  "      <arg type='i' name='handle' direction='out'/>"
  "    </method>"
  "    <method name='SetPathForActiveDevice'>"
  "    </method>"
  "    <method name='GetConnectedDeviceList'>"
  "      <arg type='i' name='device_mask' direction='in'/>"
  "      <arg type='a(iiiis)' name='device_list' direction='out'/>"
  "    </method>"
  "    <method name='GetAudioPath'>"
  "      <arg type='i' name='device_in' direction='out'/>"
  "      <arg type='i' name='device_out' direction='out'/>"
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
  "    </method>"
  "    <method name='UnregisterFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "    </method>"
  "    <method name='AcquireFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='focus_type' type='i' direction='in'/>"
  "      <arg name='name' type='s' direction='in'/>"
  "    </method>"
  "    <method name='ReleaseFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "      <arg name='focus_type' type='i' direction='in'/>"
  "      <arg name='name' type='s' direction='in'/>"
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
  "    </method>"
  "    <method name='UnwatchFocus'>"
  "      <arg name='pid' type='i' direction='in'/>"
  "      <arg name='handle_id' type='i' direction='in'/>"
  "    </method>"
  "    <method name='ASMRegisterSound'>"
#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
  "      <arg name='container' type='ay' direction='in'/>"
#else
  "      <arg name='container' type='s' direction='in'/>"
#endif
#endif
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMUnregisterSound'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "    </method>"
  "    <method name='ASMRegisterWatcher'>"
#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
  "      <arg name='container' type='ay' direction='in'/>"
#else
  "      <arg name='container' type='s' direction='in'/>"
#endif
#endif
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMUnregisterWatcher'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "    </method>"
  "    <method name='ASMGetMyState'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMGetState'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMSetState'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "      <arg name='snd_error_code' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMSetSubsession'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMGetSubsession'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMSetSubevent'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMGetSubevent'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMSetSessionOption'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMGetSessionOption'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_option_flag' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMResetResumeTag'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "      <arg name='snd_pid' type='i' direction='out'/>"
  "      <arg name='snd_alloc_handle' type='i' direction='out'/>"
  "      <arg name='snd_cmd_handle' type='i' direction='out'/>"
  "      <arg name='snd_request_id' type='i' direction='out'/>"
  "      <arg name='snd_sound_command' type='i' direction='out'/>"
  "      <arg name='snd_sound_state' type='i' direction='out'/>"
  "    </method>"
  "    <method name='ASMDump'>"
  "      <arg name='rcv_pid' type='i' direction='in'/>"
  "      <arg name='rcv_handle' type='i' direction='in'/>"
  "      <arg name='rcv_sound_event' type='i' direction='in'/>"
  "      <arg name='rcv_request_id' type='i' direction='in'/>"
  "      <arg name='rcv_sound_state' type='i' direction='in'/>"
  "      <arg name='rcv_resource' type='i' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";
GDBusConnection* conn_g;

typedef void (*dbus_method_handler)(GDBusMethodInvocation *invocation);
typedef int (*dbus_signal_sender)(GDBusConnection *conn, GVariant *parameter);

struct mm_sound_dbus_method{
	struct mm_sound_dbus_method_info info;
        dbus_method_handler handler;
};

struct mm_sound_dbus_signal{
	struct mm_sound_dbus_signal_info info;
	dbus_signal_sender sender;
};

static void handle_method_play_file_start(GDBusMethodInvocation* invocation);
static void handle_method_play_file_stop(GDBusMethodInvocation* invocation);
static void handle_method_play_dtmf(GDBusMethodInvocation* invocation);
static void handle_method_get_bt_a2dp_status(GDBusMethodInvocation* invocation);
static void handle_method_test(GDBusMethodInvocation* invocation);
static void handle_method_set_sound_path_for_active_device(GDBusMethodInvocation* invocation);
static void handle_method_get_audio_path(GDBusMethodInvocation* invocation);
static void handle_method_get_connected_device_list(GDBusMethodInvocation* invocation);

static void handle_method_register_focus(GDBusMethodInvocation* invocation);
static void handle_method_unregister_focus(GDBusMethodInvocation* invocation);
static void handle_method_acquire_focus(GDBusMethodInvocation* invocation);
static void handle_method_release_focus(GDBusMethodInvocation* invocation);
static void handle_method_watch_focus(GDBusMethodInvocation* invocation);
static void handle_method_unwatch_focus(GDBusMethodInvocation* invocation);

static void handle_method_asm_register_sound(GDBusMethodInvocation* invocation);
static void handle_method_asm_unregister_sound(GDBusMethodInvocation* invocation);
static void handle_method_asm_register_watcher(GDBusMethodInvocation* invocation);
static void handle_method_asm_unregister_watcher(GDBusMethodInvocation* invocation);
static void handle_method_asm_get_mystate(GDBusMethodInvocation* invocation);
static void handle_method_asm_get_state(GDBusMethodInvocation* invocation);
static void handle_method_asm_set_state(GDBusMethodInvocation* invocation);
static void handle_method_asm_set_subsession(GDBusMethodInvocation* invocation);
static void handle_method_asm_get_subsession(GDBusMethodInvocation* invocation);
static void handle_method_asm_set_subevent(GDBusMethodInvocation* invocation);
static void handle_method_asm_get_subevent(GDBusMethodInvocation* invocation);
static void handle_method_asm_set_session_option(GDBusMethodInvocation* invocation);
static void handle_method_asm_get_session_option(GDBusMethodInvocation* invocation);
static void handle_method_asm_reset_resume_tag(GDBusMethodInvocation* invocation);
static void handle_method_asm_dump(GDBusMethodInvocation* invocation);

/* Currently , Just using method's name and handler */
/* TODO : generate introspection xml automatically, with these value include argument and reply */
/* TODO : argument check with these information */
/* TODO : divide object and interface with features (ex. play, path, device, focus, asm) */
struct mm_sound_dbus_method methods[METHOD_CALL_MAX] = {
	[METHOD_CALL_TEST] = {
		.info = {
			.name = "MethodTest1",
		},
		.handler = handle_method_test
	},
	[METHOD_CALL_PLAY_FILE_START] = {
		.info = {
			.name = "PlayFileStart",
		},
		.handler = handle_method_play_file_start
	},
	[METHOD_CALL_PLAY_FILE_STOP] = {
		.info = {
			.name = "PlayFileStop",
		},
		.handler = handle_method_play_file_stop
	},
	[METHOD_CALL_PLAY_DTMF] = {
		.info = {
			.name = "PlayDTMF",
		},
		.handler = handle_method_play_dtmf
	},
	[METHOD_CALL_GET_BT_A2DP_STATUS] = {
		.info = {
			.name = "GetBTA2DPStatus",
		},
		.handler = handle_method_get_bt_a2dp_status
	},
	[METHOD_CALL_SET_PATH_FOR_ACTIVE_DEVICE] = {
		.info = {
			.name = "SetPathForActiveDevice",
		},
		.handler = handle_method_set_sound_path_for_active_device
	},
	[METHOD_CALL_GET_AUDIO_PATH] = {
		.info = {
			.name = "GetAudioPath",
		},
		.handler = handle_method_get_audio_path
	},
	[METHOD_CALL_GET_CONNECTED_DEVICE_LIST] = {
		.info = {
			.name = "GetConnectedDeviceList",
		},
		.handler = handle_method_get_connected_device_list
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
	[METHOD_CALL_ASM_REGISTER_SOUND] = {
		.info = {
			.name = "ASMRegisterSound",
		},
		.handler = handle_method_asm_register_sound
	},
	[METHOD_CALL_ASM_UNREGISTER_SOUND] = {
		.info = {
			.name = "ASMUnregisterSound",
		},
		.handler = handle_method_asm_unregister_sound
	},
	[METHOD_CALL_ASM_REGISTER_WATCHER] = {
		.info = {
			.name = "ASMRegisterWatcher",
		},
		.handler = handle_method_asm_register_watcher
	},
	[METHOD_CALL_ASM_UNREGISTER_WATCHER] = {
		.info = {
			.name = "ASMUnregisterWatcher",
		},
		.handler = handle_method_asm_unregister_watcher
	},
	[METHOD_CALL_ASM_GET_MYSTATE] = {
		.info = {
			.name = "ASMGetMyState",
		},
		.handler = handle_method_asm_get_mystate
	},
	[METHOD_CALL_ASM_GET_STATE] = {
		.info = {
			.name = "ASMGetState",
		},
		.handler = handle_method_asm_get_state
	},
	[METHOD_CALL_ASM_SET_STATE] = {
		.info = {
			.name = "ASMSetState",
		},
		.handler = handle_method_asm_set_state
	},
	[METHOD_CALL_ASM_SET_SUBSESSION] = {
		.info = {
			.name = "ASMSetSubsession",
		},
		.handler = handle_method_asm_set_subsession
	},
	[METHOD_CALL_ASM_GET_SUBSESSION] = {
		.info = {
			.name = "ASMGetSubsession",
		},
		.handler = handle_method_asm_get_subsession
	},
	[METHOD_CALL_ASM_SET_SUBEVENT] = {
		.info = {
			.name = "ASMSetSubevent",
		},
		.handler = handle_method_asm_set_subevent
	},
	[METHOD_CALL_ASM_GET_SUBEVENT] = {
		.info = {
			.name = "ASMGetSubevent",
		},
		.handler = handle_method_asm_get_subevent
	},
	[METHOD_CALL_ASM_SET_SESSION_OPTION] = {
		.info = {
			.name = "ASMSetSessionOption",
		},
		.handler = handle_method_asm_set_session_option
	},
	[METHOD_CALL_ASM_GET_SESSION_OPTION] = {
		.info = {
			.name = "ASMGetSessionOption",
		},
		.handler = handle_method_asm_get_session_option
	},
	[METHOD_CALL_ASM_RESET_RESUME_TAG] = {
		.info = {
			.name = "ASMResetResumeTag",
		},
		.handler = handle_method_asm_reset_resume_tag
	},
	[METHOD_CALL_ASM_DUMP] = {
		.info = {
			.name = "ASMDump",
		},
		.handler = handle_method_asm_dump
	},
};

struct mm_sound_dbus_signal signals[SIGNAL_MAX] = {
	[SIGNAL_TEST] = {
		.info = {
			.name = "SignalTest1",
		},
	},
	[SIGNAL_PLAY_FILE_END] = {
		.info = {
			.name = "PlayFileEnd",
		},
	},
	[SIGNAL_VOLUME_CHANGED] = {
		.info = {
			.name = "VolumeChanged",
		},
	},
	[SIGNAL_DEVICE_CONNECTED] = {
		.info = {
			.name = "DeviceConnected",
		},
	},
	[SIGNAL_DEVICE_INFO_CHANGED] = {
		.info = {
			.name = "DeviceInfoChanged",
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

GQuark mm_sound_error_quark (void)
{
	static volatile gsize quark_volatile = 0;
	g_dbus_error_register_error_domain("mm-sound-error-quark",
					   &quark_volatile,
					   mm_sound_error_entries,
					   G_N_ELEMENTS(mm_sound_error_entries));
	return (GQuark) quark_volatile;
}


static int mm_sound_mgr_ipc_dbus_send_signal(int signal_type, GVariant *parameter)
{
	int ret = MM_ERROR_NONE;
	GDBusConnection *conn = NULL;
	GError* err = NULL;
	gboolean emit_success = FALSE;

	if (signal_type < 0 || signal_type >= SIGNAL_MAX || !parameter) {
		debug_error("Invalid Argument");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_log("Signal Emit : %s", signals[signal_type].info.name);

	if (!conn_g) {
		conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
		if (!conn && err) {
			debug_error ("g_bus_get_sync() error (%s) ", err->message);
			g_error_free (err);
			return MM_ERROR_SOUND_INTERNAL;
		}
		conn_g = conn;
	}
/*
	if (!g_variant_is_of_type(parameter, G_VARIANT_TYPE(signals[signal_type].info.argument))) {
		debug_error("Invalid Signal Parameter");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	*/

	emit_success = g_dbus_connection_emit_signal(conn_g, NULL, OBJECT_SOUND_SERVER, INTERFACE_SOUND_SERVER,
												signals[signal_type].info.name, parameter, &err);
	if (!emit_success && err) {
		debug_error("Emit signal (%s) failed, (%s)", signals[signal_type].info.name, err->message);
		g_error_free(err);
		return MM_ERROR_SOUND_INTERNAL;
	}

	return ret;
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

	if ((ret = mm_sound_mgr_ipc_dbus_send_signal(SIGNAL_TEST, g_variant_new("(i)", val+val2))) != MM_ERROR_NONE) {
		debug_error("signal send error : %X", ret);
	} else {
		debug_error("signal send success");
	}

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Method Test success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", val*val2) );
	} else {
		debug_error("Method Test failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "method for test failed");
	}

	debug_fleave();
}

static void handle_method_play_file_start(GDBusMethodInvocation* invocation)
{
	gchar* filename = NULL;
	gint32 ret = MM_ERROR_NONE, slotid = 0;
	gint32 tone = 0, repeat = 0, volume = 0, vol_config = 0, priority = 0;
	gint32 session_type = 0, session_option = 0, pid = 0, keytone = 0, handle_route =0;
	gboolean enable_session = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(siiiiiiiiiib)", &filename, &tone, &repeat, &volume,
		      &vol_config, &priority, &session_type, &session_option, &pid, &keytone, &handle_route, &enable_session);
	if (!filename) {
	    debug_error("filename null");
	    ret = MM_ERROR_SOUND_INTERNAL;
	    goto send_reply;
	}
	ret = _MMSoundMgrIpcPlayFile(filename, tone, repeat, volume, vol_config, priority,
				session_type, session_option, _get_sender_pid(invocation), keytone, handle_route, enable_session, &slotid);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Method Play file start success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		debug_error("Method Play file start failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Play file failed");
	}

	debug_fleave();
}


static void handle_method_play_dtmf(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE, slotid = 0;
	int tone = 0, repeat = 0, volume = 0, vol_config = 0, session_type = 0, session_option = 0, pid = 0;
	gboolean enable_session = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiiiib)", &tone, &repeat, &volume,
		      &vol_config, &session_type, &session_option, &pid, &enable_session);
	ret = _MMSoundMgrIpcPlayDTMF(tone, repeat, volume, vol_config,
				     session_type, session_option, _get_sender_pid(invocation), enable_session, &slotid);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Method play dtmf success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", slotid));
	} else {
		debug_error("Method play dtmf failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Play DTMF failed");
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
		debug_error("Method Stop file playing Success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Method Stop file playing failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Stop file playing failed");
	}

	debug_fleave();
}
static void handle_method_get_bt_a2dp_status(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	gboolean is_bt_on = FALSE;
	gchar* bt_name = NULL;

	debug_fenter();

//	ret = MMSoundMgrPulseHandleIsBtA2DPOnReq(&is_bt_on, &bt_name);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Get BT A2DP status success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", is_bt_on, bt_name));
	} else {
		debug_error("Get BT A2DP status failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Get BT A2DP status failed");
	}

	if (bt_name)
		g_free(bt_name);

	debug_fleave();
}

static void handle_method_set_sound_path_for_active_device( GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int device_in = 0, device_out = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(ii)", &device_in, &device_out);
	ret = __mm_sound_mgr_ipc_set_sound_path_for_active_device(device_in, device_out);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_log("Set sound path for active device success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Set sound path for active device failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Set Sound Path for active device failed");
	}

	debug_fleave();
}

static void handle_method_get_audio_path(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int device_in = 0, device_out = 0;

	debug_fenter();

	ret = __mm_sound_mgr_ipc_get_audio_path(&device_in, &device_out);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Get audio path success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(ii)", device_in, device_out));
	} else {
		debug_error("Get audio path failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Set audio Path for active device failed");
	}

	debug_fleave();
}

static void handle_method_get_connected_device_list(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	GVariant *params = NULL, *reply_v = NULL;
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
		g_dbus_method_invocation_return_value(invocation, g_variant_builder_end(&reply_builder));
		debug_log("Reply Sent");
	} else {
		debug_error("Get connected device list failed");
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Get Connected device list failed");
	}
}

static void handle_method_register_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0;
	const char* stream_type = NULL;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */


	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiis)", &cookie_data, &container_pid, &handle_id, &stream_type);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type, container, container_pid);

	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siis)", &container, &container_pid, &handle_id, &stream_type);
	ret = __mm_sound_mgr_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type, container, container_pid);

#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iis)", &pid, &handle_id, &stream_type);
	ret = __mm_sound_mgr_ipc_register_focus(_get_sender_pid(invocation), handle_id, stream_type);

#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Register focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Register focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Register focus failed");
	}

	debug_fleave();
}

static void handle_method_unregister_focus(GDBusMethodInvocation* invocation)
{

	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(ii)", &pid, &handle_id);
	ret = __mm_sound_mgr_ipc_unregister_focus(_get_sender_pid(invocation), handle_id);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Unregister focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Unregister focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Unregister focus failed");
	}

	debug_fleave();
}

static void handle_method_acquire_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0, focus_type = 0;
	const char* name = NULL;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiis)", &pid, &handle_id, &focus_type, &name);
	ret = __mm_sound_mgr_ipc_acquire_focus(_get_sender_pid(invocation), handle_id, focus_type, name);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Acquire focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Acquire focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Acquire focus failed");
	}

	debug_fleave();
}

static void handle_method_release_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0, focus_type = 0;
	const char* name = NULL;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiis)", &pid, &handle_id, &focus_type, &name);
	ret = __mm_sound_mgr_ipc_release_focus(_get_sender_pid(invocation), handle_id, focus_type, name);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Release focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Release focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Release focus failed");
	}

	debug_fleave();
}

static void handle_method_watch_focus(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle_id = 0, focus_type = 0;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiii)", &cookie_data, &container_pid, &handle_id, &focus_type);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type, container, container_pid);

	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siii)", &container, &container_pid, &handle_id, &focus_type);
	ret = __mm_sound_mgr_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type, container, container_pid);

#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iii)", &pid, &handle_id, &focus_type);
	ret = __mm_sound_mgr_ipc_watch_focus(_get_sender_pid(invocation), handle_id, focus_type);

#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Watch focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Watch focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Watch focus failed");
	}

	debug_fleave();
}

static void handle_method_unwatch_focus (GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0;
	int handle_id = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(ii)", &pid, &handle_id);
	ret = __mm_sound_mgr_ipc_unwatch_focus(_get_sender_pid(invocation), handle_id);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("Unwatch focus success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("Unwatch focus failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "Unwatch focus failed");
	}

	debug_fleave();
}

/*********************** ASM METHODS ****************************/

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
static char* _get_container_from_cookie(GVariant* cookie_data)
{
	char* container = NULL;
	int cookie_len = 0;
	char* cookie = NULL;
	int ret = 0;

	cookie_len = g_variant_get_size(cookie_data);
	if (cookie_len != COOKIE_SIZE) {
		debug_error ("cookie_len = [%d]", cookie_len);
		return NULL;
	}

	ret = security_server_get_zone_by_cookie(g_variant_get_data(cookie_data), &container);
	if (ret == SECURITY_SERVER_API_SUCCESS) {
		debug_error ("success!!!! zone = [%s]", container);
	} else {
		debug_error ("failed!!!! ret = [%d]", ret);
	}

	return container;
}
#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */

// TODO : Too many arguments..
static void handle_method_asm_register_sound(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiiiiii)", &cookie_data, &container_pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_ipc_asm_register_sound(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					container, container_pid, &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siiiiii)", &container, &container_pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_register_sound(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					container, container_pid, &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_register_sound(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					&pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM register sound success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_command_r, sound_state_r));
	} else {
		debug_error("ASM register sound failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM register sound failed");
	}

	debug_fleave();
}

static void handle_method_asm_unregister_sound(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_unregister_sound(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM unregister sound success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("ASM unregister sound failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM register sound failed");
	}

	debug_fleave();
}

static void handle_method_asm_register_watcher(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;
#ifdef SUPPORT_CONTAINER
	int container_pid = -1;
	char* container = NULL;
#ifdef USE_SECURITY
	GVariant* cookie_data;
#endif /* USE_SECURITY */
#endif /* SUPPORT_CONTAINER */

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

#ifdef SUPPORT_CONTAINER
#ifdef USE_SECURITY
	g_variant_get(params, "(@ayiiiiii)", &cookie_data, &container_pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	container = _get_container_from_cookie(cookie_data);
	ret = __mm_sound_mgr_ipc_asm_register_watcher(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						container, container_pid, &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
	if (container)
		free(container);
#else /* USE_SECURITY */
	g_variant_get(params, "(siiiiii)", &container, &container_pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_register_watcher(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					container, container_pid, &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
#endif /* USE_SECURITY */
#else /* SUPPORT_CONTAINER */
	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_register_watcher(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
			&pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);
#endif /* SUPPORT_CONTAINER */

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM register watcher success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_command_r, sound_state_r));
	} else {
		debug_error("ASM register watcher failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM register watcher failed");
	}

	debug_fleave();
}

static void handle_method_asm_unregister_watcher(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_unregister_watcher(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM unregister watcher success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("ASM unregister watcher failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM unregister watcher failed");
	}

	debug_fleave();
}

static void handle_method_asm_get_mystate(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_get_mystate(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						      &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_state_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM get mystate success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_state_r));
	} else {
		debug_error("ASM get mystate failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM get mystate failed");
	}

	debug_fleave();
}

static void handle_method_asm_set_state(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0, error_code_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_set_state(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						      &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r, &error_code_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM requeset set state success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_command_r, sound_state_r, error_code_r));
	} else {
		debug_error("ASM set state failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM set state failed");
	}

	debug_fleave();
}

static void handle_method_asm_get_state(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_get_state(pid, handle, sound_event, request_id, sound_state, resource,
						      &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_state_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM get state success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_state_r));
	} else {
		debug_error("ASM get state failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM get state failed");
	}

	debug_fleave();
}

static void handle_method_asm_set_subsession(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_set_subsession(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						      &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM set subsession success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r));
	} else {
		debug_error("ASM set subsession failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM set subsession failed");
	}

	debug_fleave();
}

static void handle_method_asm_get_subsession(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_get_subsession(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						      &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM get subsession success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								    request_id_r, sound_command_r));
	} else {
		debug_error("ASM get subsession failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM get subsession failed");
	}

	debug_fleave();
}

static void handle_method_asm_set_subevent(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_set_subevent(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						   &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM set subevent success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								 request_id_r, sound_command_r, sound_state_r));
	} else {
		debug_error("ASM set subevent failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM set subevent failed");
	}

	debug_fleave();
}

static void handle_method_asm_get_subevent(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_get_subevent(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						   &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM get subevent success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiii)", pid_r, alloc_handle_r, cmd_handle_r,
								 request_id_r, sound_command_r ));
	} else {
		debug_error("ASM get subevent failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM get subevent failed");
	}

	debug_fleave();
}

static void handle_method_asm_set_session_option(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, error_code_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_set_session_option(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
						&pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &error_code_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM set session options success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
							      request_id_r, sound_command_r, error_code_r ));
	} else {
		debug_error("ASM set session options failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM set sesion options failed");
	}

	debug_fleave();
}

static void handle_method_asm_get_session_option(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, option_flag_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_get_session_option(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					     &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &option_flag_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM get session options success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
							   request_id_r, sound_command_r, option_flag_r ));
	} else {
		debug_error("ASM get session options failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM get sesion options failed");
	}

	debug_fleave();
}

static void handle_method_asm_reset_resume_tag(GDBusMethodInvocation* invocation)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	int pid_r = 0, alloc_handle_r = 0, cmd_handle_r = 0, request_id_r = 0, sound_command_r = 0, sound_state_r = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_reset_resume_tag(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource,
					  &pid_r, &alloc_handle_r, &cmd_handle_r, &request_id_r, &sound_command_r, &sound_state_r);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM reset resume tag success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(iiiiii)", pid_r, alloc_handle_r, cmd_handle_r,
							request_id_r, sound_command_r, sound_state_r ));
	} else {
		debug_error("ASM reset resume tag failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM reset resume tag failed");
	}

	debug_fleave();
}

static void handle_method_asm_dump(GDBusMethodInvocation* invocation)
{

	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0, resource = 0;
	GVariant *params = NULL;

	debug_fenter();

	if (!(params = g_dbus_method_invocation_get_parameters(invocation))) {
		debug_error("Parameter for Method is NULL");
		ret = MM_ERROR_SOUND_INTERNAL;
		goto send_reply;
	}

	g_variant_get(params, "(iiiiii)", &pid, &handle, &sound_event, &request_id, &sound_state, &resource);
	ret = __mm_sound_mgr_ipc_asm_dump(_get_sender_pid(invocation), handle, sound_event, request_id, sound_state, resource);

send_reply:
	if (ret == MM_ERROR_NONE) {
		debug_error("ASM dump success");
		g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
	} else {
		debug_error("ASM dump failed, ret : 0x%X", ret);
		g_dbus_method_invocation_return_error(invocation, MM_SOUND_ERROR, ret, "ASM dump failed");
	}

	debug_fleave();
}

/****************************************************************/


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

	for (method_idx = 0; method_idx < METHOD_CALL_MAX; method_idx++) {
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

static void handle_signal_asm_emergent_exit(GVariant* params, gpointer user_data)
{
	int ret = MM_ERROR_NONE;
	int pid = 0, handle = 0, sound_event = 0, request_id = 0, sound_state = 0;

	debug_fenter();

	if (!params) {
		debug_error("Invalid Parameters");
		return;
	}

	g_variant_get(params, "(iiiii)", &pid, &handle, &sound_event, &request_id, &sound_state);
	ret = __mm_sound_mgr_ipc_asm_emergent_exit(pid, handle, sound_event, request_id, sound_state);

	if (ret == MM_ERROR_NONE)
		debug_error("ASM emergent exit, successfully handled");
	else
		debug_error("ASM emergent exit, handle failed, ret : 0x%X", ret);

	debug_fleave();
}

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


static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	guint reg_id;
	guint subs_id;
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

	subs_id = g_dbus_connection_signal_subscribe(connection, NULL, INTERFACE_ASM, "EmergentExit", OBJECT_ASM, \
			 NULL, G_DBUS_SIGNAL_FLAGS_NONE, handle_signal, NULL, NULL );

	if (!subs_id) {
		debug_error ("g_dbus_connection_signal_subscribe() failed ");
		return;
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

/* not for mm-sound client */
int mm_sound_mgr_ipc_dbus_send_signal_freeze (char* command, int pid)
{
	GError *err = NULL;
	GDBusConnection *conn = NULL;
	gboolean ret;

	if (command == NULL || pid <= 0) {
		debug_error ("invalid arguments [%s][%d]", command, pid);
		return -1;
	}

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!conn && err) {
		debug_error ("g_bus_get_sync() error (%s) ", err->message);
		g_error_free (err);
		return -1;
	}

	ret = g_dbus_connection_emit_signal (conn,
				NULL, PROC_DBUS_OBJECT, PROC_DBUS_INTERFACE, PROC_DBUS_METHOD,
				g_variant_new ("(si)", command, pid),
				&err);
	if (!ret && err) {
		debug_error ("g_dbus_connection_emit_signal() error (%s) ", err->message);
		goto error;
	}

	ret = g_dbus_connection_flush_sync(conn, NULL, &err);
	if (!ret && err) {
		debug_error ("g_dbus_connection_flush_sync() error (%s) ", err->message);
		goto error;
	}

	g_object_unref(conn);
	debug_msg ("sending [%s] for pid (%d) success", command, pid);

	return 0;

error:
	g_error_free (err);
	g_object_unref(conn);
	return -1;
}

int __mm_sound_mgr_ipc_dbus_notify_device_connected (mm_sound_device_t *device, gboolean is_connected)
{
	int ret = MM_ERROR_NONE;
	GVariantBuilder builder;
	GVariant* param = NULL;

	debug_log("Send Signal '%s'", signals[SIGNAL_DEVICE_CONNECTED]);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("((iiiis)b)"));
	g_variant_builder_open(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add(&builder, "(iiiis)", device->id, device->type, device->io_direction, device->state, device->name);
	g_variant_builder_close(&builder);
	g_variant_builder_add(&builder, "b", is_connected);
	param = g_variant_builder_end(&builder);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(SIGNAL_DEVICE_CONNECTED, param))!= MM_ERROR_NONE) {
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

	debug_log("Send Signal '%s'", signals[SIGNAL_DEVICE_INFO_CHANGED]);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("((iiiis)i)"));
	g_variant_builder_open(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add(&builder, "(iiiis)", device->id, device->type, device->io_direction, device->state, device->name);
	g_variant_builder_close(&builder);
	g_variant_builder_add(&builder, "i", changed_device_info_type);
	param = g_variant_builder_end(&builder);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(SIGNAL_DEVICE_INFO_CHANGED, param)) != MM_ERROR_NONE) {
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

	debug_log("Send Signal '%s'", signals[SIGNAL_VOLUME_CHANGED]);

	param = g_variant_new("(uu)", vol_type, value);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(SIGNAL_VOLUME_CHANGED, param)) != MM_ERROR_NONE) {
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

	debug_log("Send Signal '%s'", signals[SIGNAL_PLAY_FILE_END]);

	param = g_variant_new("(i)", handle);
	if (param) {
		if ((ret = mm_sound_mgr_ipc_dbus_send_signal(SIGNAL_PLAY_FILE_END, param)) != MM_ERROR_NONE) {
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
#define PA_STREAM_MANAGER_OBJECT_PATH                  "/org/pulseaudio/Ext/StreamManager"
#define PA_STREAM_MANAGER_INTERFACE                    "org.pulseaudio.Ext.StreamManager"
#define PA_STREAM_MANAGER_METHOD_NAME_GET_STREAM_LIST  "GetStreamList"
int __mm_sound_mgr_ipc_dbus_get_stream_list(stream_list_t* stream_list)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;
	GVariant *child = NULL;
	GDBusConnection *conn = NULL;
	GError *err = NULL;
	int i = 0;

	g_type_init();

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



