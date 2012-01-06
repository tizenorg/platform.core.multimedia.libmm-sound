SUBDIRS = plugin

bin_PROGRAMS = sound_server
sound_server_SOURCES = mm_sound_mgr_codec.c \
						mm_sound_mgr_ipc.c \
						mm_sound_mgr_pulse.c \
						mm_sound_mgr_run.c \
						mm_sound_plugin.c \
						mm_sound_server.c \
						mm_sound_thread_pool.c \
						mm_sound_recovery.c

sound_server_CFLAGS = -I$(srcdir)/../include \
					  -I$(srcdir)/include \
					  $(MMCOMMON_CFLAGS) \
					  $(MMLOGSVR_CFLAGS) \
					  -DMMF_LOG_OWNER=0x020 \
					  -DMMF_DEBUG_PREFIX=\"MMF-SOUND\" \
					  $(MMSESSION_CFLAGS) \
					  $(AVSYSTEM_CFLAGS) \
			  		  $(AUDIOSESSIONMGR_CFLAGS) \
			  		  $(VCONF_CFLAGS) \
			  		  $(GLIB2_CFLAGS) \
			  		  $(HEYNOTI_CFLAGS)

sound_server_LDADD = $(MMLOGSVR_LIBS) \
		     -ldl \
		     $(MMCOMMON_LIBS) \
		     $(MMSESSION_LIBS) \
		     $(AVSYSTEM_LIBS) \
			 $(AUDIOSESSIONMGR_LIBS) \
			 $(GLIB2_LIBS) \
			 $(VCONF_LIBS) \
			 $(HEYNOTI_LIBS) \
			 $(srcdir)/../common/libmmfsoundcommon.la 

if USE_PULSE
sound_server_CFLAGS += -DPULSE_CLIENT $(PULSE_CFLAGS)
sound_server_LDADD += $(PULSE_LIBS)
endif

sound_server_DEPENDENCIES = $(srcdir)/../common/libmmfsoundcommon.la
