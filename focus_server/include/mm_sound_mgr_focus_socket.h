#ifndef __MM_SOUND_MGR_FOCUS_SOCKET_H__
#define __MM_SOUND_MGR_FOCUS_SOCKET_H__

typedef struct mgr_focus_socket_data mgr_focus_socket_data;

int MMSoundMgrFocusSocketInit(mgr_focus_socket_data **_mgr_data);
void MMSoundMgrFocusSocketFini(mgr_focus_socket_data *mgr_data);

#endif
