#ifndef __MM_SOUND_MGR_IPC_DBUS_H__
#define __MM_SOUND_MGR_IPC_DBUS_H__

#include <gio/gio.h>
#include "../include/mm_sound_device.h"
#include "../include/mm_sound_stream.h"

int mm_sound_mgr_ipc_dbus_send_signal_freeze(char *command, int pid);

int __mm_sound_mgr_ipc_dbus_notify_play_file_end(int handle);
int __mm_sound_mgr_ipc_dbus_notify_device_connected(mm_sound_device_t * device, gboolean is_connected);
int __mm_sound_mgr_ipc_dbus_notify_device_info_changed(mm_sound_device_t * device, int changed_device_info_type);
int __mm_sound_mgr_ipc_dbus_notify_volume_changed(unsigned int vol_type, unsigned int value);
int __mm_sound_mgr_ipc_dbus_notify_active_device_changed(int device_in, int device_out);
int __mm_sound_mgr_ipc_dbus_notify_available_device_changed(int device_in, int device_out, int available);
int __mm_sound_mgr_ipc_dbus_get_stream_list(stream_list_t * stream_list);

int MMSoundMgrDbusInit(void);
void MMSoundMgrDbusFini(void);

#endif							/* __MM_SOUND_MGR_DBUS_H__ */
