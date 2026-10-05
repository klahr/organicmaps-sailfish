Name:       organicmaps
Summary:    Offline maps and navigation based on OpenStreetMap
# The date based version of the other platforms, see tools/unix/version.sh.
Version:    %(bash tools/unix/version.sh ios_version)
Release:    %(bash tools/unix/version.sh count)
# The map data files are under DATA_LICENSE.txt.
License:    Apache-2.0
URL:        https://organicmaps.app
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  cmake
# For tools/unix/version.sh, which falls back to release 0 without git.
BuildRequires:  git
BuildRequires:  ninja
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Positioning)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Sensors)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(glesv2)
# QML modules, the linked Qt libraries are found automatically.
Requires:       sailfishsilica-qt5
Requires:       sailfishshare-components
Requires:       libkeepalive

%description
Organic Maps is a privacy-focused offline maps and navigation app based on OpenStreetMap data.

%prep
%setup -q -n %{name}-%{version}

%build
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DSAILFISH=ON \
  -DBUILD_TESTING=OFF
cmake --build build --target organicmaps_sailfish

%install
# Only the app; the bundled 3party libraries carry their own install rules.
DESTDIR=%{buildroot} cmake -P build/sailfish/cmake_install.cmake
# brp-strip skips shared objects, which would otherwise ship with ~300 MB of debug info.
%{__strip} --strip-unneeded %{buildroot}%{_datadir}/%{name}/lib/liborganicmaps.so

%files
%license LICENSE DATA_LICENSE.txt
%{_bindir}/organicmaps
%{_datadir}/organicmaps
%{_datadir}/applications/organicmaps.desktop
%{_datadir}/dbus-1/services/app.organicmaps.organicmaps.service
%{_sysconfdir}/sailjail/permissions/organicmaps.profile
%{_datadir}/icons/hicolor/*/apps/organicmaps.png
