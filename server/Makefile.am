SUBDIRS = plugin

bin_PROGRAMS = focus_server
focus_server_SOURCES = mm_sound_mgr_codec.c \
						mm_sound_mgr_ipc.c \
						mm_sound_mgr_ipc_dbus.c \
						mm_sound_mgr_run.c \
						mm_sound_mgr_focus.c \
						mm_sound_mgr_focus_dbus.c \
						mm_sound_mgr_focus_ipc.c \
						mm_sound_plugin.c \
						mm_sound_server.c \
						mm_sound_thread_pool.c

focus_server_CFLAGS = -I$(srcdir)/../include \
					  -I$(srcdir)/include \
					  $(MMCOMMON_CFLAGS) \
					  $(MMLOGSVR_CFLAGS) \
					  -DMMF_LOG_OWNER=0x002 \
					  -DMMF_DEBUG_PREFIX=\"MMF-SOUND\" \
					  -D__DEBUG_MODE__ -DUSE_VCONF -D__USE_LOGMANAGER__ \
					  $(MMSESSION_CFLAGS) \
			  		  $(AUDIOSESSIONMGR_CFLAGS) \
			  		  $(VCONF_CFLAGS) \
			  		  $(GLIB2_CFLAGS) \
			  		  $(GIO_CFLAGS)

focus_server_LDADD = $(MMLOGSVR_LIBS) \
		     -ldl -lrt \
		     $(MMCOMMON_LIBS) \
		     $(MMSESSION_LIBS) \
		     $(AUDIOSESSIONMGR_LIBS) \
		     $(GLIB2_LIBS) \
		     $(GIO_LIBS) \
		     $(VCONF_LIBS) \
		     $(srcdir)/../libmmfsound.la \
		     $(srcdir)/../common/libmmfsoundcommon.la

if USE_PULSE
focus_server_CFLAGS += -DPULSE_CLIENT $(PA_CFLAGS) $(PASIMPLE_CFLAGS)
focus_server_LDADD += $(PA_LIBS) $(PASIMPLE_CFLAGS)
endif

if USE_SECURITY
focus_server_CFLAGS += $(SECURITY_CFLAGS) -DUSE_SECURITY
focus_server_LDADD += $(SECURITY_LIBS)
endif

focus_server_DEPENDENCIES = $(srcdir)/../common/libmmfsoundcommon.la
if !USE_PRELINK
focus_server_CFLAGS += -fPIC -pie
endif
installsounddir = $(prefix)/share/sounds/sound-server
installsound_DATA = sounds/Tizen_HW_Touch.ogg
