Name:       libmm-sound
Summary:    MMSound Package contains client lib and sound_server binary
Version:    0.4.1
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Requires(pre): /bin/pidof
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
BuildRequires: vconf-keys-devel
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
CFLAGS="%{optflags} -fvisibility=hidden -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\""; export CFLAGS
%configure --prefix=/usr --enable-sdk --enable-pulse

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s %{_sysconfdir}/init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S40soundserver
ln -s %{_sysconfdir}/init.d/soundserver %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S40soundserver

%pre
#PID=`/bin/pidof sound_server`
#if [ -n "$PID" ]; then
#    echo "preinst: kill current sound server"
#    /usr/bin/killall -9 sound_server
#fi


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

if [ -n "`env|grep SBOX`" ]; then
    echo "postinst: sbox installation skip lauching daemon"
else
        echo "postinst: run sound server again"
        /usr/bin/sound_server -S&
fi

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
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
%{_sysconfdir}/rc.d/init.d/soundserver
%{_sysconfdir}/rc.d/rc3.d/S40soundserver
%{_sysconfdir}/rc.d/rc4.d/S40soundserver

%files devel
%defattr(-,root,root,-)
%{_libdir}/libmmfkeysound.so
%{_libdir}/libmmfsound.so
%{_libdir}/libsoundpluginheadset.so
%{_libdir}/libsoundplugintone.so
%{_libdir}/libmmfsoundcommon.so
%{_libdir}/libsoundpluginwave.so
%{_libdir}/libsoundpluginkeytone.so
%{_includedir}/mmf/mm_sound_private.h


%files sdk-devel
%defattr(-,root,root,-)
%{_includedir}/mmf/mm_sound.h
%{_libdir}/pkgconfig/mm-keysound.pc
%{_libdir}/pkgconfig/mm-sound.pc

%files tool
%defattr(-,root,root,-)
%{_bindir}/mm_sound_testsuite
