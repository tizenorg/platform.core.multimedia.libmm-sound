#ifndef __MM_SOUND_SOCKET_H__
#define __MM_SOUND_SOCKET_H__

typedef struct mm_sound_socket_server mm_sound_socket_server;
typedef struct mm_sound_socket_client mm_sound_socket_client;
typedef struct mm_sound_socket_connection mm_sound_socket_connection;
typedef struct mm_sound_packet mm_sound_packet;

typedef enum {
    SOCKET_CONNECTION_STATUS_NEW,
    SOCKET_CONNECTION_STATUS_CONNECTED,
    SOCKET_CONNECTION_STATUS_DISCONNECTED,
    SOCKET_CONNECTION_STATUS_DESTROYED,
} socket_connection_status_t;

typedef void (*mm_sound_socket_packet_callback) (mm_sound_socket_connection *socket_conn, void *msg, size_t len, void *userdata);
typedef void (*mm_sound_socket_connected_callback) (mm_sound_socket_connection *socket_conn, const char *socket_id, void *userdata);

int mm_sound_socket_server_new(const char *socket_path, mm_sound_socket_server **_socket_server);
void mm_sound_socket_server_destroy(mm_sound_socket_server *socket_server);
int mm_sound_socket_server_start(mm_sound_socket_server *socket_server);
void mm_sound_socket_server_stop(mm_sound_socket_server *socket_server);
int mm_sound_socket_server_set_connected_callback(mm_sound_socket_server *socket_server, mm_sound_socket_connected_callback conn_cb, void *userdata);

int mm_sound_socket_client_new(const char *socket_id, mm_sound_socket_client **_socket_client);
int mm_sound_socket_client_destroy(mm_sound_socket_client *socket_client);
int mm_sound_socket_client_connect(mm_sound_socket_client *socket_client, const char *server_socket_path, mm_sound_socket_connection **_socket_conn);
void mm_sound_socket_connection_destroy(mm_sound_socket_connection *socket_conn);
int mm_sound_socket_connection_send(mm_sound_socket_connection *socket_conn, const void *msg, size_t len);
int mm_sound_socket_connection_recv(mm_sound_socket_connection *socket_conn, void *msg, size_t *len);
int mm_sound_socket_connection_set_packet_callback(mm_sound_socket_connection *socket_conn, mm_sound_socket_packet_callback packet_cb, void *userdata);
int mm_sound_socket_connection_unset_packet_callback(mm_sound_socket_connection *socket_conn);

#endif
