/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungbae Shin <seungbae.shin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <pthread.h>

#include <errno.h>
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
#include "../include/mm_sound_socket.h"

#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixinputstream.h>
#include <glib-object.h>

#define BACKLOG 5
#define SOCKET_PATH_BASE "/var/run/audio/"
#define SOCKET_PATH_MAX 30

#define PACKET_LEN_MAX 128

#define SOCKET_TEST
#define CLIENT_ABSTRACT_SOCKET

struct mm_sound_packet {
    size_t length;
    void *msg;
};

struct mm_sound_socket_client{
    GSocketClient *client;
};

struct mm_sound_socket_server {
    GMainLoop *main_loop;
    GMainContext *main_context;
    GThread *server_thread;
    GSocketService *service;
//    GSocketConnection *connections;
    GHashTable *connections;
    mm_sound_socket_connected_callback conn_cb;
    void *conn_data;
};

struct mm_sound_socket_connection {
    GSocketConnection *conn;
    GMainLoop *main_loop;
    GMainContext *main_context;
    GThread *conn_thread;
    GSource *in_poll_source;
    mm_sound_socket_packet_callback packet_callback;
    void *packet_callback_data;
    socket_connection_status_t status;
};

static char* _get_path_from_id(const char *socket_id) {
    char *socket_path = NULL;

    if (!socket_id)
        return NULL;

    socket_path = g_strdup_printf("%s%s", SOCKET_PATH_BASE, socket_id);

    return socket_path;
}

static const char* _get_id_from_path(const char *path) {
    char *socket_id = NULL;

    if (!path)
        return NULL;

    if (g_str_has_prefix(path, SOCKET_PATH_BASE) == FALSE) {
        debug_error("wrong path : %s", path);
        return NULL;
    }

    if (g_strcmp0(path, SOCKET_PATH_BASE) == 0) {
        debug_error("same path with base : %s", path);
        return NULL;
    }

    socket_id = g_strdup(path + sizeof(SOCKET_PATH_BASE) -1);

    return socket_id;
}

static gpointer socket_connection_thread_func(gpointer data)
{
    mm_sound_socket_connection *socket_conn = (mm_sound_socket_connection*) data;
    debug_fenter();
    if (!socket_conn || !socket_conn->main_loop || !socket_conn->main_context) {
        debug_error("Invalid Parameter");
        goto FINISH;
    }

    g_main_loop_run(socket_conn->main_loop);

FINISH:
    debug_fleave();
    return NULL;
}

static gboolean poll_input_callback_func(GPollableInputStream *in, gpointer userdata)
{
    GError *error = NULL;
    int msg_length;
    gssize nread;
    char buf[PACKET_LEN_MAX] = {0};
    mm_sound_socket_connection *socket_conn = (mm_sound_socket_connection *) userdata;

    debug_fenter();
    // first, get size of message to read.
    nread = g_pollable_input_stream_read_nonblocking (in, &msg_length, sizeof(int), NULL, &error);
//    buf = g_malloc0(msg_length);
    // get real message.
    nread = g_pollable_input_stream_read_nonblocking (in, buf, msg_length, NULL, &error);

    if (socket_conn->packet_callback) {
        (socket_conn->packet_callback)(socket_conn, buf, msg_length, socket_conn->packet_callback_data);
    }
    debug_fleave();

    return G_SOURCE_CONTINUE;
}

