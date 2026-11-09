# Hardware testing

Hardware tests live in `tests/hw/` and build into `periph-hwtest`. They are skipped unless `PERIPH_HW_TEST=1` is set.

## Requirements

- A G502 LIGHTSPEED attached by cable (`046D:C08D`) or through its Lightspeed receiver (`046D:C539`, slot 1).
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
| pending | 0.1 | Ubuntu 26.04 | cable | not run yet (build + `HwProbe.*`) |
