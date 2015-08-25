
#include <stdlib.h>
#include <string.h>

#include "../include/mm_sound_socket.h"
#include "../include/mm_sound_connection.h"
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"

#include <mm_error.h>
#include <mm_debug.h>
#include <glib.h>
#include <gio/gio.h>

#define FOCUS_SERVER_PATH           "/run/focus-server"
#define FOCUS_SERVER_SOCK_PATH      FOCUS_SERVER_PATH "/focus-server.socket"
#define SOUND_SERVER_PATH           "/run/sound-server"
#define SOUND_SERVER_SOCK_PATH      SOUND_SERVER_PATH "/sound-server.socket"

#define MSG_LEN_MAX                 128

struct mm_sound_connection {
    mm_sound_socket_client *socket_client;
    mm_sound_socket_connection *socket_connection;
    GMainLoop *mainloop;
    unsigned message_count;
    GHashTable *response_handler;
};

typedef struct event_handle_data {
    mm_sound_connection *conn;
    mm_sound_event_handler user_handler;
    void *user_data;
} event_handle_data;

typedef struct request_handle_data {
    mm_sound_connection *conn;
    mm_sound_request_handler user_handler;
    void *user_data;
} request_handle_data;


EXPORT_API
int mm_sound_connection_remove_event_callback(mm_sound_connection *conn, unsigned callback_id) {
    int ret = MM_ERROR_NONE;
    debug_fenter();
FINISH:
    debug_fleave();
    return ret;
}

static int build_msg(unsigned msg_id, msg_type_t msg_type, GVariant *iner_var, void *msg, size_t *msg_size) {
    GVariant *msg_var;
    gsize size;

    if((msg_var = g_variant_new("(uiv)", msg_id, msg_type, iner_var)) == NULL) {
        debug_error("create msg variant error");
        return -1;
    }

    if((size = g_variant_get_size(msg_var)) > MSG_LEN_MAX) {
        debug_error("message size is bigger than buffer size : %d", MSG_LEN_MAX);
        return -1;
    }

    g_variant_store(msg_var, msg);
    *msg_size = (size_t)size;

    g_variant_unref(msg_var);

    return 0;
}

static int build_request_msg(unsigned msg_id, int request_type, GVariant *params, void *msg, size_t *msg_size) {
    GVariant *msg_var, *request_var;
    size_t size;

    if(!params || !msg || !msg_size) {
        debug_error("Null parameters for build request msg");
        return -1;
    }

    if ((request_var = g_variant_new("(iv)", request_type, params)) == NULL) {
        debug_error("create request variant error");
        return -1;
    }

    if (build_msg(msg_id, MSG_TYPE_REQUEST, request_var, msg, &size) < 0) {
        debug_error("build_msg failed");
        return -1;
    }
    *msg_size = size;
    g_variant_unref(request_var);

    return 0;
}

static int build_response_msg(unsigned msg_id, unsigned msg_id_response_for, int type_reponse_for, GVariant *params, void *msg, size_t *msg_size) {
    GVariant *msg_var, *request_var;
    size_t size;
    msg_type_t msg_type = MSG_TYPE_REQUEST;

    if(!params || !msg || !msg_size) {
        debug_error("Null parameters for build response msg");
        return -1;
    }

    if ((request_var = g_variant_new("(uiv)", msg_id_response_for, type_reponse_for, params)) == NULL) {
        debug_error("create response variant error");
        return -1;
    }

    if (build_msg(msg_id, MSG_TYPE_REQUEST, request_var, msg, &size) < 0) {
        debug_error("build_msg failed");
        return -1;
    }
    *msg_size = size;
    g_variant_unref(request_var);

    return 0;
}

static int extract_msg(void *msg, size_t size, unsigned *msg_id, msg_type_t *msg_type, GVariant **params) {
    GVariant *msg_var = NULL, *_params = NULL;
    GVariantType *var_type = NULL;
    unsigned _msg_id;
    int _msg_type;

    var_type = g_variant_type_new("(iiv)");

    msg_var = g_variant_new_from_data(var_type, msg, size, TRUE, NULL, NULL);
    g_variant_get(msg_var, "(iiv)", &_msg_id, &_msg_type, &_params);

    *msg_id = _msg_id;
    *msg_type = _msg_type;
    *params = _params;

    return 0;
}

static void packet_received_callback(mm_sound_socket_connection *socket_conn, void *packet_data, size_t packet_size, void *userdata) {
    int type;
    GVariant *param = NULL;
    msg_type_t msg_type;
    unsigned msg_id = 0;
    mm_sound_connection *conn = (mm_sound_connection*) userdata;

    debug_fenter();

    if(!conn) {
        debug_error("handler null");
    }

    extract_msg(packet_data, packet_size, &msg_id, &msg_type, &param);

    // TODO : classify packet and call callback or handler.
    if (msg_type == MSG_TYPE_REQUEST) {
        debug_log("Got request message");
    } else if (msg_type == MSG_TYPE_RESPONSE) {
        debug_log("Got response message");
    } else if (msg_type == MSG_TYPE_SIGNAL) {
        debug_log("Got siganl message");
    } else if (msg_type == MSG_TYPE_SUBSCRIBE) {
        debug_log("Got subscribe message");
    }

    debug_fleave();
}

// For client, should connect with mm_sound_connection_connect.
EXPORT_API
mm_sound_connection* mm_sound_connection_new(GMainLoop *mainloop) {
    int ret = MM_ERROR_NONE;
    mm_sound_socket_client *socket_client;
    mm_sound_connection *conn;

    debug_fenter();
    mm_sound_socket_client_new("socket-client", &socket_client);

    conn = g_malloc0(sizeof(mm_sound_connection));
    conn->socket_client = socket_client;
    conn->mainloop = mainloop;
    conn->socket_connection = NULL;
    conn->message_count = 0;

FINISH:
    debug_fleave();
    return ret;
}

