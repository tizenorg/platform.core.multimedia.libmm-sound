if IS_SDK
SUBDIRS = 	common \
		pkgconfig-sdk \
		. \
		server \
		testsuite
else
SUBDIRS = common \
		pkgconfig \
		. \
		server \
		testsuite
endif

SUBDIRS += init
		
lib_LTLIBRARIES = libmmfsound.la

includelibmmfsounddir = $(includedir)/mmf
includelibmmfsound_HEADERS = include/mm_sound.h \
							include/mm_sound_private.h

libmmfsound_la_SOURCES = mm_sound.c \
			mm_sound_client.c 

libmmfsound_la_DEPENDENCIES = common/libmmfsoundcommon.la
								

libmmfsound_la_CFLAGS = -I$(srcdir)/include \
			$(MMCOMMON_CFLAGS) \
			$(VCONF_CFLAGS) \
			-I$(srcdir)/servertype/include \
			$(AVSYSTEM_CFLAGS) \
			$(GLIB2_CFLAGS) \
			$(MMSESSION_CFLAGS) \
			$(AUDIOSESSIONMGR_CFLAGS) \
			-D_USE_INTERNAL_SOUND_PATH_ \
			$(VCONF_CFLAGS)

if USE_PULSE
libmmfsound_la_CFLAGS += -DPULSE_CLIENT
endif
			

libmmfsound_la_LIBADD = $(MMCOMMON_LIBS) \
			$(AVSYSTEM_LIBS) \
			$(MMSESSION_LIBS) \
			$(AUDIOSESSIONMGR_LIBS) \
			$(GLIB2_LIBS) \
			$(srcdir)/common/libmmfsoundcommon.la \
			$(VCONF_LIBS)
			
libmmfsound_la_LDFLAGS = -version-info 1:0:1

#For logmanager
if IS_SDK
else
libmmfsound_la_CFLAGS += $(MMLOGSVR_CFLAGS) -DMMF_LOG_OWNER=0x020 -DMMF_DEBUG_PREFIX=\"MMF-SOUND\"
libmmfsound_la_LIBADD += $(MMLOGSVR_LIBS)
endif


#For dtmf
installdtmfdir = /opt/system/
installdtmf_DATA = $(srcdir)/server/plugin/dtmf/dtmf_all.raw





lib_LTLIBRARIES += libmmfkeysound.la

includelibmmfkeysounddir = $(includedir)/mmf

libmmfkeysound_la_SOURCES = mm_sound_keysound.c

libmmfkeysound_la_CFLAGS = -I$(srcdir)/include \
			$(MMCOMMON_CFLAGS) \
			-D_USE_INTERNAL_SOUND_PATH_

libmmfkeysound_la_LIBADD = $(MMCOMMON_LIBS)
if IS_SDK
else
libmmfkeysound_la_CFLAGS += $(MMLOGSVR_CFLAGS) -DMMF_LOG_OWNER=0x020 -DMMF_DEBUG_PREFIX=\"MMF-SOUND\"
libmmfkeysound_la_LIBADD += $(MMLOGSVR_LIBS)
endif

