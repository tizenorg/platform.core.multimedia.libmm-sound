#ifndef __MM_SOUND_SOCKET_H__
#define __MM_SOUND_SOCKET_H__

typedef struct mm_sound_socket_server mm_sound_socket_server;
typedef struct mm_sound_socket_client mm_sound_socket_client;
typedef struct mm_sound_socket_connection mm_sound_socket_connection;

int mm_sound_socket_server_new(const char *socket_path, mm_sound_socket_server **_socket_server);
void mm_sound_socket_server_destroy(mm_sound_socket_server *socket_server);
int mm_sound_socket_server_start(mm_sound_socket_server *socket_server);
void mm_sound_socket_server_stop(mm_sound_socket_server *socket_server);
int mm_sound_socket_server_send(mm_sound_socket_server *socket_server, char *target_id, const char *msg, size_t len);
int mm_sound_socket_server_recv(mm_sound_socket_server *socket_server, char *target_id, char *msg, size_t len);

int mm_sound_socket_client_new(mm_sound_socket_client **_socket_client, const char *socket_id);
int mm_sound_socket_client_destroy(mm_sound_socket_client *socket_client);
int mm_sound_socket_client_connect(mm_sound_socket_client *socket_client, const char *server_socket_path, mm_sound_socket_connection **_socket_conn);
int mm_sound_socket_connection_close(mm_sound_socket_connection *socket_conn);
int mm_sound_socket_connection_send(mm_sound_socket_connection *socket_conn, const char *msg, size_t len);
int mm_sound_socket_connection_recv(mm_sound_socket_connection *socket_conn, char *msg, size_t len);

#endif
