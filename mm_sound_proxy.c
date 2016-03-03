#include <glib.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "include/mm_sound_proxy.h"
#include "include/mm_sound_common.h"
#include "include/mm_sound_dbus.h"
#include "include/mm_sound_intf.h"

#ifdef USE_SECURITY
#include <security-server.h>
#define COOKIE_SIZE 20
#endif


struct callback_data {
	void *user_cb;
	void *user_data;
	void *extra_data;
	mm_sound_proxy_userdata_free free_func;
	void *extra_data_free_func;
};

#define GET_CB_DATA(_cb_data, _func, _userdata, _freefunc, _extradata, _extradatafreefunc) \
	do { \
		_cb_data = (struct callback_data*) g_malloc0(sizeof(struct callback_data)); \
		_cb_data->user_cb = _func; \
		_cb_data->user_data = _userdata; \
		_cb_data->free_func = _freefunc; \
		_cb_data->extra_data = _extradata; \
		_cb_data->extra_data_free_func = _extradatafreefunc; \
	} while (0)

/* subscribe is true when add callback,
 * false when remove callback */
static int _notify_subscription(audio_event_t event, unsigned subs_id, gboolean subscribe)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL;
	const char *event_name = NULL;

	debug_fenter();

	if((ret = mm_sound_dbus_get_event_name(event, &event_name) != MM_ERROR_NONE)) {
		debug_error("Failed to get event name");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if(!(params = g_variant_new("(sub)", event_name, subs_id, subscribe))) {
		debug_error("Construct Param failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if ((ret = mm_sound_dbus_emit_signal(AUDIO_PROVIDER_AUDIO_CLIENT, AUDIO_EVENT_CLIENT_SUBSCRIBED, params))) {
		debug_error("dbus send signal for client subscribed failed");
	}

	debug_fleave();
	return ret;
}

static int _notify_signal_handled(audio_event_t event, unsigned event_id, unsigned subs_id, GVariant *signal_params)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL;
	const char *event_name = NULL;

	debug_fenter();

	if((ret = mm_sound_dbus_get_event_name(event, &event_name) != MM_ERROR_NONE)) {
		debug_error("Failed to get event name");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if(!(params = g_variant_new("(usuv)", event_id, event_name, subs_id, signal_params))) {
		debug_error("Construct Param failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if ((ret = mm_sound_dbus_emit_signal(AUDIO_PROVIDER_AUDIO_CLIENT, AUDIO_EVENT_CLIENT_HANDLED, params))) {
		debug_error("dbus send signal for client handled failed");
	}

	debug_fleave();
	return ret;
}

/* This callback unmarshall general-formed paramters to subject specific parameters,
 * and call proper callback */
static void dbus_callback(audio_event_t event, GVariant *params, void *userdata)
{
	struct callback_data *cb_data  = (struct callback_data*) userdata;

	if (event == AUDIO_EVENT_VOLUME_CHANGED) {
		char *volume_type_str = NULL, *direction = NULL;
		unsigned volume_level;

		g_variant_get(params, "(&s&su)", &direction, &volume_type_str, &volume_level);
		((mm_sound_volume_changed_wrapper_cb)(cb_data->user_cb))(direction, volume_type_str, volume_level, cb_data->user_data);
	} else if (event == AUDIO_EVENT_DEVICE_CONNECTED) {
		const char *name = NULL, *device_type = NULL;
		gboolean is_connected = FALSE;
		int device_id, io_direction, state;
		unsigned event_id, *subs_id;

		g_variant_get(params, "((ui&sii&s)b)", &event_id, &device_id, &device_type, &io_direction,
					&state, &name, &is_connected);
		((mm_sound_device_connected_wrapper_cb)(cb_data->user_cb))(device_id, device_type, io_direction, state, name, is_connected, cb_data->user_data);
		subs_id = (unsigned*) cb_data->extra_data;
		_notify_signal_handled(event, event_id, *subs_id, g_variant_new("(ib)", device_id, is_connected));
	} else if (event == AUDIO_EVENT_DEVICE_INFO_CHANGED) {
		const char *name = NULL, *device_type = NULL;
		int changed_device_info_type = 0;
		int device_id, io_direction, state;

		g_variant_get(params, "((i&sii&s)i)", &device_id, &device_type, &io_direction,
					&state, &name, &changed_device_info_type);
		((mm_sound_device_info_changed_wrapper_cb)(cb_data->user_cb))(device_id, device_type, io_direction, state, name, changed_device_info_type, cb_data->user_data);
	} else if (event == AUDIO_EVENT_FOCUS_CHANGED) {
	} else if (event == AUDIO_EVENT_FOCUS_WATCH) {
	} else if (event == AUDIO_EVENT_TEST) {
		int test_var = 0;
		g_variant_get(params, "(i)", &test_var);
		((mm_sound_test_cb)(cb_data->user_cb))(test_var, cb_data->user_data);
	} else if (event == AUDIO_EVENT_PLAY_FILE_END) {
		int ended_handle = 0;
		g_variant_get(params, "(i)", &ended_handle);
		((mm_sound_stop_callback_wrapper_func)(cb_data->user_cb))(ended_handle, cb_data->user_data);
	}
}

static void simple_callback_data_free_func(void *data)
{
	struct callback_data *cb_data = (struct callback_data*) data;

	if (cb_data->free_func)
		cb_data->free_func(cb_data->user_data);

	g_free(cb_data);
}

int mm_sound_proxy_add_test_callback(mm_sound_test_cb func, void *userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id)
{
	int ret = MM_ERROR_NONE;
	struct callback_data *cb_data;

	debug_fenter();

	GET_CB_DATA(cb_data, func, userdata, freefunc, NULL, NULL);

	if ((ret = mm_sound_dbus_signal_subscribe_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_EVENT_TEST, dbus_callback, cb_data, simple_callback_data_free_func, subs_id)) != MM_ERROR_NONE) {
		debug_error("add test callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_remove_test_callback(unsigned subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = mm_sound_dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove test callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_test(int a, int b, int *get)
{
	int ret = MM_ERROR_NONE;
	int reply = 0;
	GVariant *params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(ii)", a, b);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_TEST, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_get_current_connected_device_list(int device_flags, GList** device_list)
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_DEVICE_MANAGER, AUDIO_METHOD_GET_CONNECTED_DEVICE_LIST, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_add_device_connected_callback(int device_flags, mm_sound_device_connected_wrapper_cb func, void *userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id)
{
	int ret = MM_ERROR_NONE;
	struct callback_data *cb_data;
	int *_subs_id;

	debug_fenter();

	_subs_id = (int*) g_malloc0(sizeof(int));
	GET_CB_DATA(cb_data, func, userdata, freefunc, (void*) _subs_id, g_free);

	if ((ret = mm_sound_dbus_signal_subscribe_to(AUDIO_PROVIDER_DEVICE_MANAGER, AUDIO_EVENT_DEVICE_CONNECTED, dbus_callback, cb_data, simple_callback_data_free_func, _subs_id)) != MM_ERROR_NONE) {
		debug_error("add device connected callback failed");
		goto finish;
	}

	if ((ret = _notify_subscription(AUDIO_EVENT_DEVICE_CONNECTED, *_subs_id, TRUE)) != MM_ERROR_NONE) {
		debug_error("failed to notify subscription of device connected event");
		goto finish;
	}

	*subs_id = *_subs_id;

finish:
	debug_fleave();
	return ret;
}

int mm_sound_proxy_remove_device_connected_callback(unsigned subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = mm_sound_dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove device connected callback failed");
		goto finish;
	}

	if ((ret = _notify_subscription(AUDIO_EVENT_DEVICE_CONNECTED, subs_id, FALSE)) != MM_ERROR_NONE)
		debug_error("failed to notify unsubscription of device connected event");

finish:
	debug_fleave();
	return ret;
}

int mm_sound_proxy_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_wrapper_cb func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id)
{
	int ret = MM_ERROR_NONE;
	struct callback_data *cb_data;

	debug_fenter();

	GET_CB_DATA(cb_data, func, userdata, freefunc, NULL, NULL);

	if ((ret = mm_sound_dbus_signal_subscribe_to(AUDIO_PROVIDER_DEVICE_MANAGER, AUDIO_EVENT_DEVICE_INFO_CHANGED, dbus_callback, cb_data, simple_callback_data_free_func, subs_id)) != MM_ERROR_NONE) {
		debug_error("Add device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_remove_device_info_changed_callback(unsigned subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = mm_sound_dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("remove device info changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_set_volume_by_type(const char *volume_type, const unsigned volume_level)
{
	int ret = MM_ERROR_NONE;
	char *reply = NULL, *direction = "out";
	GVariant *params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(ssu)", direction, volume_type, volume_level);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_STREAM_MANAGER, AUDIO_METHOD_SET_VOLUME_LEVEL, params, &result)) != MM_ERROR_NONE) {
			debug_error("dbus set volume by type failed");
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

int mm_sound_proxy_add_volume_changed_callback(mm_sound_volume_changed_wrapper_cb func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id)
{
	int ret = MM_ERROR_NONE;
	struct callback_data *cb_data;

	debug_fenter();

	GET_CB_DATA(cb_data, func, userdata, freefunc, NULL, NULL);

	if ((ret = mm_sound_dbus_signal_subscribe_to(AUDIO_PROVIDER_STREAM_MANAGER, AUDIO_EVENT_VOLUME_CHANGED, dbus_callback, cb_data, simple_callback_data_free_func, subs_id)) != MM_ERROR_NONE) {
		debug_error("Add Volume changed callback failed");
	}

	debug_fleave();

	return ret;
}

int mm_sound_proxy_remove_volume_changed_callback(unsigned subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = mm_sound_dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("Remove Volume changed callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_play_tone(int tone, int repeat, int volume, int volume_config,
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_PLAY_DTMF, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_play_tone_with_stream_info(int client_pid, int tone, char *stream_type, int stream_index, int volume, int repeat, int *codechandle)
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_PLAY_DTMF_WITH_STREAM_INFO, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_play_sound(const char* filename, int tone, int repeat, int volume, int volume_config,
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_PLAY_FILE_START, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_play_sound_with_stream_info(const char* filename, int repeat, int volume,
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_PLAY_FILE_START_WITH_STREAM_INFO, params, &result)) != MM_ERROR_NONE) {
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


int mm_sound_proxy_stop_sound(int handle)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_PLAY_FILE_STOP, g_variant_new("(i)", handle), &result)) != MM_ERROR_NONE) {
		debug_error("dbus stop file playing failed");
		goto cleanup;
	}

cleanup:
	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_proxy_clear_focus(int pid)
{
	int ret = MM_ERROR_NONE;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_METHOD_CLEAR_FOCUS, g_variant_new("(i)", pid), &result)) != MM_ERROR_NONE) {
		debug_error("dbus clear focus failed");
	}

	if (result)
		g_variant_unref(result);

	debug_fleave();
	return ret;
}

int mm_sound_proxy_add_play_sound_end_callback(mm_sound_stop_callback_wrapper_func func, void* userdata, mm_sound_proxy_userdata_free freefunc, unsigned *subs_id)
{
	int ret = MM_ERROR_NONE;
	struct callback_data *cb_data;

	debug_fenter();

	GET_CB_DATA(cb_data, func, userdata, freefunc, NULL, NULL);

	if ((ret = mm_sound_dbus_signal_subscribe_to(AUDIO_PROVIDER_SOUND_SERVER, AUDIO_EVENT_PLAY_FILE_END, dbus_callback, cb_data, simple_callback_data_free_func, subs_id)) != MM_ERROR_NONE) {
		debug_error("add play sound end callback failed");
	}

	debug_fleave();

	return ret;
}

int mm_sound_proxy_remove_play_sound_end_callback(unsigned subs_id)
{
	int ret = MM_ERROR_NONE;
	debug_fenter();

	if ((ret = mm_sound_dbus_signal_unsubscribe(subs_id)) != MM_ERROR_NONE) {
		debug_error("Remove Play File End callback failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_emergent_exit(int exit_pid)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL;

	debug_fenter();

	params = g_variant_new("(i)", exit_pid);
	if (params) {
	    if ((ret = mm_sound_dbus_emit_signal(AUDIO_PROVIDER_AUDIO_CLIENT, AUDIO_EVENT_EMERGENT_EXIT, params)) != MM_ERROR_NONE) {
			debug_error("dbus emergent exit failed");
			goto cleanup;
		}
	} else {
		debug_error("Construct Param for emergent exit signal failed");
		ret = MM_ERROR_SOUND_INTERNAL;
	}

cleanup:

	debug_fleave();
	return ret;
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

int mm_sound_proxy_get_unique_id(int *id)
{
	int ret = MM_ERROR_NONE;
	int res = 0;
	GVariant *result = NULL;

	debug_fenter();

	if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_GET_UNIQUE_ID, NULL, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_register_focus(int id, int instance, const char *stream_type, mm_sound_focus_changed_cb callback, bool is_for_session, void* userdata)
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_REGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_unregister_focus(int instance, int id, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", instance, id, is_for_session);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_UNREGISTER_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_set_foucs_reacquisition(int instance, int id, bool reacquisition)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", instance, id, reacquisition);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_SET_FOCUS_REACQUISITION, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_get_acquired_focus_stream_type(int focus_type, char **stream_type, char **additional_info)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	if (!(params = g_variant_new("(i)", focus_type))) {
		debug_error("Construct Param for method call failed");
		return MM_ERROR_SOUND_INTERNAL;
	}

	if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_GET_ACQUIRED_FOCUS_STREAM_TYPE, params, &result)) == MM_ERROR_NONE) {
		if (result) {
			g_variant_get(result, "(ss)", stream_type, additional_info);
			g_variant_unref(result);
		}
	} else {
		debug_error("dbus get stream type of acquired focus failed");
	}

	debug_fleave();
	return ret;
}

int mm_sound_proxy_acquire_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iiisb)", instance, id, type, option, is_for_session);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_ACQUIRE_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_release_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iiisb)", instance, id, type, option, is_for_session);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_RELEASE_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_set_focus_watch_callback(int instance, int handle, mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, bool is_for_session, void* userdata)
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
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_WATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

int mm_sound_proxy_unset_focus_watch_callback(int focus_tid, int handle, bool is_for_session)
{
	int ret = MM_ERROR_NONE;
	GVariant* params = NULL, *result = NULL;

	debug_fenter();

	params = g_variant_new("(iib)", focus_tid, handle, is_for_session);
	if (params) {
		if ((ret = mm_sound_dbus_method_call_to(AUDIO_PROVIDER_FOCUS_SERVER, AUDIO_METHOD_UNWATCH_FOCUS, params, &result)) != MM_ERROR_NONE) {
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

#endif /* USE_FOCUS */
/*------------------------------------------ FOCUS --------------------------------------------------*/

int mm_sound_proxy_initialize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_fleave();

	return ret;
}

int mm_sound_proxy_finalize(void)
{
	int ret = MM_ERROR_NONE;

	debug_fenter();
	debug_fleave();

	return ret;
}
