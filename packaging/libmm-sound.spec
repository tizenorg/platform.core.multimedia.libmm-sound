Name:       libmm-sound
Summary:    MMSound Package contains client lib and sound_server binary
Version:    0.10.28
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sound-server.service
Source2:    sound-server.path
Source3:    sound-server.conf
Source4:    focus-server.service
Source5:    focus-server.path
Source6:    focus-server.conf
Requires: security-config
%if "%{?tizen_profile_name}" == "tv"
Source7:    sound-server-tv.service
Source8:    focus-server-tv.service
%endif
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(mm-log)
BuildRequires: pkgconfig(mm-session)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(libpulse)
BuildRequires: pkgconfig(iniparser)
%if "%{?tizen_profile_name}" != "tv"
BuildRequires: pkgconfig(capi-network-bluetooth)
%endif
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

%if "%{?tizen_profile_name}" == "tv"
%define tizen_audio_feature_bluetooth_enable 0
%else
%define tizen_audio_feature_bluetooth_enable 1
%endif

%ifarch %{arm}
	CFLAGS="%{optflags} -fvisibility=hidden -DSUPPORT_CONTAINER -D_TIZEN_PUBLIC_ -DUSE_FOCUS -DMM_DEBUG_FLAG -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"" ;export CFLAGS
%else
	CFLAGS="%{optflags} -fvisibility=hidden -DSUPPORT_CONTAINER -D_TIZEN_PUBLIC_ -DUSE_FOCUS -DMM_DEBUG_FLAG -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"" ;export CFLAGS
%endif

%if "%{?tizen_profile_name}" == "wearable"
	CFLAGS+=" -DTIZEN_MICRO";export CFLAGS
%endif
%if "%{?tizen_profile_name}" == "tv"
        CFLAGS+=" -DTIZEN_TV";export CFLAGS
%endif

./autogen.sh
%configure \
%if 0%{?tizen_audio_feature_ogg_enable}
       --enable-ogg \
       --with-plugindir=%{_libdir}/soundplugins/ \
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
mkdir -p %{buildroot}/etc/dbus-1/system.d/
cp %{SOURCE3} %{buildroot}/etc/dbus-1/system.d/sound-server.conf
cp %{SOURCE6} %{buildroot}/etc/dbus-1/system.d/focus-server.conf
%if "%{?tizen_profile_name}" == "tv"
cp %{SOURCE7} %{SOURCE1}
cp %{SOURCE8} %{SOURCE4}
%endif

%make_install
%if "%{?tizen_profile_name}" == "tv"
install -d %{buildroot}%{_unitdir}/sysinit.target.wants
%else
install -d %{buildroot}%{_unitdir}/multi-user.target.wants
%endif
install -m0644 %{SOURCE1} %{buildroot}%{_unitdir}/
install -m0644 %{SOURCE2} %{buildroot}%{_unitdir}/
install -m0644 %{SOURCE4} %{buildroot}%{_unitdir}/
install -m0644 %{SOURCE5} %{buildroot}%{_unitdir}/
%if "%{?tizen_profile_name}" == "tv"
ln -sf ../sound-server.path %{buildroot}%{_unitdir}/sysinit.target.wants/sound-server.path
ln -sf ../focus-server.path %{buildroot}%{_unitdir}/sysinit.target.wants/focus-server.path
%else
ln -sf ../sound-server.path %{buildroot}%{_unitdir}/multi-user.target.wants/sound-server.path
ln -sf ../focus-server.path %{buildroot}%{_unitdir}/multi-user.target.wants/focus-server.path
%endif
%post
/sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest libmm-sound.manifest
%defattr(-,root,root,-)
%caps(cap_chown,cap_dac_override,cap_fowner,cap_mac_override,cap_lease=eip) %{_bindir}/sound_server
%caps(cap_chown,cap_dac_override,cap_fowner,cap_mac_override,cap_lease=eip) %{_bindir}/focus_server
%{_bindir}/focus_server
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
%if "%{?tizen_profile_name}" == "tv"
%{_unitdir}/sysinit.target.wants/sound-server.path
%{_unitdir}/sysinit.target.wants/focus-server.path
%else
%{_unitdir}/multi-user.target.wants/sound-server.path
%{_unitdir}/multi-user.target.wants/focus-server.path
%endif
%{_unitdir}/sound-server.service
%{_unitdir}/sound-server.path
%{_unitdir}/focus-server.service
%{_unitdir}/focus-server.path
/usr/share/sounds/sound-server/*
%{_datadir}/license/%{name}
%{_datadir}/license/libmm-sound-tool
/usr/share/sounds/sound-server/*
/etc/dbus-1/system.d/sound-server.conf
/etc/dbus-1/system.d/focus-server.conf

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
%{_includedir}/mmf/mm_sound_focus.h
%{_includedir}/mmf/mm_sound_device.h
%exclude %{_includedir}/mmf/mm_sound_pa_client.h
%{_libdir}/pkgconfig/mm-keysound.pc
%{_libdir}/pkgconfig/mm-bootsound.pc
%{_libdir}/pkgconfig/mm-sound.pc

%files tool
%manifest libmm-sound-tool.manifest
%defattr(-,root,root,-)
%{_bindir}/mm_sound_testsuite
