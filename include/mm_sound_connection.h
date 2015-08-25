#ifndef __MM_SOUND_CONNECTION_H__
#define __MM_SOUND_CONNECTION_H__

#include <glib.h>
#include "mm_sound_msg.h"
#include "mm_sound_socket.h"

typedef struct mm_sound_connection mm_sound_connection;

typedef void (*mm_sound_request_handler)(mm_sound_connection *conn, GVariant *params, void *userdata);
typedef void (*mm_sound_event_handler)(mm_sound_connection *conn, int event_type, GVariant *params, void *userdata);

// for client-side
mm_sound_connection* mm_sound_connection_new(GMainLoop *mainloop);
// for server-side
mm_sound_connection* mm_sound_connection_new_with_socket_connection(GMainLoop *mainloop, mm_sound_socket_connection *socket_conn);
int mm_sound_connection_connect(mm_sound_connection* conn, audio_service_t server);
int mm_sound_connection_disconnect(mm_sound_connection* conn);
void mm_sound_connection_finalize(mm_sound_connection* conn);

int mm_sound_connection_add_event_callback(mm_sound_connection *conn, int event_type, mm_sound_event_handler callback, GVariant *params, void *destroy_func, unsigned *callback_id, void *userdata);
int mm_sound_connection_remove_event_callback(mm_sound_connection* conn, unsigned callback_id);
int mm_sound_connection_request(mm_sound_connection *conn, int request_type, GVariant *params, GVariant **result);
int mm_sound_connection_response(mm_sound_connection* conn, int reponse_for, int request_or_event_type, GVariant *param_ret);

int mm_sound_connection_send_signal(mm_sound_connection* conn, sound_server_signal_t signal_type, GVariant *params);
int mm_sound_connection_send_signal_with_return(mm_sound_connection *conn, sound_server_signal_t signal_type, GVariant *params, GVariant **result);

int mm_sound_connection_install_handler_table(mm_sound_connection *conn, mm_sound_request_handler handlers[], unsigned entry_num);

#endif
