

#ifndef __MM_SOUND_CLIENT_DBUS_H__
#define __MM_SOUND_CLIENT_DBUS_H__

#include "mm_sound_private.h"
#include "mm_sound_device.h"
#ifdef USE_FOCUS
#include "mm_sound_focus.h"
#endif
#include "include/mm_sound_client.h"

int mm_sound_client_dbus_play_tone(int tone, int repeat, int volume, int volume_config,
			int session_type, int session_options, int client_pid,
			bool enable_session, int *codechandle, char *stream_type, int stream_index);
int mm_sound_client_dbus_play_tone_with_stream_info(int client_pid, int tone, char *stream_type, int stream_id, int volume, int repeat, int *codechandle);
int mm_sound_client_dbus_play_sound(const char* filename, int tone, int repeat, int volume, int volume_config,
			int priority, int session_type, int session_options, int client_pid, int handle_route,
			bool enable_session, int *codechandle, char *stream_type, int stream_index);
int mm_sound_client_dbus_play_sound_with_stream_info(const char* filename, int repeat, int volume,
			int priority, int client_pid, int handle_route, int *codechandle, char *stream_type, int stream_index);
int mm_sound_client_dbus_stop_sound(int handle);
int mm_sound_client_dbus_clear_focus(int pid); // Not original focus feature, only for cleaning up tone/wav player internal focus usage.
int mm_sound_client_dbus_is_route_available(mm_sound_route route, bool *is_available);
int mm_sound_client_dbus_foreach_available_route_cb(mm_sound_available_route_cb, void *user_data);
int mm_sound_client_dbus_set_active_route(mm_sound_route route, bool need_broadcast);
int mm_sound_client_dbus_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out);
int mm_sound_client_dbus_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data);
int mm_sound_client_dbus_remove_active_device_changed_callback(const char *name);
int mm_sound_client_dbus_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data);
int mm_sound_client_dbus_remove_available_route_changed_callback(void);
int mm_sound_client_dbus_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in);
int mm_sound_client_dbus_set_active_route_auto(void);
int mm_sound_client_dbus_add_play_sound_end_callback(mm_sound_stop_callback_wrapper_func stop_cb, void* userdata, unsigned int *subs_id);
int mm_sound_client_dbus_remove_play_sound_end_callback(unsigned int subs_id);
int mm_sound_client_dbus_get_current_connected_device_list(int device_flags, GList** device_list);
int mm_sound_client_dbus_add_device_connected_callback(int device_flags, mm_sound_device_connected_wrapper_cb func, void* user_data, unsigned int *subs_id);
int mm_sound_client_dbus_remove_device_connected_callback(unsigned int subs_id);
int mm_sound_client_dbus_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_wrapper_cb func, void* user_data, unsigned int *subs_id);
int mm_sound_client_dbus_remove_device_info_changed_callback(unsigned int subs_id);
int mm_sound_client_dbus_is_bt_a2dp_on (bool *connected, char** bt_name);
int mm_sound_client_dbus_set_volume_by_type(const char *volume_type, const unsigned int volume_level);
int mm_sound_client_dbus_add_volume_changed_callback(mm_sound_volume_changed_wrapper_cb func, void* user_data, unsigned int *subs_id);
int mm_sound_client_dbus_remove_volume_changed_callback(unsigned int subs_id);
int mm_sound_client_dbus_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out);

#ifdef USE_FOCUS
int mm_sound_client_dbus_register_focus(int id, int instance, const char *stream_type, mm_sound_focus_changed_cb callback, bool is_for_session, void* user_data);
int mm_sound_client_dbus_unregister_focus(int instance, int id, bool is_for_session);
int mm_sound_client_dbus_set_foucs_reacquisition(int instance, int id, bool reacquisition);
int mm_sound_client_dbus_acquire_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session);
int mm_sound_client_dbus_release_focus(int instance, int id, mm_sound_focus_type_e type, const char *option, bool is_for_session);
int mm_sound_client_dbus_set_focus_watch_callback(int instance, int handle, mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, bool is_for_session, void *user_data);
int mm_sound_client_dbus_unset_focus_watch_callback(int focus_tid, int handle, bool is_for_session);
int mm_sound_client_dbus_emergent_exit_focus(int exit_pid);
#endif

int mm_sound_client_dbus_add_test_callback(mm_sound_test_cb func, void* user_data, unsigned int *subs_id);
int mm_sound_client_dbus_remove_test_callback(unsigned int subs_id);
int mm_sound_client_dbus_test(int a, int b, int* get);

int mm_sound_client_dbus_initialize(void);
int mm_sound_client_dbus_finalize(void);

#endif /* __MM_SOUND_CLIENT_DBUS_H__ */
