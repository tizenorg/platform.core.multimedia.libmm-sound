lib_LTLIBRARIES = libsoundpluginheadset.la

libsoundpluginheadset_la_SOURCES = mm_sound_plugin_run_headset.c

libsoundpluginheadset_la_CFLAGS  = \
				$(MMCOMMON_CFLAGS) \
				$(AVSYSTEM_CFLAGS) \
				$(VCONF_CFLAGS) \
				$(AUDIOSESSIONMGR_CFLAGS)\
      				-I$(srcdir)/../../../include \
      				$(PULSE_CFLAGS) \
				$(MMLOGSVR_CFLAGS) -DMMF_LOG_OWNER=0x020 -DMMF_DEBUG_PREFIX=\"MMF-SOUND\"

libsoundpluginheadset_la_LIBADD  =  \
				$(MMLOGSVR_LIBS) \
				$(AVSYSTEM_LIBS) \
				$(MMCOMMON_LIBS) \
				$(VCONF_LIBS) \
				$(AUDIOSESSIONMGR_LIBS) \
				$(PULSE_LIBS) \
				$(srcdir)/../../../common/libmmfsoundcommon.la

install-exec-hook:
	mkdir -p $(DESTDIR)$(libdir)/soundplugins
	ln -sf /usr/lib/libsoundpluginheadset.so $(DESTDIR)$(libdir)/soundplugins/libsoundpluginheadset.so

