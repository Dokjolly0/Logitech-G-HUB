# Hardware testing

Hardware tests live in `tests/hw/` and build into `periph-hwtest`. They are skipped unless `PERIPH_HW_TEST=1` is set.

## Requirements

- A G502 LIGHTSPEED attached by cable (`046D:C08D`) or through its Lightspeed receiver (`046D:C539`, slot 1). On Linux
  the mouse is also reachable through its own `hid-logitech-dj` node (`046D:407F`).
- By cable, the mouse must be enumerated as a USB device: `lsusb` must list `046d:c08d`. If it is charging but not
  listed, the data link did not come up and the mouse stays on the receiver; replug the cable.
- **Tests that write to the device need exclusive access.** Quit G HUB completely (tray icon → Quit) so that `lghub_agent.exe` is not running. Such tests call `PERIPH_HW_REQUIRE_EXCLUSIVE()` and fail if G HUB is still active.
- On Linux, install the udev rule from `packaging/linux/` first.

## Running

```powershell
# Windows (PowerShell)
$env:PERIPH_HW_TEST = '1'
build\win-msvc\tests\Release\periph-hwtest.exe
```

```sh
# Linux
PERIPH_HW_TEST=1 build/linux-gcc/tests/periph-hwtest
```

The full Ubuntu procedure (toolchain, udev permissions, cable and receiver runs, troubleshooting) is in
[linux.md](linux.md).

## Tests

| Test | Checks | Paths |
|---|---|---|
| `HwProbe.FindsG502WithExpectedFeatures` | HID++ ≥ 2.0, public features present, name contains "G502"; prints the path used | first found: cable, receiver, DJ node |
| `HwProbe.ReadingsAreInPlausibleRanges` | battery 3000–4500 mV, DPI 100–25600, current report rate in the supported list | first found |
| `HwProbe.WirelessPathsReportTheSameFeatureTable` | receiver and DJ node report the same protocol and feature table | Linux, wireless; skipped elsewhere |

## Rules for write tests

Every test that changes device state must:

1. take a backup first (onboard memory or the value being changed), from task 0.7 onward;
2. apply the change, read it back and compare;
3. restore the backup, even when the test fails (use a scope guard);
4. list in `docs/testing/hw-checklist.md` any visual or physical check (LED colour, button behaviour) that a person must confirm.

## Matrix

Each feature is tested on both operating systems and, where relevant, on both connections:

| | Cable | Receiver |
|---|---|---|
| Windows 11 | ✅ required | ✅ required |
| Ubuntu 26.04 | ✅ required | ✅ required |

## Log

| Date | Task | OS | Connection | Result |
|---|---|---|---|---|
| 2026-11-09 | 0.1 | Windows 11 | cable | `HwProbe.*` 2/2 passed |
| 2026-11-11 | 0.1 | Ubuntu 26.04 | receiver (+ DJ node `407F`) | unit 32/32; `HwProbe.*` 3/3 passed, via receiver |
| 2026-11-11 | 0.1 | Ubuntu 26.04 | cable | unit 32/32; `HwProbe.*` 2/2 passed via cable, wireless-paths test skipped |
