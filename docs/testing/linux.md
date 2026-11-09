# Testing on Linux (Ubuntu 26.04)

Step-by-step procedure to build periph, run the unit tests and run the hardware tests against the G502 LIGHTSPEED on
Ubuntu. The reference setup is a dual boot with the Windows development machine. General rules for hardware tests
are in [hardware.md](hardware.md).

## 1. Install the toolchain

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config libudev-dev
```

Check the versions: GCC ≥ 14 (needed for `<print>` and `std::expected`) and CMake ≥ 3.25.

```sh
g++ --version
cmake --version
```

Optional: `sudo apt install libhidapi-dev googletest`. If CMake finds them it uses the system packages; otherwise it
downloads the pinned releases (hidapi 0.14.0, GoogleTest 1.15.2) on the first configure, which needs network access.

## 2. Get the sources

Clone the repository into the Linux file system. Do not build directly on the Windows NTFS partition: it is slower,
and Windows Fast Startup or hibernation leaves it mounted read-only.

```sh
# From the Windows partition (path depends on where Ubuntu mounts it):
git clone /media/$USER/<windows-data-volume>/Projects/Logitech-G-HUB ~/src/periph
# or from a remote, once one exists:
# git clone <remote-url> ~/src/periph
cd ~/src/periph
```

## 3. Give your user access to the device

Linux exposes HID++ through `/dev/hidraw*` nodes, which are root-only by default. The rule in
`packaging/linux/70-periph-logitech.rules` grants access to the user logged in at the seat (`uaccess` tag) for every
Logitech hidraw node, USB (`0003:046D:*`) and Bluetooth (`0005:046D:*`).

```sh
sudo cp packaging/linux/70-periph-logitech.rules /etc/udev/rules.d/
sudo udevadm control --reload
sudo udevadm trigger
```

Unplug and replug the cable and the receiver, then check:

```sh
for d in /sys/class/hidraw/hidraw*; do
  printf '%s  ' "/dev/$(basename "$d")"; grep HID_NAME "$d/device/uevent"
done
getfacl /dev/hidraw* 2>/dev/null | grep -B1 "user:$USER"
```

Each Logitech node must list `user:<you>:rw-`. Without `getfacl` (package `acl`), `ls -l /dev/hidraw*` shows a `+`
after the permissions when an ACL is set.

## 4. Build and run the unit tests

```sh
cmake --preset linux-gcc
cmake --build --preset linux-gcc
ctest --preset linux-gcc
```

Expected: every `unit` test passes (24 as of task 0.1), and the `hw` tests show as *Skipped*.

Useful variations:

```sh
ctest --preset linux-gcc -L unit              # only unit tests
ctest --preset linux-gcc -R Channel           # tests whose name matches
build/linux-gcc/tests/periph-unit-tests --gtest_filter='G502Fixture.*'
```

## 5. Read-only check with periph-probe

```sh
build/linux-gcc/tools/probe/periph-probe
build/linux-gcc/tools/probe/periph-probe --trace   # also print raw frames
```

Expected with the cable plugged in: one endpoint `PID C08D` with `[index 0xFF] G502 LIGHTSPEED …`, 30 features, a
battery voltage, DPI and report rate. The Lightspeed receiver (`PID C539`) shows slot 1 offline while the mouse runs on
the cable.

### How Linux differs from Windows

- **One node per interface.** Windows splits short and long HID++ reports into two collections; on Linux one hidraw
  node carries both. `periph-probe` therefore reports `1 collection(s)` per endpoint, which is normal.
- **`hid-logitech-dj` driver.** For Unifying and Lightspeed receivers the kernel creates an extra hidraw node per
  paired, connected device. The G502 in wireless mode can appear as its own endpoint (PID `407F`) next to the
  receiver (`C539`). Both paths are valid; the receiver path uses slot index 1.
- **Other HID++ tools.** Stop Solaar (or anything else using HID++) while testing. Replies are matched by software id,
  but notifications and device state can still interfere.

## 6. Hardware tests

```sh
PERIPH_HW_TEST=1 build/linux-gcc/tests/periph-hwtest
# single test:
PERIPH_HW_TEST=1 build/linux-gcc/tests/periph-hwtest --gtest_filter='HwProbe.*'
```

G HUB does not exist on Linux, so the exclusive-access guard never triggers. Make sure Solaar and similar tools are
closed.

Run the suite twice, as required by the test matrix:

1. **Cable**: G502 connected by USB cable.
2. **Receiver**: unplug the cable, switch the mouse on, keep the Lightspeed receiver plugged in. `find_g502()` falls
   back to the receiver (slot 1) when no wired device is found.

Then record the result in the log table in [hardware.md](hardware.md) (date, task, `Ubuntu 26.04`, connection,
result).

## 7. Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `No Logitech HID++ interface found` | The udev rule is not active. Repeat step 3, replug the device, check the ACL. As a one-off check, run with `sudo` to confirm it is a permission problem. |
| `cannot open: …` or `Permission denied` | The node exists but your user has no access: the session is remote (SSH sessions do not get `uaccess`) or the rule did not match. Check `udevadm info -a /dev/hidrawN \| grep KERNELS`. |
| Endpoint found but every request times out | Another program holds the device in a conflicting state (Solaar), or the mouse is asleep: move it and retry. |
| Only the receiver appears, slot 1 offline | The mouse is off, on the cable, or paired with another receiver. |
| Configure fails downloading hidapi or GoogleTest | No network access: install `libhidapi-dev` and `googletest` from apt, or configure once while online. |
| `fatal error: print: No such file` | GCC older than 14: `sudo apt install g++-14` and set `CMAKE_CXX_COMPILER=g++-14`. |

Kernel messages about the device:

```sh
sudo dmesg | grep -iE 'logitech|hidraw' | tail -20
```

## 8. Later tasks (preview)

These steps arrive with their tasks and will be documented here then:

- **0.5:** `periphd` as a systemd `--user` service (`systemctl --user start periphd`).
- **2.5:** udev rule for `/dev/uinput`, needed for macros and remapping.
- **3.2:** GNOME Shell extension for active-window detection on Wayland.
- **4.3:** PipeWire ScreenCast portal permission for the screen sampler.
