Name:       organicmaps
Summary:    Offline maps and navigation based on OpenStreetMap
Version:    0.1
Release:    1
License:    ASL 2.0
URL:        https://organicmaps.app
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  cmake
BuildRequires:  ninja
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Positioning)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(glesv2)
Requires:       sailfishsilica-qt5
Requires:       qt5-qtpositioning
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
  -DSKIP_QT_GUI=ON \
  -DBUILD_TESTING=OFF \
  -DSKIP_TOOLS=ON
cmake --build build --target organicmaps_sailfish

%install
# Only the app; the bundled 3party libraries carry their own install rules.
DESTDIR=%{buildroot} cmake -P build/sailfish/cmake_install.cmake

%files
%{_bindir}/organicmaps
%{_datadir}/organicmaps
%{_datadir}/applications/organicmaps.desktop
%{_datadir}/icons/hicolor/*/apps/organicmaps.png
