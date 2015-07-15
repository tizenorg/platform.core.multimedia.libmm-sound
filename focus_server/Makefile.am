bin_PROGRAMS = focus_server
focus_server_SOURCES = mm_sound_mgr_focus_ipc.c \
						mm_sound_mgr_focus_dbus.c \
						mm_sound_focus_server.c \
						mm_sound_mgr_focus.c

focus_server_CFLAGS = -I$(srcdir)/../include \
					  -I$(srcdir)/include \
					  $(MMCOMMON_CFLAGS) \
					  $(MMLOGSVR_CFLAGS) \
					  -DMMF_LOG_OWNER=0x004 \
					  -DMMF_DEBUG_PREFIX=\"MMF-SOUND\" \
					  -D__DEBUG_MODE__ -DUSE_VCONF -D__USE_LOGMANAGER__ \
			  		  $(VCONF_CFLAGS) \
			  		  $(GLIB2_CFLAGS) \
			  		  $(GIO_CFLAGS) \
			  		  $(HEYNOTI_CFLAGS)

focus_server_LDADD = $(MMLOGSVR_LIBS) \
		     -ldl -lrt \
		     $(MMCOMMON_LIBS) \
		     $(GLIB2_LIBS) \
		     $(GIO_LIBS) \
		     $(VCONF_LIBS) \
		     $(HEYNOTI_LIBS) \
		     $(srcdir)/../libmmfsound.la \
		     $(srcdir)/../common/libmmfsoundcommon.la

if USE_SECURITY
focus_server_CFLAGS += $(SECURITY_CFLAGS) -DUSE_SECURITY
focus_server_LDADD += $(SECURITY_LIBS)
endif

focus_server_DEPENDENCIES = $(srcdir)/../common/libmmfsoundcommon.la
focus_server_CFLAGS += -fPIC -pie