static int socket_connection_set_input_cb(mm_sound_socket_connection *socket_conn, GSourceFunc input_cb)
{
    int ret = MM_ERROR_NONE;
    GIOStream *io_stream = NULL;
    GInputStream *in_stream = NULL;
    GUnixInputStream *unix_in_stream = NULL;
    GSource *in_poll_source = NULL;

    debug_fenter();

    if (!socket_conn || !socket_conn->conn || !input_cb) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    io_stream = G_IO_STREAM(socket_conn->conn);
    in_stream = g_io_stream_get_input_stream(io_stream);
    if (G_IS_UNIX_INPUT_STREAM(in_stream)) {
        debug_log("[DEBUG_LOG] unix input stream");
    } else {
        debug_log("[DEBUG_LOG] not unix input stream");
    }
    unix_in_stream = G_UNIX_INPUT_STREAM(in_stream);

    if ((in_poll_source = g_pollable_input_stream_create_source(G_POLLABLE_INPUT_STREAM(unix_in_stream), NULL)) == NULL) {
        debug_error("create source fore input stream failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    g_source_set_callback(in_poll_source, input_cb, socket_conn, g_free);
    if (g_source_attach(in_poll_source, socket_conn->main_context) <= 0) {
        debug_error("in_poll_source attach failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }
    socket_conn->in_poll_source = in_poll_source;
//    g_source_unref (in_poll_source);

FINISH:

    debug_fleave();
    return ret;
}

static int socket_connection_new(GSocketConnection *conn, mm_sound_socket_connection **_socket_conn)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_connection *socket_conn;

    debug_fenter();

    socket_conn = g_malloc0(sizeof(mm_sound_socket_connection));
    socket_conn->status = SOCKET_CONNECTION_STATUS_NEW;
    socket_conn->conn = conn;
    socket_conn->main_context = g_main_context_new();
    socket_conn->main_loop = g_main_loop_new(socket_conn->main_context, FALSE);
    socket_conn->in_poll_source = NULL;
    socket_conn->conn_thread = g_thread_new("socket_connection_thread", socket_connection_thread_func, socket_conn);

    socket_connection_set_input_cb(socket_conn, (GSourceFunc) poll_input_callback_func);

    *_socket_conn = socket_conn;

    debug_fleave();

    return ret;
}
/* send message through connected socket connection */
static int socket_connection_send(mm_sound_socket_connection *socket_conn, const char *msg, size_t len)
{
    int ret = MM_ERROR_NONE;
    GIOStream *io_stream = NULL;
    GOutputStream *out_stream = NULL;
    GError *error = NULL;

    debug_fenter();
    if (!socket_conn|| !socket_conn->conn || !msg) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_socket_connection_is_connected(socket_conn->conn) == TRUE) {
        debug_error("Socket Connection is closed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    io_stream = G_IO_STREAM(socket_conn->conn);
    out_stream = g_io_stream_get_output_stream(io_stream);
    if (g_output_stream_write(out_stream, msg, len, NULL, &error) == -1) {
        debug_error("output stream write failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();
    return ret;
}

/* recv message through connected socket connection */
static int socket_connection_recv(mm_sound_socket_connection *socket_conn, void *msg, size_t *len)
{
    int ret = MM_ERROR_NONE;
    size_t packet_len = 0;
    GIOStream *io_stream = NULL;
    GInputStream *in_stream = NULL;
    GError *error = NULL;

    debug_fenter();
    if (!socket_conn|| !socket_conn->conn || !msg || !len) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_socket_connection_is_connected(socket_conn->conn) == TRUE) {
        debug_error("Socket Connection is closed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    io_stream = G_IO_STREAM(socket_conn->conn);
    in_stream = g_io_stream_get_input_stream(io_stream);
    if (g_input_stream_read(in_stream, &packet_len, sizeof(size_t), NULL, &error) == -1) {
        debug_error("input stream read packet length failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_input_stream_read(in_stream, msg, packet_len, NULL, &error) == -1) {
        debug_error("input stream read packet length failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();
    return ret;
}

/*
static const char* get_reponse_var_type(int request_type)
{
    GVariantType *var_type = NULL;
    var_type = g_variant_type_new("i");

    return var_type;
}

static int socket_connection_request(mm_sound_socket_connection *socket_conn, int request_type, GVariant *params)
{
    int ret = MM_ERROR_NONE;
    int result_len = 0;
    gpointer result_data = NULL;
    GVariant *result = NULL;
    GVariantType *result_type = NULL;

    debug_fenter();
    socket_connection_send(socket_conn, msg, len);
    socket_connection_recv(socket_conn, &result_len, sizeof(result_len));
    result_data = g_malloc0(result_len);
    socket_connection_recv(socket_conn, result_data, result_len);
    result_type = get_response_var_type(request_type);
    g_variant_new_from_data(result_type, result_data, result_len, TRUE, g_free, result_data);
    debug_fleave();

    return ret;
}

static int socket_connection_add_handler(mm_sound_socket_connection *socket_conn, int request_type, void *handler_func)
{
}

static int socket_connection_add_callback(mm_sound_socket_connection *socket_conn, int event_type, void *callback_func)
{
}

static int socket_connection_notify(mm_sound_socket_connection *socket_conn, int event_type, GVariant *params)
{
}

static int socket_connection_notify_with_return(mm_sound_socket_connection *socket_conn, int event_type, GVariant *params)
{
}
*/

static int socket_connection_remove_input_cb(mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;
    guint source_id = 0;

    debug_fenter();

    source_id = g_source_get_id(socket_conn->in_poll_source);
    g_source_remove(source_id);
    g_source_destroy(socket_conn->in_poll_source);
    socket_conn->in_poll_source = NULL;

    debug_fleave();

    return ret;
}
/* close socket connection */
static int socket_connection_close(mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;
    GIOStream *io_stream = NULL;
    GError *error = NULL;
    debug_fenter();

    if (!socket_conn) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    io_stream = G_IO_STREAM(socket_conn->conn);

    if (g_io_stream_close(io_stream, NULL, &error)) {
        debug_error("io stream close failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    socket_conn->status = SOCKET_CONNECTION_STATUS_DISCONNECTED;

FINISH:
    debug_fleave();

    return ret;
}

/* destroy socket connection object */
static void socket_connection_destroy(mm_sound_socket_connection *socket_conn)
{
    debug_fenter();

    if (!socket_conn) {
        debug_warning("Null Parameter");
        return;
    }
    socket_conn->status = SOCKET_CONNECTION_STATUS_DESTROYED;

    if (socket_conn->conn)
        g_object_unref(socket_conn->conn);
    socket_conn->conn = NULL;

    if (socket_conn->main_loop)
        g_main_loop_quit(socket_conn->main_loop);
    if (socket_conn->conn_thread)
        g_thread_join(socket_conn->conn_thread);
    if (socket_conn->main_loop)
        g_main_loop_unref(socket_conn->main_loop);
    if (socket_conn->main_context)
        g_main_context_unref(socket_conn->main_context);

    socket_conn->main_loop = NULL;
    socket_conn->conn_thread = NULL;
    socket_conn->main_context = NULL;

    g_free(socket_conn);

    debug_fleave();
}

static void _free_socket_client(mm_sound_socket_client *socket_client)
{
    if (!socket_client)
        return;

    g_object_unref(socket_client->client);
    g_free(socket_client);
}

/*
   Create socket client object.
   If socket_path is not null, will be used for bind.
*/
static int socket_client_new(const char *socket_path, mm_sound_socket_client **_socket_client)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_client *socket_client = NULL;
    GSocketAddress *gaddr = NULL;
    GUnixSocketAddressType gaddr_type = G_UNIX_SOCKET_ADDRESS_PATH;

    debug_fenter();

    debug_log("socket client new with path %s", socket_path);

    if ((socket_client = g_malloc0(sizeof(mm_sound_socket_client))) == NULL) {
        debug_error("allocate socket client failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_client->client = g_socket_client_new()) == NULL) {
        debug_error("socket client new failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    g_socket_client_set_family(socket_client->client, G_SOCKET_FAMILY_UNIX);
    g_socket_client_set_protocol(socket_client->client, G_SOCKET_PROTOCOL_DEFAULT);
    g_socket_client_set_socket_type(socket_client->client, G_SOCKET_TYPE_STREAM);

    if (socket_path) {
#ifdef CLIENT_ABSTRACT_SOCKET
        if (g_unix_socket_address_abstract_names_supported() == TRUE)
            gaddr_type = G_UNIX_SOCKET_ADDRESS_ABSTRACT;
        else
            gaddr_type = G_UNIX_SOCKET_ADDRESS_PATH;
#else
        gaddr_type = G_UNIX_SOCKET_ADDRESS_PATH;
#endif
        debug_log("GUnixSocketAddressType for client socket : %d", gaddr_type);

        if ((gaddr = g_unix_socket_address_new_with_type(socket_path, -1, gaddr_type)) == NULL) {
            debug_error("socket address new failed");
            ret = MM_ERROR_SOUND_INTERNAL;
            goto FINISH;
        }
        g_socket_client_set_local_address(socket_client->client, gaddr);
    }
    *_socket_client = socket_client;

FINISH:
    if (ret != MM_ERROR_NONE) {
        _free_socket_client(socket_client);
    }
    g_object_unref(gaddr);
    debug_fleave();

    return ret;
}

/* Destroy socket client object */
static int socket_client_destroy(mm_sound_socket_client *socket_client)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_client) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    _free_socket_client(socket_client);

FINISH:
    debug_fleave();

    return ret;
}

/*
   Connect to socket which of address is server_path.
   When connected, socket_connection will be given, which could be used to real communiction (send/receive message).
*/
static int socket_client_connect(mm_sound_socket_client *socket_client, const char *server_path, mm_sound_socket_connection **_socket_conn)
{
    int ret = MM_ERROR_NONE;
    GSocketAddress *gaddr = NULL;
    GSocketConnectable *connectable = NULL;
    GSocketConnection *conn;
    mm_sound_socket_connection *socket_conn = NULL;
    GError *error = NULL;

    debug_fenter();
    if (!socket_client || !server_path || !_socket_conn) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    gaddr = g_unix_socket_address_new(server_path);
    connectable = G_SOCKET_CONNECTABLE(gaddr);
    if ((conn = g_socket_client_connect(socket_client->client, connectable, NULL, &error)) == NULL) {
        debug_error("socket client connect failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_conn = g_malloc0(sizeof(mm_sound_socket_connection *))) == NULL) {
        debug_error("allocate for socket connection failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_new(conn, &socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket connection new failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    *_socket_conn = socket_conn;

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_socket_client_new(const char *socket_id, mm_sound_socket_client **_socket_client)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_client *socket_client = NULL;
    char *socket_path = NULL;

    debug_fenter();

    if (!_socket_client) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    socket_path = _get_path_from_id(socket_id);

    if ((ret = socket_client_new(socket_path, &socket_client)) != MM_ERROR_NONE) {
        debug_error("socket_client_new failed");
        goto FINISH;
    }

    *_socket_client = socket_client;

FINISH:

    g_free(socket_path);
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_client_destroy(mm_sound_socket_client *socket_client)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_client) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_client_destroy(socket_client)) != MM_ERROR_NONE) {
        debug_error("socket_client_destroy failed");
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_client_connect(mm_sound_socket_client *socket_client, const char *server_socket_path, mm_sound_socket_connection **_socket_conn)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_connection *socket_conn = NULL;

    debug_fenter();

    if (!socket_client) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_client_connect(socket_client, server_socket_path, &socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket_client_connect failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    *_socket_conn = socket_conn;

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
void mm_sound_socket_connection_destroy(mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_conn) {
        debug_error("Invalid Parameter");
        goto FINISH;
    }

    if ((ret = socket_connection_close(socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket_connection_close failed");
        goto FINISH;
    }

    socket_connection_destroy(socket_conn);

FINISH:
    debug_fleave();
}

// TODO : send packet
EXPORT_API
int mm_sound_socket_connection_send(mm_sound_socket_connection *socket_conn, const void *msg, size_t len)
{
    int ret = MM_ERROR_NONE;
    mm_sound_packet packet;

    debug_fenter();

    if (!socket_conn || !msg) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_send(socket_conn, msg, len)) != MM_ERROR_NONE) {
        debug_error("socket_connection_send failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

// TODO : get packet
EXPORT_API
int mm_sound_socket_connection_recv(mm_sound_socket_connection *socket_conn, void *msg, size_t *len)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_conn || !msg) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_recv(socket_conn, msg, len)) != MM_ERROR_NONE) {
        debug_error("socket_connection_recv failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_connection_set_packet_callback(mm_sound_socket_connection *socket_conn, mm_sound_socket_packet_callback packet_cb, void *userdata)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();
    socket_conn->packet_callback = packet_cb;
    socket_conn->packet_callback_data = userdata;
    /*
    if ((ret = socket_connection_set_input_cb(socket_conn, input_cb, userdata)) != MM_ERROR_NONE) {
        debug_error("socket_connection_set_input_cb failed");
        ret = MM_ERROR_SOUND_INTERNAL;
    }
    */
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_connection_unset_packet_callback(mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();
    socket_conn->packet_callback = NULL;
    /*
    if ((ret = socket_connection_remove_input_cb(socket_conn) != MM_ERROR_NONE) {
        debug_error("socket_connection_remove_input_cb failed");
        ret = MM_ERROR_SOUND_INTERNAL;
    }
    */
    debug_fleave();

    return ret;
}

static void _free_socket_server(mm_sound_socket_server *socket_server)
{
    if (!socket_server)
        return;

    g_hash_table_destroy(socket_server->connections);
    g_object_unref(socket_server->service);
    g_free(socket_server);
}

static int _socket_server_get_connection_from_id(mm_sound_socket_server *socket_server, const char *target_id, mm_sound_socket_connection **_socket_conn)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_connection *socket_conn = NULL;

    debug_fenter();

    if (!socket_server || !socket_server->connections || !target_id || !_socket_conn) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_conn = g_hash_table_lookup(socket_server->connections, target_id)) == NULL) {
        debug_error("No Socket for '%s'", target_id);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    *_socket_conn = socket_conn;

FINISH:
    debug_fleave();

    return ret;
}

/* Store socket connections with given key (target_id) */
static int _socket_server_store_connection(mm_sound_socket_server *socket_server, const char *target_id, mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_server || !socket_server->connections || !target_id || !socket_conn) {
        debug_error("Invalid Parameter");
        if (!socket_server)
            debug_error("[DEBUG_LOG] socket_server null");
        if (!socket_server->connections)
            debug_error("[DEBUG_LOG] socket_server->connections null");
        if (!target_id)
            debug_error("[DEBUG_LOG] target_id null");
        if (!socket_conn)
            debug_error("[DEBUG_LOG] socket_conn null");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_hash_table_insert(socket_server->connections, (gpointer) target_id, socket_conn) == FALSE) {
        debug_warning("same target id exists");
    }
FINISH:
    debug_fleave();

    return ret;
}

/* Called when new socket is accepted */
static gboolean connected_callback(GSocketService *service, GSocketConnection *conn, GObject *source_object, gpointer userdata)
{
    int ret = MM_ERROR_SOUND_INTERNAL;
    GError *error = NULL;
    GSocketAddress *gaddr = NULL;
    const char *remote_path, *remote_id;
    mm_sound_socket_server *socket_server = (mm_sound_socket_server*) userdata;
    mm_sound_socket_connection *socket_conn;

    debug_fenter();

    debug_log("Socket Connected");

    if ((gaddr = g_socket_connection_get_remote_address(conn, &error)) == NULL) {
        debug_error("get remote address failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((remote_path = g_unix_socket_address_get_path(G_UNIX_SOCKET_ADDRESS(gaddr))) == NULL) {
        debug_error("get remote path failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    debug_log("Socket Connected path : %s", remote_path);

    if ((remote_id = _get_id_from_path(remote_path)) == NULL) {
        debug_error("get remote id failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    debug_log("Socket Connected ID: %s", remote_id);

    if ((socket_conn = g_malloc0(sizeof(mm_sound_socket_connection))) == NULL) {
        debug_error("allocate for socket connection failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_new(conn, &socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket connection new failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = _socket_server_store_connection(socket_server, remote_id, socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket_store failed for '%s'", remote_id);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    socket_conn->status = SOCKET_CONNECTION_STATUS_CONNECTED;

    if (socket_server->conn_cb) {
        debug_log("Call connected callback");
        ((mm_sound_socket_connected_callback)(socket_server->conn_cb)) (socket_conn, remote_id, socket_server->conn_data);
    }

FINISH:
    if (ret != MM_ERROR_NONE) {
        if (gaddr)
            g_object_unref(gaddr);
    }

    debug_fleave();

    return ret;
}

static int _unlink_socket_if_exists(const char *socket_path)
{
    if (!socket_path)
        return -1;

    if (access(socket_path, F_OK) == 0) {
        if (unlink(socket_path) == -1) {
            debug_error("failed to unlink socket '%s' : %s", socket_path, strerror(errno));
            return -1;
        }
    }

    return 0;
}

static int socket_server_new(const char *socket_path, mm_sound_socket_server **_socket_server)
{
    int ret = MM_ERROR_NONE;
    GError *error = NULL;
    mm_sound_socket_server *socket_server = NULL;
    GSocketAddress *gaddr = NULL;
    GSocket *gsocket = NULL;

    debug_fenter();

    if (!socket_path || !_socket_server) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_server = g_malloc0(sizeof(mm_sound_socket_server))) == NULL) {
        debug_error("allocate mm_sound_socket_server failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_server->service = g_socket_service_new()) == NULL) {
        debug_error("allocate GSocketService failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((gaddr = g_unix_socket_address_new(socket_path)) == NULL) {
        debug_error("socket address new failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((gsocket = g_socket_new(G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, &error)) == NULL) {
        debug_error("socket_new failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (_unlink_socket_if_exists(socket_path) < 0) {
        debug_error("unlink exists socket failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_socket_bind(gsocket, gaddr, TRUE, &error) == FALSE) {
        debug_error("socket_bind failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (g_socket_listen(gsocket, &error) == FALSE) {
        debug_error("socket_listen failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

#if 1
    gint reuseaddr = 0;
    if (g_socket_get_option(gsocket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &error) == FALSE) {
        debug_log("[DEBUG_LOG] get option failed");
    } else {
        debug_log("[DEBUG_LOG] socketoption resuseaddr : %d", reuseaddr);
    }
    debug_log("[DEBUG_LOG] gsocketprotocol : %d", g_socket_get_protocol(gsocket));
    debug_log("[DEBUG_LOG] gsocketfamily : %d", g_socket_get_family(gsocket));
    debug_log("[DEBUG_LOG] gsockettype : %d", g_socket_get_socket_type(gsocket));
#endif

    if (g_socket_listener_add_socket(G_SOCKET_LISTENER(socket_server->service), gsocket, NULL, &error) == FALSE) {
        debug_error("socket_listener_add_address failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

#if 0
    if (g_socket_listener_add_address(G_SOCKET_LISTENER(socket_server->service), gaddr, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL, NULL, &error) == FALSE) {
        debug_error("socket_listener_add_address failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }
#endif

    if ((socket_server->connections = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)socket_connection_destroy)) == NULL) {
        debug_error("allocate hashtable for socket connnections failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    g_signal_connect(socket_server->service, "incoming", G_CALLBACK(connected_callback), socket_server);

    *_socket_server = socket_server;

FINISH:
    g_object_unref(gaddr);
    g_object_unref(gsocket);
    if (ret != MM_ERROR_NONE)
        _free_socket_server(socket_server);
    if (error)
        g_error_free(error);

    debug_fleave();
    return ret;
}

static int _make_socket_base_dir(const char *base_dir_path)
{
    mode_t mode = 0777;
    if (access(base_dir_path, F_OK) == -1) {
        debug_log("There is no directory for audio socket clients, make it %s", base_dir_path);
        if (mkdir(base_dir_path, mode) == -1) {
            debug_error("mkdir for audio socket client failed : %s", strerror(errno));
            return -1;
        }
    } else {
        debug_log("There is already directory for audio socket clients, %s", base_dir_path);
    }

    return 0;
}

static int socket_server_start(mm_sound_socket_server *socket_server)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_server || !socket_server->service) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    _make_socket_base_dir(SOCKET_PATH_BASE);

    g_socket_service_start(socket_server->service);

FINISH:
    debug_fleave();
    return ret;
}

static int socket_server_stop(mm_sound_socket_server *socket_server)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_server || !socket_server->service) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    g_socket_service_stop(socket_server->service);

FINISH:
    debug_fleave();
    return ret;
}

static gboolean _socket_connection_close_destroy(gpointer key, gpointer value, gpointer user_data)
{
    socket_connection_close((mm_sound_socket_connection *)value);
    socket_connection_destroy((mm_sound_socket_connection *)value);
    return TRUE;
}

static void socket_server_destroy(mm_sound_socket_server *socket_server)
{
    debug_fenter();

    g_hash_table_foreach_remove(socket_server->connections, (GHRFunc)_socket_connection_close_destroy ,NULL);
    _free_socket_server(socket_server);

    debug_fleave();
}

EXPORT_API
int mm_sound_socket_server_new(const char *socket_path, mm_sound_socket_server **_socket_server)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_server *socket_server = NULL;

    debug_fenter();

    if (!socket_path || !_socket_server) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }
    if ((ret = socket_server_new(socket_path, &socket_server)) != MM_ERROR_NONE) {
        debug_error("socket server new failed");
        goto FINISH;
    }

    socket_server->main_context = g_main_context_new();
    socket_server->main_loop = g_main_loop_new(socket_server->main_context, FALSE);

    *_socket_server = socket_server;

FINISH:

    if (ret != MM_ERROR_NONE)
        g_free(socket_server);

    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_server_set_connected_callback(mm_sound_socket_server *socket_server, mm_sound_socket_connected_callback conn_cb, void *userdata)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_server || !conn_cb) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    socket_server->conn_cb = conn_cb;
    socket_server->conn_data = userdata;

FINISH:

    debug_fleave();

    return ret;
}

gpointer socket_server_func(gpointer data)
{
    mm_sound_socket_server *socket_server = (mm_sound_socket_server *)data;

    g_main_loop_run(socket_server->main_loop);

    return NULL;
}

EXPORT_API
int mm_sound_socket_server_start(mm_sound_socket_server *socket_server)
{
    int ret = MM_ERROR_NONE;
    debug_fenter();

    if (!socket_server) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_server_start(socket_server)) != MM_ERROR_NONE) {
        debug_error("socket server start failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((socket_server->server_thread = g_thread_new("socket-server-thread", socket_server_func, socket_server)) == NULL) {
        debug_error("socket server thread new failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
void mm_sound_socket_server_stop(mm_sound_socket_server *socket_server)
{
    int ret = MM_ERROR_NONE;
    debug_fenter();

    if (!socket_server) {
        debug_error("Invalid Parameter");
        return;
    }
    socket_server_stop(socket_server);
    g_main_loop_quit(socket_server->main_loop);
    g_thread_join(socket_server->server_thread);

    debug_fleave();
}

EXPORT_API
void mm_sound_socket_server_destroy(mm_sound_socket_server *socket_server)
{
    debug_fenter();

    if (!socket_server) {
        debug_warning("Invalid Parameter");
        return;
    }
    g_main_loop_unref(socket_server->main_loop);
    g_main_context_unref(socket_server->main_context);
    socket_server_destroy(socket_server);

    debug_fleave();
    return;
}
