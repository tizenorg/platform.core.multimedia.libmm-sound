#ifndef __MM_SOUND_PROXY_H__
#define __MM_SOUND_PROXY_H__

#include "mm_sound_client.h"

int mm_sound_proxy_initialize(void);
int mm_sound_proxy_finalize(void);
int mm_sound_proxy_test(int a, int b, int *_sum);
int mm_sound_proxy_add_focus_callback(int instance, int handle, mm_sound_focus_changed_wrapper_cb callback, unsigned *callback_id, void *userdata);
int mm_sound_proxy_remove_focus_callback(unsigned callback_id);
int mm_sound_proxy_add_focus_watch_callback(mm_sound_focus_changed_watch_wrapper_cb callback, void *user_data);

#endif
