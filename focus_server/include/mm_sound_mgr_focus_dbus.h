#ifndef __MM_SOUND_MGR_FOCUS_DBUS_H__
#define __MM_SOUND_MGR_FOCUS_DBUS_H__

#include <gio/gio.h>
#include "../include/mm_sound_device.h"
#include "../include/mm_sound_stream.h"

int mm_sound_mgr_focus_dbus_send_signal_freeze(char *command, int pid);

int __mm_sound_mgr_focus_dbus_get_stream_list(stream_list_t * stream_list);

int MMSoundMgrFocusDbusInit(void);
void MMSoundMgrFocusDbusFini(void);

#endif /* __MM_SOUND_MGR_FOCUS_DBUS_H__ */
