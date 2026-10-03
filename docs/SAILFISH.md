# Building for Sailfish OS

The Sailfish OS app lives in `sailfish/`. It is a Qt Quick / Silica app on top of the same core libraries as the
other platforms. Sailfish OS ships Qt 5.6, so the shared Qt code keeps Qt 5 compatible paths for it
(`libs/platform/http_client_qt.cpp`, `libs/platform/location_service/qt_location_service.cpp`), and rendering uses
OpenGL ES 3.

## Requirements

- Sailfish SDK 5.2 or newer: the core needs C++23, and the GCC 10 of older SDKs can't build it.
  Without the SDK installed, the Docker image `coderus/sailfishos-platform-sdk-aarch64:5.2.0.15` works too.
- The submodules: `git submodule update --init --recursive`.

## Build

From the repository root, in the SDK (for the Docker image, mount the repository under `/home/mersdk/src`):

```bash
mb2 -t SailfishOS-5.2.0.15-aarch64 --specfile sailfish/rpm/organicmaps.spec build -j$(nproc)
```

The RPM is written to `RPMS/`. A first build takes over an hour, rebuilds reuse the `build/` directory.

The CMake configuration is selected with `-DSAILFISH=ON` (see `sailfish/rpm/organicmaps.spec`), which defines
`PLATFORM_SAILFISH` in CMake and `OMIM_OS_SAILFISH` in C++. Sailfish is a Linux platform, so `OMIM_OS_LINUX` and
`PLATFORM_LINUX` are set as well.

## Install

Copy the RPM to the device and install it there:

```bash
scp RPMS/organicmaps-*.aarch64.rpm defaultuser@<device>:
ssh defaultuser@<device> devel-su pkcon install-local organicmaps-*.aarch64.rpm
```

## Notes

- UI strings come from `data/strings/strings.txt` and `data/strings/types_strings.txt`, read at runtime by
  `libs/platform/localization_sailfish.cpp`.
- The SVG icons are converted from the Android vector drawables at build time by
  `sailfish/tools/android_vector_to_svg.py`.
- Voice guidance uses Speech Note over D-Bus when installed, or a local speech synthesizer such as espeak-ng.
- The app runs in Sailjail with the permissions in `sailfish/organicmaps.profile` and `sailfish/organicmaps.desktop`.
