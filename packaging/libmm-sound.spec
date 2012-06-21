Name:       libmm-sound
Summary:    MMSound Package contains client lib and sound_server binary
Version:    0.5.11
Release:    1
Group:      Libraries/Sound
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sound-server.service
Source1001: packaging/libmm-sound.manifest 
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

%package tools
Summary: MMSound utility package - contians mm_sound_testsuite, sound_check
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}-%{release}

%description tools
MMSound utility package - contians mm_sound_testsuite, sound_check for sound system



%prep
%setup -q


%build
cp %{SOURCE1001} .
CFLAGS="%{optflags} -fvisibility=hidden -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\""; export CFLAGS
./autogen.sh
%configure  --enable-pulse

make %{?_smp_mflags}

%install
%make_install
install -d %{buildroot}%{_libdir}/systemd/user/tizen-middleware.target.wants
install -m0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/user/
ln -sf ../sound-server.service %{buildroot}%{_libdir}/systemd/user/tizen-middleware.target.wants/sound-server.service

# FIXME: remove after systemd is in
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s ../init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S40soundserver
ln -s ../init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S40soundserver


%post
/sbin/ldconfig
/usr/bin/vconftool set -t int db/volume/system 5 -g 29
/usr/bin/vconftool set -t int db/volume/notification 7 -g 29
/usr/bin/vconftool set -t int db/volume/alarm 6 -g 29
/usr/bin/vconftool set -t int db/volume/ringtone 13 -g 29
/usr/bin/vconftool set -t int db/volume/media 7 -g 29
/usr/bin/vconftool set -t int db/volume/call 7 -g 29
/usr/bin/vconftool set -t int db/volume/fixed 0 -g 29
/usr/bin/vconftool set -t int db/volume/java 11 -g 29
/usr/bin/vconftool set -t int memory/Sound/RoutePolicy 0 -i -g 29

%postun -p /sbin/ldconfig


%files
%manifest libmm-sound.manifest
%attr(0755,root,root) %{_sysconfdir}/rc.d/init.d/soundserver
%{_sysconfdir}/rc.d/rc3.d/S40soundserver
%{_sysconfdir}/rc.d/rc4.d/S40soundserver
%{_bindir}/sound_server
%{_libdir}/libmmfsound.so.*
%{_libdir}/libsoundplugintone.so.*
%{_libdir}/libmmfsoundcommon.so.*
%{_libdir}/libsoundpluginwave.so.*
%{_libdir}/libsoundpluginkeytone.so.*
%{_libdir}/libmmfkeysound.so.*
%{_libdir}/libsoundpluginheadset.so.*
%{_libdir}/soundplugins/libsoundpluginktone.so
%{_libdir}/soundplugins/libsoundpluginheadset.so
%{_libdir}/soundplugins/libsoundpluginwave.so
%{_libdir}/soundplugins/libsoundpluginkeytone.so
%{_libdir}/systemd/user/tizen-middleware.target.wants/sound-server.service
%{_libdir}/systemd/user/sound-server.service
%{_libdir}/libmmfkeysound.so
%{_libdir}/libmmfsound.so
%{_libdir}/libsoundpluginheadset.so
%{_libdir}/libsoundplugintone.so
%{_libdir}/libmmfsoundcommon.so
%{_libdir}/libsoundpluginwave.so
%{_libdir}/libsoundpluginkeytone.so

%files devel
%manifest libmm-sound.manifest
%{_includedir}/mmf/mm_sound_private.h


%files sdk-devel
%manifest libmm-sound.manifest
%{_includedir}/mmf/mm_sound.h
%{_libdir}/pkgconfig/mm-keysound.pc
%{_libdir}/pkgconfig/mm-sound.pc

%files tools
%manifest libmm-sound.manifest
%{_bindir}/mm_sound_testsuite
