
#include <stdlib.h>
#include <string.h>

#include "include/mm_sound_socket.h"
#include "include/mm_sound_common.h"
#include "include/mm_sound_msg.h"
#include "include/mm_sound_connection.h"
#include "include/mm_sound_common.h"
#include "include/mm_sound_client.h"
#include "include/mm_sound_proxy.h"

#include <mm_error.h>
#include <mm_debug.h>
#include <glib.h>
#include <gio/gio.h>

typedef struct proxy_cb_data {
    void *user_cb;
    void *user_data;
} proxy_cb_data;

mm_sound_connection *focus_conn, *sound_conn;
GMainLoop *focus_mainloop, *sound_mainloop;
GThread *focus_thread;

static gpointer focus_mainloop_thread_func(gpointer data)
{
    GMainLoop *thread_mainloop = (GThread*) data;
    debug_log(">>> Run threaded mainloop , thread id(%u)\n", (unsigned int)pthread_self());
    g_main_loop_run(thread_mainloop);
    debug_log("<<< threaded mainloop end\n");
    return NULL;
}

int mm_sound_proxy_initialize(void)
{
    GMainContext* focus_context = g_main_context_new();
    focus_mainloop = g_main_loop_new(focus_context, FALSE);
    focus_thread = g_thread_new("focus-thread", focus_mainloop_thread_func, focus_mainloop);
    focus_conn = mm_sound_connection_new(focus_mainloop);
    mm_sound_connection_connect(focus_conn, AUDIO_SERVICE_FOCUS_SERVER);
//    mm_sound_connection_set_state_callback(focus_conn, state_callback);
}

int mm_sound_proxy_finalize(void)
{
    GMainContext *focus_context = g_main_loop_get_context(focus_mainloop);
    mm_sound_connection_disconnect(focus_conn);
    mm_sound_connection_finalize(focus_conn);
    g_main_context_unref(focus_context);

    g_main_loop_quit(focus_mainloop);
    g_thread_join(focus_thread);
    g_main_loop_unref(focus_mainloop);
}

static void focus_event_callback(mm_sound_connection *conn, int event_type, GVariant *params, void *userdata) {
    proxy_cb_data *cb_data = (proxy_cb_data *)userdata;

    if (cb_data == NULL) {
        return;
    }

    if (event_type == SIGNAL_FOCUS_CHANGED) {
        int pid, handle, type, state;
        int cb_ret;
        const char *stream_type = NULL, *name = NULL;
        GVariant *param_ret;

        g_variant_get(params, "(iiii&s&s)", &pid, &handle, &type, &state, &stream_type, &name);
        cb_ret = ((mm_sound_focus_changed_wrapper_cb)(cb_data->user_cb))(pid, handle, type, state, stream_type, name);
        param_ret = g_variant_new("(i)", cb_ret);
        mm_sound_connection_response(conn, event_type, MSG_TYPE_SIGNAL, param_ret);
    } else if (event_type == SIGNAL_FOCUS_WATCH) {
    }

    return;
}

static int sound_event_callback(int event_type, GVariant *params, void *userdata) {
    proxy_cb_data *cb_data = (proxy_cb_data *)userdata;

    if (cb_data == NULL) {
        return -1;
    }

    if (event_type == SIGNAL_VOLUME_CHANGED) {
    } else if (event_type == SIGNAL_DEVICE_CONNECTED) {
    }

    return 0;
}

int mm_sound_proxy_test(int a, int b, int *_sum)
{
	GVariant *params = NULL, *result = NULL;
    int sum = 0;

	params = g_variant_new("(ii)", a, b);
    mm_sound_connection_request(focus_conn, METHOD_CALL_TEST, params, &result);
    g_variant_get(result, "(i)", &sum);

    *_sum = sum;

    return 0;
}

int mm_sound_proxy_add_focus_callback(int instance, int handle, mm_sound_focus_changed_wrapper_cb callback, unsigned *callback_id, void *userdata)
{
	GVariant* params = NULL;
    proxy_cb_data *cb_data = NULL;

    cb_data = g_malloc(sizeof(cb_data));
    cb_data->user_cb = callback;
    cb_data->user_data = userdata;
	params = g_variant_new("(ii)", instance, handle);
    mm_sound_connection_add_event_callback(focus_conn, SIGNAL_FOCUS_CHANGED, focus_event_callback, params, g_free, callback_id, cb_data);

    return 0;
}

int mm_sound_proxy_remove_focus_callback(unsigned callback_id) {
    mm_sound_connection_remove_event_callback(focus_conn, callback_id);
}

int mm_sound_proxy_add_focus_watch_callback(mm_sound_focus_changed_watch_wrapper_cb callback, void *user_data)
{
}
