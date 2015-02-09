

#ifndef __MM_SOUND_CLIENT_DBUS_H__
#define __MM_SOUND_CLIENT_DBUS_H__

#include "mm_sound_private.h"
#include "mm_sound_device.h"
#ifdef USE_FOCUS
#include "mm_sound_focus.h"
#endif

//int MMSoundClientDbusPlayTone(int number, int volume_config, double volume, int time, int *handle, bool enable_session);
int MMSoundClientDbusPlayTone(int tone, int repeat, int volume, int volume_config,
			   int session_type, int session_options, int client_pid,
			   bool enable_session, int *codechandle);
//int MMSoundClientDbusPlaySound(MMSoundPlayParam *param, int tone, int keytone, int *handle);
int MMSoundClientDbusPlaySound(char* filename, int tone, int repeat, int volume, int volume_config,
			   int priority, int session_type, int session_options, int client_pid, int keytone,  int handle_route,
			   bool enable_session, int *codechandle);
int MMSoundClientDbusStopSound(int handle);
int _mm_sound_client_dbus_is_route_available(mm_sound_route route, bool *is_available);
int _mm_sound_client_dbus_foreach_available_route_cb(mm_sound_available_route_cb, void *user_data);
int _mm_sound_client_dbus_set_active_route(mm_sound_route route, bool need_broadcast);
int _mm_sound_client_dbus_get_active_device(mm_sound_device_in *device_in, mm_sound_device_out *device_out);
int _mm_sound_client_dbus_add_active_device_changed_callback(const char *name, mm_sound_active_device_changed_cb func, void* user_data);
int _mm_sound_client_dbus_remove_active_device_changed_callback(const char *name);
int _mm_sound_client_dbus_add_available_route_changed_callback(mm_sound_available_route_changed_cb func, void* user_data);
int _mm_sound_client_dbus_remove_available_route_changed_callback(void);
int _mm_sound_client_dbus_set_sound_path_for_active_device(mm_sound_device_out device_out, mm_sound_device_in device_in);
#ifdef USE_FOCUS
int _mm_sound_client_dbus_register_focus(int id, const char *stream_type, mm_sound_focus_changed_cb callback, void* user_data);
int _mm_sound_client_dbus_unregister_focus(int id);
int _mm_sound_client_dbus_acquire_focus(int id, mm_sound_focus_type_e type, const char *option);
int _mm_sound_client_dbus_release_focus(int id, mm_sound_focus_type_e type, const char *option);
int _mm_sound_client_dbus_set_focus_watch_callback(mm_sound_focus_type_e type, mm_sound_focus_changed_watch_cb callback, void* user_data);
int _mm_sound_client_dbus_unset_focus_watch_callback(void);
#endif

int _mm_sound_client_dbus_set_active_route_auto(void);



int _mm_sound_client_dbus_add_play_sound_end_callback(int handle, mm_sound_stop_callback_func stop_cb, void* userdata);
//int _mm_sound_client_dbus_get_current_connected_device_list(int device_flags, mm_sound_device_list_t *device_list);
int _mm_sound_client_dbus_get_current_connected_device_list(int device_flags, GList** device_list);
int _mm_sound_client_dbus_add_device_connected_callback(int device_flags, mm_sound_device_connected_cb func, void* user_data);
int _mm_sound_client_dbus_remove_device_connected_callback(void);
int _mm_sound_client_dbus_add_device_info_changed_callback(int device_flags, mm_sound_device_info_changed_cb func, void* user_data);
int _mm_sound_client_dbus_remove_device_info_changed_callback(void);
int MMSoundClientDbusIsBtA2dpOn (bool *connected, char** bt_name);
int _mm_sound_client_dbus_add_volume_changed_callback(mm_sound_volume_changed_cb func, void* user_data);
int _mm_sound_client_dbus_remove_volume_changed_callback(void);
int _mm_sound_client_dbus_get_audio_path(mm_sound_device_in *device_in, mm_sound_device_out *device_out);

int _mm_sound_client_dbus_add_test_callback(mm_sound_test_cb func, void* user_data);
int _mm_sound_client_dbus_remove_test_callback(void);
int _mm_sound_client_dbus_test(int a, int b, int* get);


int MMSoundClientDbusInit(void);
int MMSoundClientDbusFini(void);

#endif /* __MM_SOUND_CLIENT_DBUS_H__ */
