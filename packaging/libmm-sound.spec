Name:       libmm-sound
Summary:    MMSound Package contains client lib and sound_server binary
Version:    0.9.202
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sound-server.service
Source2:    sound-server.path
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(mm-log)
BuildRequires: pkgconfig(mm-session)
BuildRequires: pkgconfig(audio-session-mgr)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(libpulse)
BuildRequires: pkgconfig(iniparser)
%ifarch %{arm}
%endif
BuildRequires: pkgconfig(libtremolo)

%description
MMSound Package contains client lib and sound_server binary for sound system


%package devel
Summary: MMSound development package
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
MMSound development package for sound system

%package sdk-devel
Summary: MMSound development package
Group:      Development/Libraries
Requires:   %{name}-devel = %{version}-%{release}

%description sdk-devel
MMSound development package for sound system

%package tool
Summary: MMSound utility package - contians mm_sound_testsuite, sound_check
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}-%{release}

%description tool
MMSound utility package - contians mm_sound_testsuite, sound_check for sound system

%prep
%setup -q

%build
%define tizen_audio_feature_ogg_enable 1

%ifarch %{arm}
	CFLAGS="%{optflags} -fvisibility=hidden -DSUPPORT_CONTAINER -D_TIZEN_PUBLIC_ -DUSE_FOCUS -DMM_DEBUG_FLAG -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"" ;export CFLAGS
%else
	CFLAGS="%{optflags} -fvisibility=hidden -DSUPPORT_CONTAINER -D_TIZEN_PUBLIC_ -DUSE_FOCUS -DMM_DEBUG_FLAG -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"" ;export CFLAGS
%endif

%if "%{?tizen_profile_name}" == "wearable"
	CFLAGS+=" -DTIZEN_MICRO";export CFLAGS
%endif

./autogen.sh
%configure \
%if 0%{?tizen_audio_feature_ogg_enable}
       --enable-ogg \
%endif
%ifarch %{arm}
	--prefix=/usr --enable-pulse --enable-focus --disable-security
%else
	--prefix=/usr --enable-pulse --enable-focus --disable-security
%endif

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}
cp LICENSE.APLv2 %{buildroot}/usr/share/license/libmm-sound-tool
mkdir -p %{buildroot}/opt/etc/dump.d/module.d/
cp dump_audio.sh %{buildroot}/opt/etc/dump.d/module.d/dump_audio.sh

%make_install
install -d %{buildroot}%{_unitdir}/multi-user.target.wants
install -m0644 %{SOURCE1} %{buildroot}%{_unitdir}/
install -m0644 %{SOURCE2} %{buildroot}%{_unitdir}/
ln -sf ../sound-server.path %{buildroot}%{_unitdir}/multi-user.target.wants/sound-server.path

%post
/sbin/ldconfig

/usr/bin/vconftool set -t int memory/private/Sound/ASMReady 0 -g 29 -f -i -s system::vconf_multimedia
/usr/bin/vconftool set -t int memory/private/Sound/VoiceControlOn 0 -g 29 -f -i -s system::vconf_multimedia
#SPK REC EAR BTS BTA DOCK HDMI MIR USB MDOCK
#/usr/bin/vconftool set -t string file/private/sound/volume/system "09090909090909090909" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/notification "11" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/alarm "7" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/ringtone "11" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/media "07070907070707070707" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/call "04040404040404040404" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/voip "04040404040404040404" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/voice "07070707070707070707" -g 29 -f
#/usr/bin/vconftool set -t string file/private/sound/volume/fixed "00000000000000000000" -g 29 -f
/usr/bin/vconftool set -t int memory/private/sound/PrimaryVolumetypeForce -1 -g 29 -f -i
/usr/bin/vconftool set -t int memory/private/sound/PrimaryVolumetype -1 -g 29 -f -i
/usr/bin/vconftool set -t int memory/private/sound/hdmisupport 0 -g 29 -f -i
/usr/bin/vconftool set -t int memory/factory/loopback 0 -g 29 -f -i

/usr/bin/vconftool set -t int file/private/sound/volume/system 9 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/notification 11 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/alarm 7 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/ringtone 11 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/media 7 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/call 4 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/voip 4 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/voice 7 -g 29 -f -s User
/usr/bin/vconftool set -t int file/private/sound/volume/fixed 0 -g 29 -f -s User


%postun -p /sbin/ldconfig


%files
%manifest libmm-sound.manifest
%defattr(-,root,root,-)
%caps(cap_chown,cap_dac_override,cap_fowner,cap_mac_override,cap_lease=eip) %{_bindir}/sound_server
%{_libdir}/libmmfsound.so.*
%{_libdir}/libmmfsoundcommon.so.*
%{_libdir}/libmmfkeysound.so.*
%{_libdir}/libmmfbootsound.so.*
%{_libdir}/libsoundplugintone.so*
%{_libdir}/libsoundpluginwave.so*
%{_libdir}/libsoundpluginkeytone.so*
%if 0%{?tizen_audio_feature_ogg_enable}
%{_libdir}/libsoundplugintremoloogg.so*
%endif
%{_libdir}/soundplugins/libsoundplugintone.so
%{_libdir}/soundplugins/libsoundpluginwave.so
%{_libdir}/soundplugins/libsoundpluginkeytone.so
%if 0%{?tizen_audio_feature_ogg_enable}
%{_libdir}/soundplugins/libsoundplugintremoloogg.so
%endif
%{_unitdir}/multi-user.target.wants/sound-server.path
%{_unitdir}/sound-server.service
%{_unitdir}/sound-server.path
/usr/share/sounds/sound-server/*
%{_datadir}/license/%{name}
%{_datadir}/license/libmm-sound-tool
/usr/share/sounds/sound-server/*
/opt/etc/dump.d/module.d/dump_audio.sh

%files devel
%defattr(-,root,root,-)
%{_libdir}/libmmfkeysound.so
%{_libdir}/libmmfbootsound.so
%{_libdir}/libmmfsound.so
%{_libdir}/libmmfsoundcommon.so
%{_includedir}/mmf/mm_sound_private.h
%exclude %{_includedir}/mmf/mm_sound_pa_client.h

%files sdk-devel
%defattr(-,root,root,-)
%{_includedir}/mmf/mm_sound.h
%{_includedir}/mmf/mm_sound_pcm_async.h
%{_includedir}/mmf/mm_sound_focus.h
%exclude %{_includedir}/mmf/mm_sound_pa_client.h
%{_libdir}/pkgconfig/mm-keysound.pc
%{_libdir}/pkgconfig/mm-bootsound.pc
%{_libdir}/pkgconfig/mm-sound.pc

%files tool
%manifest libmm-sound-tool.manifest
%defattr(-,root,root,-)
%{_bindir}/mm_sound_testsuite
