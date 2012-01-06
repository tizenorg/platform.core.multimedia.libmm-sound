lib_LTLIBRARIES = libsoundplugindtmf.la

libsoundplugindtmf_la_SOURCES = mm_sound_plugin_codec_dtmf.c

libsoundplugindtmf_la_CFLAGS  = \
				$(MMCOMMON_CFLAGS) \
				-I$(srcdir)/../../../../common/include \
      			       -I$(srcdir)/../../../include \
				$(MMLOGSVR_CFLAGS) -DMMF_LOG_OWNER=0x020 -DMMF_DEBUG_PREFIX=\"MMF-SOUND\"

libsoundplugindtmf_la_LIBADD  = \
				$(MMCOMMON_LIBS) \
				$(MMLOGSVR_LIBS)

libsoundplugindtmf_la_CFLAGS += $(AVSYSTEM_CFLAGS)
libsoundplugindtmf_la_LIBADD += $(AVSYSTEM_LIBS)

install-exec-hook:
	mkdir -p $(DESTDIR)$(libdir)/soundplugins
	ln -sf /usr/lib/libsoundplugindtmf.so $(DESTDIR)$(libdir)/soundplugins/libsoundpluginkdtmf.so

