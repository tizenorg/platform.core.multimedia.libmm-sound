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
#include <glib-object.h>

#define BACKLOG 5
#define SOCKET_PATH_BASE "/var/run/audio/"
#define SOCKET_PATH_MAX 30

#define SOCKET_TEST
#define CLIENT_ABSTRACT_SOCKET

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
};

struct mm_sound_socket_connection {
    GSocketConnection *conn;
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

static int socket_connection_recv(mm_sound_socket_connection *socket_conn, char *msg, size_t len)
{
    int ret = MM_ERROR_NONE;
    GIOStream *io_stream = NULL;
    GInputStream *in_stream = NULL;
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
    in_stream = g_io_stream_get_input_stream(io_stream);
    if (g_input_stream_read(in_stream, msg, len, NULL, &error) == -1) {
        debug_error("input stream read failed : %s", error->message);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();
    return ret;
}

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

FINISH:
    debug_fleave();

    return ret;
}

static void socket_connection_destroy(mm_sound_socket_connection *socket_conn)
{
    debug_fenter();

    if (!socket_conn) {
        debug_warning("Null Parameter");
        return;
    }

    if (socket_conn)
        g_object_unref(socket_conn->conn);
    g_free(socket_conn);

    debug_fleave();
}

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


    g_socket_client_set_family(socket_client->client, G_SOCKET_FAMILY_UNIX);
    g_socket_client_set_protocol(socket_client->client, G_SOCKET_PROTOCOL_DEFAULT);
    g_socket_client_set_socket_type(socket_client->client, G_SOCKET_TYPE_STREAM);
    g_socket_client_set_local_address(socket_client->client, gaddr);

    *_socket_client = socket_client;

FINISH:
    g_object_unref(gaddr);
    debug_fleave();

    return ret;
}


static int socket_client_destroy(mm_sound_socket_client *socket_client)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_client) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

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

    socket_conn->conn = conn;
    *_socket_conn = socket_conn;

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_socket_client_new(mm_sound_socket_client **_socket_client, const char *socket_id)
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
int mm_sound_socket_connection_close(mm_sound_socket_connection *socket_conn)
{
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (!socket_conn) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_close(socket_conn)) != MM_ERROR_NONE) {
        debug_error("socket_connection_close failed");
        goto FINISH;
    }

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_socket_connection_send(mm_sound_socket_connection *socket_conn, const char *msg, size_t len)
{
    int ret = MM_ERROR_NONE;

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

EXPORT_API
int mm_sound_socket_connection_recv(mm_sound_socket_connection *socket_conn, char *msg, size_t len)
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

static gboolean incoming_callback(GSocketService *service, GSocketConnection *conn, GObject *source_object, gpointer userdata)
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

    if ((socket_conn = g_malloc0(sizeof(mm_sound_socket_connection *))) == NULL) {
        debug_error("allocate for socket connection failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    socket_conn->conn = conn;

    if (_socket_server_store_connection(socket_server, remote_id, socket_conn) != MM_ERROR_NONE) {
        debug_error("socket_store failed for '%s'", remote_id);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
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

    g_signal_connect(socket_server->service, "incoming", G_CALLBACK(incoming_callback), socket_server);

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

/*
static int socket_server_send(mm_sound_socket_server *socket_server, const char *target_id, const char *msg, size_t len)
{
    int ret = MM_ERROR_NONE;
    mm_sound_socket_connection *socket_conn = NULL;

    debug_fenter();

    if (!socket_server || !socket_server->service || !socket_server->connections) {
        debug_error("Invalid Parameter");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = _socket_server_get_connection_from_id(socket_server, target_id, &socket_conn)) != MM_ERROR_NONE) {
        debug_error("get connection from id failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = socket_connection_send(socket_conn, msg, len)) != MM_ERROR_NONE) {
        debug_error("socket connection send failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

FINISH:
    debug_fleave();
    return ret;
}
*/

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
    debug_fenter();

    if (!socket_server) {
        debug_error("Invalid Parameter");
        return;
    }
    socket_server_stop(socket_server);
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
