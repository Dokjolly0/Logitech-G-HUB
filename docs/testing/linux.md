# Testing on Linux (Ubuntu 26.04)

Step-by-step procedure to build periph, run the unit tests and run the hardware tests against the G502 LIGHTSPEED on
Ubuntu. The reference setup is a dual boot with the Windows development machine. General rules for hardware tests
are in [hardware.md](hardware.md).

## 1. Install the toolchain

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config libudev-dev libhidapi-dev googletest
```

Check the versions: GCC ≥ 14 (needed for `<print>` and `std::expected`) and CMake ≥ 3.25.

```sh
g++ --version
cmake --version
```

Reference versions on Ubuntu 26.04: GCC 15.2, CMake 4.2, Ninja 1.13, hidapi 0.15.0, GoogleTest 1.17.0.

CMake prefers system packages. Ubuntu's `libhidapi-dev` ships only a pkg-config file (`hidapi-hidraw.pc`), which
`cmake/PeriphDependencies.cmake` picks up; the configure output then says
`hidapi: system hidapi-hidraw 0.15.0 (pkg-config)`. Without the packages CMake downloads the pinned releases
(hidapi 0.14.0, GoogleTest 1.15.2) on the first configure, which needs network access, and building that hidapi
needs `libudev-dev`. `libudev-dev` is also needed later for hotplug (task 0.3).

## 2. Get the sources

Clone the repository into the Linux file system (any ext4 path works, e.g. `~/src/periph` or `/projects/…`). Do not
build directly on the Windows NTFS partition: it is slower, and Windows Fast Startup or hibernation leaves it mounted
read-only.

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
after the permissions when an ACL is set (`crw-rw----+`). Non-Logitech nodes (touchpad, …) stay `crw-------`.

The rule matches the receiver nodes and the per-device nodes that `hid-logitech-dj` creates under them (their parent is
also `0003:046D:*`).

## 4. Build and run the unit tests

```sh
cmake --preset linux-gcc
cmake --build --preset linux-gcc
ctest --preset linux-gcc
```

Expected: every `unit` test passes (32 as of task 0.1), and the 3 `hw` tests show as *Skipped*.

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

Expected **wireless** (observed on 2026-11-11, kernel 7.0):

```text
== USB Receiver (PID C539, 1 collection(s))
  receiver (HID++ 1.0); scanning paired slots
  [index 0x01] G502 LIGHTSPEED Wireless Gaming Mouse (mouse) - HID++ 4.2
    features (30):
      …
      #29  0x2121 v0 HiResWheel
    battery: 4033 mV (via 0x1001)
    dpi: 6400 (default 800), supported: 100-25600 step 50
    report rate: 1000 Hz, supported: 1000 Hz, 500 Hz, 250 Hz, 125 Hz
  [index 0x02] HID++1.0 error 0x08 (unknown device)
  …
== Logitech G502 (PID 407F, 1 collection(s), hid-logitech-dj slot 1)
  [index 0x01] G502 LIGHTSPEED Wireless Gaming Mouse (mouse) - HID++ 4.2
  … same features and readings …
```

Expected **by cable** (observed on 2026-11-11): one endpoint `PID C08D` with
`[index 0xFF] G502 LIGHTSPEED … HID++ 4.2`, 30 features (`0x00C2` DfuControlSigned at index 13, no `0x2121`),
`battery: 4211 mV, charging`, DPI and report rate. While the mouse runs on the cable the receiver shows slot 1 as
`HID++1.0 error 0x09`, and the `407F` node stays listed but answers with the same offline error.

Other Logitech receivers on the machine (for example a Unifying receiver `C52B`) are listed too. Errors on their slots
are normal for devices that are off or asleep.

### How Linux differs from Windows

- **One node per interface.** Windows splits short and long HID++ reports into two collections; on Linux one hidraw
  node carries both. `periph-probe` therefore reports `1 collection(s)` per endpoint, which is normal.
- **`hid-logitech-dj` driver.** For Unifying and Lightspeed receivers the kernel creates an extra hidraw node per
  paired, connected device: the G502 appears as its own endpoint (PID `407F`, name `Logitech G502`) next to the
  receiver (`C539`). On that node the kernel replaces the device index of every request with the slot, so it answers
  only at index `0x01`, never at `0xFF`. periph reads the slot from `HID_PHYS` in sysfs (`…/input2:1`) and stores it
  in `Endpoint::device_index`. Both paths work and report the same feature table.
- **Power saving.** When idle the mouse sleeps and can drop the first request over the receiver. `Device::probe`
  retries the ping up to 3 times (400 ms each), so the first run after idle can take up to 0.8 s longer.
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

`HwProbe.FindsG502WithExpectedFeatures` prints the path it used (`[ hw ] G502 via cable|receiver|…`).

Run the suite twice, as required by the test matrix:

1. **Cable**: G502 connected by a USB **data** cable. Check first that `lsusb | grep 046d:c08d` prints a line; if it
   does not, the data link did not come up and the mouse keeps running on the receiver (see troubleshooting).
   `HwProbe.WirelessPathsReportTheSameFeatureTable` is skipped in this run.
2. **Receiver**: unplug the cable, switch the mouse on, keep the Lightspeed receiver plugged in. `find_g502()` falls
   back to the receiver (slot 1) when no wired device is found; the wireless-paths test also probes the `407F` node.

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
| `Package 'libudev' not found` during configure | CMake is building the downloaded hidapi: `sudo apt install libudev-dev libhidapi-dev`, then delete `build/linux-gcc` and configure again. |
| Cable plugged in, mouse charging, but no `046d:c08d` in `lsusb` and nothing in `journalctl -k` | The USB data link did not come up (seen once with the original cable; replugging fixed it). Unplug and replug the cable, then try another port (not a hub). The mouse keeps working through the receiver meanwhile. |
| `[index 0x01] timeout` on the receiver, but the next run works | The mouse was asleep and woke up too late. Move it and retry; probe already retries the ping 3 times. |
| `no answer at index 0xFF` on a `407F`/`402D` endpoint | A build older than the DJ-node fix, which pinged `0xFF`. Rebuild. |
| `I/O error: Broken pipe` on a slot of another receiver | The receiver stalled the request (seen on a Unifying `C52B` with its device asleep). It does not affect the G502. |
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
