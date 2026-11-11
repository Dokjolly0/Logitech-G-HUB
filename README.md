# periph

A fast, cross-platform (Windows, Linux) configuration tool for Logitech gaming peripherals, written from scratch in C++23.
The goal is full parity with every setting G HUB exposes for the **G502 LIGHTSPEED**, without G HUB. Firmware updates are out of scope.

> Independent project, not affiliated with Logitech. It talks to devices through the HID++ protocol;
> it contains no Logitech code, assets or data files.

## Status

Phase 0 (foundations). See [docs/roadmap.md](docs/roadmap.md).

| Component | State |
|---|---|
| `periph_core` HID++ transport, feature discovery, battery/DPI/report-rate reads | ✅ |
| `periph-probe` read-only diagnostic | ✅ |
| Verified on hardware | Windows 11 (cable), Ubuntu 26.04 (cable, receiver and `hid-logitech-dj` node) |
| `periphd` daemon, `periphctl` CLI, Qt UI | planned |

## Build

Requirements: CMake ≥ 3.25 and a C++23 compiler (MSVC 17.10+, GCC 14+, Clang 18+).
hidapi and GoogleTest come from system packages when available (on Linux hidapi is also found through pkg-config),
otherwise CMake downloads pinned releases.

```sh
# Windows (Visual Studio 2022)
cmake --preset win-msvc
cmake --build --preset win-msvc
ctest --preset win-msvc

# Linux (Ubuntu: sudo apt install build-essential cmake ninja-build pkg-config libudev-dev libhidapi-dev googletest)
cmake --preset linux-gcc
cmake --build --preset linux-gcc
ctest --preset linux-gcc
```

On Linux, install the udev rule once so that no root access is needed:

```sh
sudo cp packaging/linux/70-periph-logitech.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger
```

## Try it

```sh
periph-probe            # list HID++ interfaces, devices, features and readings
periph-probe --trace    # also print raw HID++ frames
```

`periph-probe` only reads; it never changes device settings.

## Documentation

- [Architecture](docs/architecture.md)
- [Implementation plan](docs/plan.md) and [roadmap](docs/roadmap.md) (progress)
- [Hardware testing](docs/testing/hardware.md) and [testing on Linux](docs/testing/linux.md)
- [Reverse-engineering notes on G HUB](docs/re/FINDINGS.md) (interoperability only)
