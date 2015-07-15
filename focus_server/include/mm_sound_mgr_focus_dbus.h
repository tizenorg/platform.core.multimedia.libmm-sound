#ifndef __MM_SOUND_MGR_IPC_DBUS_H__
#define __MM_SOUND_MGR_IPC_DBUS_H__

#include <gio/gio.h>
#include "../include/mm_sound_device.h"
#include "../include/mm_sound_stream.h"


int mm_sound_mgr_ipc_dbus_send_signal_freeze (char* command, int pid);

int __mm_sound_mgr_ipc_dbus_get_stream_list(stream_list_t* stream_list);

int MMSoundMgrDbusInit(void);
void MMSoundMgrDbusFini(void);


#endif /* __MM_SOUND_MGR_DBUS_H__ */
