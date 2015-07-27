lib_LTLIBRARIES = libsoundplugintone.la

libsoundplugintone_la_SOURCES = mm_sound_plugin_codec_tone.c

libsoundplugintone_la_CFLAGS  = \
				$(MMCOMMON_CFLAGS) \
				-I$(srcdir)/../../../../common/include \
				-I$(srcdir)/../../../include \
				$(PA_CFLAGS) \
				$(MMLOGSVR_CFLAGS) -DMMF_LOG_OWNER=0x002 -DMMF_DEBUG_PREFIX=\"MMF-SOUND\"

libsoundplugintone_la_LIBADD  = \
				$(MMCOMMON_LIBS) \
				$(PA_LIBS) \
				$(srcdir)/../../../libmmfsound.la \
				$(srcdir)/../../../common/libmmfsoundcommon.la \
				$(MMLOGSVR_LIBS)

install-exec-hook:
	mkdir -p $(DESTDIR)$(libdir)/soundplugins
	ln -sf ../libsoundplugintone.so $(DESTDIR)$(libdir)/soundplugins/libsoundplugintone.so

