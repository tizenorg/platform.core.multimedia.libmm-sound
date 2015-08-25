
#ifndef __MM_SOUND_CONNECTOR_H__
#define __MM_SOUND_CONNECTOR_H__

#include "mm_sound_connection.h"
#include "mm_sound_msg.h"

typedef struct mm_sound_connector mm_sound_connector;

typedef void (*mm_sound_connection_handler)(mm_sound_connection *conn, void *userdata);

mm_sound_connector* mm_sound_connector_new(audio_service_t service);
int mm_sound_connector_set_connection_handler(mm_sound_connector *connector, mm_sound_connection_handler handler, void *userdata);
int mm_sound_connector_start(mm_sound_connector* connector);
void mm_sound_connector_stop(mm_sound_connector* connector);
void mm_sound_connector_destroy(mm_sound_connector* connector);

#endif
