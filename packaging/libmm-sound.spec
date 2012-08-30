Name:       libmm-sound
Summary:    MMSound Package contains client lib and sound_server binary
Version:    0.6.0
Release:    17
Group:      System/Libraries
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Requires(pre): /bin/pidof
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(avsystem)
BuildRequires: pkgconfig(mm-log)
BuildRequires: pkgconfig(mm-session)
BuildRequires: pkgconfig(audio-session-mgr)
BuildRequires: pkgconfig(sysman)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(heynoti)
BuildRequires:  pkgconfig(security-server)

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
./autogen.sh
%ifarch %{arm}
CFLAGS="%{optflags} -fvisibility=hidden -DMM_DEBUG_FLAG -DSEPARATE_EARPHONE_VOLUME -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\""; export CFLAGS
%else
CFLAGS="%{optflags} -fvisibility=hidden -DMM_DEBUG_FLAG -DSEPARATE_EARPHONE_VOLUME -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\""; export CFLAGS
%endif
%configure --prefix=/usr --enable-pulse --enable-security
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc5.d
ln -s %{_sysconfdir}/rc.d/init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S23soundserver
ln -s %{_sysconfdir}/rc.d/init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S23soundserver



%post
/sbin/ldconfig

# -DSEPARATE_EARPHONE_VOLUME
/usr/bin/vconftool set -t int db/private/sound/volume/system 1285 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/notification 1799 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/alarm 1799 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/ringtone 3341 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/media 1799 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/call 1799 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/fixed 0 -g 29
/usr/bin/vconftool set -t int db/private/sound/volume/java 3084 -g 29

# No -DSEPARATE_EARPHONE_VOLUME
#/usr/bin/vconftool set -t int db/private/sound/volume/system 5 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/notification 7 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/alarm 7 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/ringtone 13 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/media 7 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/call 7 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/fixed 0 -g 29
#/usr/bin/vconftool set -t int db/private/sound/volume/java 11 -g 29

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_bindir}/sound_server
%{_libdir}/libmmfsound.so.*
%{_libdir}/libmmfsoundcommon.so.*
%{_libdir}/libmmfkeysound.so.*
%{_libdir}/libsoundplugintone.so*
%{_libdir}/libsoundpluginwave.so*
%{_libdir}/libsoundpluginkeytone.so*
%{_libdir}/soundplugins/libsoundplugintone.so
%{_libdir}/soundplugins/libsoundpluginwave.so
%{_libdir}/soundplugins/libsoundpluginkeytone.so
%{_sysconfdir}/rc.d/init.d/soundserver
%{_sysconfdir}/rc.d/rc3.d/S23soundserver
%{_sysconfdir}/rc.d/rc4.d/S23soundserver

%files devel
%defattr(-,root,root,-)
%{_libdir}/libmmfkeysound.so
%{_libdir}/libmmfsound.so
%{_libdir}/libmmfsoundcommon.so
%{_includedir}/mmf/mm_sound_private.h


%files sdk-devel
%defattr(-,root,root,-)
%{_includedir}/mmf/mm_sound.h
%{_libdir}/pkgconfig/mm-keysound.pc
%{_libdir}/pkgconfig/mm-sound.pc

%files tool
%defattr(-,root,root,-)
%{_bindir}/mm_sound_testsuite