// For server, use already connected socket connection.
EXPORT_API
mm_sound_connection* mm_sound_connection_new_with_socket_connection(GMainLoop *mainloop, mm_sound_socket_connection *socket_conn) {
    int ret = MM_ERROR_NONE;
    mm_sound_connection *conn;

    debug_fenter();

    conn = g_malloc0(sizeof(mm_sound_connection));
    conn->socket_client = NULL;
    conn->mainloop = mainloop;
    conn->socket_connection = socket_conn;
    conn->message_count = 0;

    mm_sound_socket_connection_set_packet_callback(conn->socket_connection, packet_received_callback, conn);

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_connect(mm_sound_connection *conn, audio_service_t service) {
    int ret = MM_ERROR_NONE;
    const char *server_sock_path;
    mm_sound_socket_connection *socket_connection;
    debug_fenter();

    if(service != AUDIO_SERVICE_SOUND_SERVER && service != AUDIO_SERVICE_FOCUS_SERVER) {
        debug_error("wrong service : %d", service);
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if (service == AUDIO_SERVICE_FOCUS_SERVER) {
        server_sock_path = FOCUS_SERVER_SOCK_PATH;
    } else if (service == AUDIO_SERVICE_SOUND_SERVER) {
        server_sock_path = SOUND_SERVER_SOCK_PATH;
    }

    if((ret = mm_sound_socket_client_connect(conn->socket_client, server_sock_path, &socket_connection)) != MM_ERROR_NONE) {
        debug_error("socket client connect failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }


    conn->socket_connection = socket_connection;

    mm_sound_socket_connection_set_packet_callback(conn->socket_connection, packet_received_callback, conn);

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_disconnect(mm_sound_connection *conn) {
    int ret = MM_ERROR_NONE;
    debug_fenter();

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
void mm_sound_connection_finalize(mm_sound_connection *conn) {
    debug_fenter();

    mm_sound_socket_client_destroy(conn->socket_client);

    mm_sound_socket_connection_destroy(conn->socket_connection);

    debug_fleave();
    return;
}

EXPORT_API
int mm_sound_connection_add_event_callback(mm_sound_connection *conn, int event_type, mm_sound_event_handler callback, GVariant *params, void *destroy_func, unsigned *callback_id, void *userdata) {
    int ret = MM_ERROR_NONE;
    event_handle_data *handle_data = NULL;
    debug_fenter();

    handle_data = g_malloc0(sizeof(event_handle_data));
    handle_data->conn = conn;
    handle_data->user_handler = callback;
    handle_data->user_data = userdata;
FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_request(mm_sound_connection *conn, int request_type, GVariant *params, GVariant **result) {
    int ret = MM_ERROR_NONE;
    char request_msg[MSG_LEN_MAX] = {0}, response_msg[MSG_LEN_MAX] = {0};
    size_t request_msg_size = -1, response_msg_size = -1;
    int response_type = -1;
    GVariant *response_param = NULL;
    unsigned cnt = 0;

    debug_fenter();
    cnt = conn->message_count++;
    build_request_msg(cnt, request_type, params, request_msg, &request_msg_size);
    mm_sound_socket_connection_send(conn->socket_connection, request_msg, request_msg_size);
    // TODO : register callback? for response and wait.

    *result = response_param;
FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_response(mm_sound_connection *conn, int response_for, int request_or_event_type, GVariant *param_ret) {
    int ret = MM_ERROR_NONE;
    char response_msg[MSG_LEN_MAX] = {0};
    size_t response_msg_size = -1;

    debug_fenter();

    build_msg(MSG_TYPE_RESPONSE, response_for, param_ret, response_msg, &response_msg_size);
    mm_sound_socket_connection_send(conn->socket_connection, response_msg, response_msg_size);

FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_send_signal(mm_sound_connection *conn, sound_server_signal_t signal_type, GVariant *params) {
    int ret = MM_ERROR_NONE;
    char signal_msg[MSG_LEN_MAX] = {0};
    size_t signal_msg_size = -1;

    debug_fenter();
    build_msg(MSG_TYPE_SIGNAL, signal_type, params, signal_msg, &signal_msg_size);
    mm_sound_socket_connection_send(conn->socket_connection, signal_msg, signal_msg_size);
FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_send_signal_with_return(mm_sound_connection *conn, sound_server_signal_t signal_type, GVariant *params, GVariant **result) {
    int ret = MM_ERROR_NONE;
    char signal_msg[MSG_LEN_MAX] = {0}, response_msg[MSG_LEN_MAX] = {0};
    size_t signal_msg_size = -1, response_msg_size = -1;
    int response_type = -1;
    GVariant *response_param = NULL;

    debug_fenter();
    build_msg(MSG_TYPE_SIGNAL, signal_type, params, signal_msg, &signal_msg_size);
    mm_sound_socket_connection_send(conn->socket_connection, signal_msg, signal_msg_size);
    /*
    mm_sound_socket_connection_recv(conn->socket_connection, response_msg, &response_msg_size);
    extract_msg(reponse_msg, response_msg_size, &response_type, &response_param);
    */

    *result = response_param;
FINISH:
    debug_fleave();
    return ret;
}

EXPORT_API
int mm_sound_connection_install_handler_table(mm_sound_connection *conn, mm_sound_request_handler handlers[], unsigned entry_num) {
    int ret = MM_ERROR_NONE;
    debug_fenter();
FINISH:
    debug_fleave();
    return ret;
}

