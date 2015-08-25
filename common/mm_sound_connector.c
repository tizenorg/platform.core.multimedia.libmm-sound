

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mm_error.h>
#include <mm_debug.h>

#include "../include/mm_sound_connector.h"
#include "../include/mm_sound_connection.h"
#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
#include "../include/mm_sound_socket.h"

struct mm_sound_connector {
    mm_sound_socket_server *socket_server;
    mm_sound_connection_handler connection_handler;
    void *connection_handler_data;
};

#define FOCUS_SERVER_PATH           "/run/focus-server"
#define FOCUS_SERVER_SOCK_PATH      FOCUS_SERVER_PATH "/focus-server.socket"
#define SOUND_SERVER_PATH           "/run/sound-server"
#define SOUND_SERVER_SOCK_PATH      SOUND_SERVER_PATH "/sound-server.socket"

static int make_dir(const char *path)
{
    if (access(path, F_OK) == 0) {
        debug_log("directory '%s' already exists", path);
        return 0;
    }

    if (mkdir(path, 0777) < 0) {
        debug_error("make directory '%s' failed", path);
        return MM_ERROR_SOUND_INTERNAL;
    }

    return 0;
}

EXPORT_API
mm_sound_connector* mm_sound_connector_new(audio_service_t service) {
    int ret = MM_ERROR_NONE;
    mm_sound_connector *connector = NULL;
    mm_sound_socket_server *socket_server;
    const char *server_path = NULL, *server_sock_path = NULL;

    debug_fenter();

    if (service != AUDIO_SERVICE_SOUND_SERVER && service != AUDIO_SERVICE_FOCUS_SERVER) {
        debug_error("not supported service : %d", service);
        return NULL;
    }

    if (service == AUDIO_SERVICE_FOCUS_SERVER) {
        server_path = FOCUS_SERVER_PATH;
        server_sock_path = FOCUS_SERVER_SOCK_PATH;
    } else if (service == AUDIO_SERVICE_SOUND_SERVER) {
        server_path = SOUND_SERVER_PATH;
        server_sock_path = SOUND_SERVER_SOCK_PATH;
    }

    if ((ret = make_dir(server_path)) != MM_ERROR_NONE) {
        debug_error("make directory failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((connector = g_malloc0(sizeof(mm_sound_connector))) == NULL) {
        debug_error("Allocate Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    if ((ret = mm_sound_socket_server_new(server_sock_path, &socket_server)) != MM_ERROR_NONE) {
        debug_error("Socket Server New Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }

    connector->socket_server = socket_server;
    connector->connection_handler = NULL;

FINISH:
    debug_fleave();

    return connector;
}

void socket_connected_callback_func(mm_sound_socket_connection *socket_conn, const char *socket_id, void *userdata) {
    GMainLoop *mainloop = NULL;
    mm_sound_connection *conn;
    mm_sound_connector *connector = (mm_sound_connector*) userdata;

    debug_fenter();
    if (connector == NULL || socket_conn == NULL) {
        debug_error("data null");
        goto FINISH;
    }
    conn = mm_sound_connection_new_with_socket_connection(mainloop, socket_conn);

    if (connector->connection_handler) {
        ((mm_sound_connection_handler)connector->connection_handler)(conn, connector->connection_handler_data);
    }
FINISH:
    debug_fleave();
}

EXPORT_API
int mm_sound_connector_set_connection_handler(mm_sound_connector *connector, mm_sound_connection_handler handler, void *userdata) {
    int ret = MM_ERROR_NONE;

    debug_fenter();

    if (connector == NULL) {
        debug_error("connector null");
        goto FINISH;
    }
    connector->connection_handler = handler;
    connector->connection_handler_data = userdata;

    ret = mm_sound_socket_server_set_connected_callback(connector->socket_server, socket_connected_callback_func, connector);

FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
int mm_sound_connector_start(mm_sound_connector *connector) {
    int ret = MM_ERROR_NONE;
    debug_fenter();

    if (connector == NULL) {
        debug_error("connector null");
        ret = MM_ERROR_SOUND_INTERNAL;
    }

    if ((ret = mm_sound_socket_server_start(connector->socket_server)) != MM_ERROR_NONE) {
        debug_error("Socket Server Start Failed");
        ret = MM_ERROR_SOUND_INTERNAL;
        goto FINISH;
    }
FINISH:
    debug_fleave();

    return ret;
}

EXPORT_API
void mm_sound_connector_stop(mm_sound_connector *connector) {
    debug_fenter();

    if (connector == NULL) {
        debug_error("connector null");
        goto FINISH;
    }

    mm_sound_socket_server_stop(connector->socket_server);

FINISH:
    debug_fleave();
    return;
}

EXPORT_API
void mm_sound_connector_destroy(mm_sound_connector *connector) {
    debug_fenter();

    if (connector == NULL) {
        debug_error("connector null");
        goto FINISH;
    }

    mm_sound_socket_server_destroy(connector->socket_server);
    g_free(connector);

FINISH:
    debug_fleave();
    return;
}
