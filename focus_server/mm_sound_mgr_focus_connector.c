#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include "include/mm_sound_mgr_focus_ipc.h"

#include "../include/mm_sound_common.h"
#include "../include/mm_sound_msg.h"
#include "../include/mm_sound_connector.h"
#include "../include/mm_sound_connection.h"
#include "include/mm_sound_mgr_focus.h"
#include <mm_error.h>
#include <mm_debug.h>

#include <gio/gio.h>

mm_sound_connector *connector = NULL;

void handler_register_focus(mm_sound_connection *conn, GVariant *params, void *userdata)
{
    debug_fenter();
    debug_fleave();
}

void handler_test(mm_sound_connection *conn, GVariant *params, void *userdata)
{
    debug_fenter();
    debug_fleave();
}

mm_sound_request_handler focus_handler_table[METHOD_CALL_MAX] = {
    [METHOD_CALL_REGISTER_FOCUS] = handler_register_focus,
    [METHOD_CALL_TEST] = handler_test,
};

void new_connection_handler(mm_sound_connection *conn, void *userdata)
{
    mm_sound_connection_install_handler_table(conn, focus_handler_table, METHOD_CALL_MAX);
}

int mm_sound_mgr_connector_init()
{
    connector = mm_sound_connector_new(AUDIO_SERVICE_FOCUS_SERVER);
    mm_sound_connector_set_connection_handler(connector, new_connection_handler, NULL);
    mm_sound_connector_start(connector);
    return 0;
}

void mm_sound_mgr_connector_fini()
{
    mm_sound_connector_stop(connector);
    mm_sound_connector_destroy(connector);
    return 0;
}

