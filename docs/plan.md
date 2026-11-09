# Implementation plan: full G HUB parity for the G502 LIGHTSPEED

This is the complete plan. [roadmap.md](roadmap.md) tracks progress against it task by task.

## 1. Context

G HUB, Logitech's configuration software, runs only on Windows. periph is an independent, optimised and cross-platform
replacement that must reproduce **every setting G HUB exposes** for the **G502 LIGHTSPEED**, on **Windows 11** and
**Ubuntu 26.04** (a dual-boot setup on the same PC). Firmware updates are excluded.

Also in scope: Lua scripting, software LIGHTSYNC, per-application profiles, Discord/OBS integrations.

### Starting point

- Commit `11d24b1`: HID++ library (hidapi 0.14 via FetchContent, framing, feature discovery, name/battery/DPI/report
  rate readers) and the read-only `periph-probe`, verified on the real mouse.
- Commit `6603d1e` (task 0.1): core/tools/tests split, `ITransport`, fakes, unit and hardware tests, architecture test.
- Interoperability notes on G HUB: [re/FINDINGS.md](re/FINDINGS.md).

### Target device facts

Observed with `periph-probe` and in the G HUB device database:

| Item | Value |
|---|---|
| Wired USB id | `046D:C08D`, device index `0xFF`, HID++ 4.2 |
| Wireless | Lightspeed receiver `046D:C539`, slot 1 (also `046D:407F` as a Linux DJ child) |
| Public features | 0x0003 FW version, 0x0005 name/type, 0x1D4B wireless status, 0x0020 config change, 0x1001 battery voltage, 0x8070 v4 colour LED effects, 0x1300 LED control, 0x8100 onboard profiles, 0x8110 mouse button spy, 0x8060 report rate, 0x2201 adjustable DPI (100–25600, step 50) |
| Lighting | `MOUSE_RGB_ZONAL`, zones PRIMARY and LOGO, effects OFF / FIXED / CYCLE / BREATHING, low-battery effect, DPI lighting |
| Inputs | layers BASE and G-Shift, 11 programmable controls |
| Pairing | receiver type `LS1`, pairing action "power cycle" |
| Other | Powerplay compatible |

### Clean-room rule

G HUB is studied only to understand interoperability. The repository never contains Logitech code, assets, curves or
data files (device database, application database, presets). Byte-level formats come from public HID++ documentation,
from open-source implementations (Solaar, libratbag) as a reference, and from dumps of our own device.

## 2. Architecture

The repository rule: **core and UI are completely separate**, so that the CLI is as capable as the UI.

```
core/        periph_core     C++23, no Qt. transport/ hidpp/ features/ device/ onboard/
                             input/ (capture + injection, per-platform backends) macros/ profiles/
                             lighting/ scripting/ automation/ integrations/ storage/ service/
ipc/         periph_ipc      JSON-RPC 2.0 schema + codec + local transport (named pipe / Unix socket). No Qt.
             periph_client   typed client used by UI and CLI. No Qt.
daemon/      periphd         core + IPC server; the only process that opens devices
cli/         periphctl       CLI client (grows with every task; also drives hardware tests)
ui/qt/       periph-gui      Qt 6 Quick; links ONLY periph_client, never core
tools/probe/ periph-probe    read-only diagnostic
tests/       unit/ (GoogleTest) ipc/ ui/ (Qt Test on view models with a fake client) hw/ (opt-in)
docs/        architecture.md, api/ (IPC methods), protocol/ (one page per HID++ feature),
             user/ (user guide), testing/ (procedures, hw checklist), feature-matrix.md
packaging/   linux/ (udev, systemd --user, .desktop, AppImage/.deb), windows/ (installer, autostart)
```

How the separation is enforced:

- separate CMake targets; only `ui/qt/CMakeLists.txt` may call `find_package(Qt6)`;
- `architecture.no_qt_outside_ui` (`cmake/CheckNoQt.cmake`) fails on any `#include <Q…>` or Qt CMake usage in
  `core/`, `ipc/`, `daemon/`, `cli/` or `tools/`;
- the UI reaches devices only through `periph_client`, so every UI action is also scriptable from `periphctl`.

Why a daemon: software macros, per-application switching and host-driven lighting must keep working when the window
is closed. This mirrors G HUB, whose UI only talks to `lghub_agent` over a local WebSocket.

### Dependencies

All pinned; a system package is preferred when available, otherwise CMake FetchContent downloads the release.

| Dependency | Used by | From task |
|---|---|---|
| hidapi 0.14.0 | core transport | done |
| GoogleTest 1.15.2 | tests | done |
| nlohmann_json | IPC, profile store | 0.5 |
| Asio (standalone) | IPC transport | 0.5 |
| Qt ≥ 6.6 (6.8 LTS recommended) | UI only | 0.6 |
| Lua 5.4 | scripting | 5.1 |
| IXWebSocket | OBS integration | 7.1 |
| Linux: libudev, PipeWire | hotplug, screen and audio capture | 0.3, 4.3 |

## 3. Definition of done (same sequence for every task)

1. **Implementation** in core, exposed over IPC, a `periphctl` command, a page or control in the Qt UI (UI and IPC
   from tasks 0.5/0.6 onward).
2. **Unit tests**: pure logic and byte-level codecs against `FakeG502` (a HID++ simulator with the real feature table
   and, from task 0.7, an onboard memory image taken from a dump); IPC tests; Qt view-model tests.
3. **Documentation**: `docs/protocol/<feature>.md` (byte format and observed behaviour), `docs/api/`, `docs/user/`,
   and the task's row in `docs/feature-matrix.md`.
4. **Hardware test** with G HUB closed (`periph-hwtest` refuses to write while `lghub_agent` runs): backup, set, read
   back and compare, restore. Automated where possible; visual or physical checks (LED colour, button behaviour) go
   to `docs/testing/hw-checklist.md` and the user confirms them. On **Windows and Ubuntu**, by **cable and receiver**
   where relevant. Procedures: [testing/hardware.md](testing/hardware.md), [testing/linux.md](testing/linux.md).
5. **Commit** in English (`feat(core): …`, `test: …`, `docs: …`), one per task, with the Co-Authored-By trailer;
   roadmap checkbox ticked and hardware test log updated in the same commit.

## 4. Phases and tasks

### Phase 0: Foundations

- **0.1 Repository restructuring** ✅. Layout above, CMake presets (`win-msvc`, `linux-gcc`), clang-format/clang-tidy,
  GoogleTest, `ITransport` + `FakeTransport` + `FakeG502`, architecture test.
  *HW:* `periph-probe` output unchanged.
- **0.2 Feature matrix.** Walk the G HUB UI for the G502 and cross-reference the agent API paths
  (`docs/re/agent_ws_paths.txt`) to list every setting: assignments, macros, sensitivity, lighting, profiles, device
  settings and application settings. Each row records the HID++ feature or mechanism, the task that implements it and
  its status. This table is the acceptance checklist for "100 %".
- **0.3 Robust transport.**
  - Reader thread and dispatcher that separates replies from notifications; timeouts and retries.
  - Hotplug: Windows `CM_Register_Notification`, Linux udev monitor. Reconnection.
  - Cable ↔ wireless switching; events 0x1D4B (wireless status) and 0x0020 (config change).
  - *HW:* unplug and replug the cable, switch to the receiver, power cycle the mouse.
- **0.4 Device registry.** Stable identity (serial / unit id from 0x0003). The G502 seen by cable and through the
  receiver becomes one logical device. Model and capabilities derived from the feature table.
- **0.5 Daemon, IPC, client and base CLI.**
  - JSON-RPC 2.0: `device.list`, `device.get`, `events.subscribe`.
  - Transport: named pipe `\\.\pipe\periph` on Windows, `$XDG_RUNTIME_DIR/periph.sock` on Linux; access restricted to
    the current user.
  - `periphctl list|info`. Start-up: systemd `--user` unit on Linux, per-user autostart on Windows.
- **0.6 Qt UI shell.** Window, device list, device page (battery, connection), daemon status; testable view models.
- **0.7 Onboard memory safety.** `periphctl backup|restore` (full dump of 0x8100 sectors to a file with a checksum)
  and factory reset of profiles. **Blocks every later write task.**
  *HW:* backup → restore → byte-for-byte comparison.

### Phase 1: Onboard profiles and basic settings

- **1.1 Onboard memory read/write (0x8100).** Memory info (sectors, sector size, profile and button counts), profile
  directory, profile parser/serializer with CRC, transactional writes (write, read back, compare, restore from backup
  on mismatch). Golden tests based on the real dump.
- **1.2 Mode and profile management.** "Onboard memory mode" on/off (onboard/host), active profile, enable/disable,
  rename, restore the default profile.
- **1.3 Sensitivity.** Up to 5 DPI levels, default level, DPI shift (sniper), current DPI index, live DPI in host mode
  (0x2201 plus profile fields), DPI indicator LEDs.
- **1.4 Report rate.** 125 / 250 / 500 / 1000 Hz, live (0x8060) and per profile.
- **1.5 Firmware lighting (0x8070 + profile LED section).** Zones PRIMARY/LOGO; OFF / FIXED / CYCLE / BREATHING with
  colour, period and brightness; persistence; low-battery effect; turn-off after inactivity.
  *HW:* visual checklist.
- **1.6 Battery and power.** Voltage → percentage with our own curve (calibrated from a discharge log we record),
  charging state, low-battery notifications, Powerplay detection.

### Phase 2: Buttons and macros

- **2.1 Onboard button assignments.** All 11 controls × layers BASE and G-Shift. Binding types: mouse button, key +
  modifiers, consumer/media, special functions (DPI up/down/cycle/shift, profile up/down/cycle, G-Shift, disabled).
  *HW:* checklist pressing every button.
- **2.2 HID usage tables and key names** that follow the keyboard layout (Windows / Linux).
- **2.3 Onboard macros.** Macro format codec, sector allocator, capacity check, repeat modes supported by the firmware.
  Starts with a spike on the dump to confirm the opcodes.
- **2.4 Host-mode button capture (spike, then implementation).** 0x8110 MouseButtonSpy and/or diverted bindings, for
  software actions, G-Shift and software macros.
- **2.5 Input injection.** Windows `SendInput`; Linux `uinput` (udev rule for `/dev/uinput`); keyboard, mouse,
  consumer keys, Unicode text.
- **2.6 Software macro engine.** Sequences with delays; no repeat / repeat while held / toggle / hold; recorder
  (Windows low-level hook, Linux evdev); text, launch application, system and media actions; the G HUB "commands"
  catalogue mapped to our own key combinations.
- **2.7 Software G-Shift and saving to onboard memory** whatever the firmware supports, with a warning for software-only
  bindings.

### Phase 3: Software and per-application profiles

- **3.1 Profile store.** Versioned JSON in the user configuration directory, schema migrations, import/export.
- **3.2 Active application detection.** Windows `SetWinEventHook`; Linux X11 `_NET_ACTIVE_WINDOW`; Ubuntu 26.04 GNOME
  Wayland through a small GNOME Shell extension shipped in the repository (and a KWin script for KDE). Our own game
  scan (Steam libraries, manual choice), not Logitech's application database.
- **3.3 Switching rules.** Desktop profile, persistent profile (lock), priorities, sync to onboard memory for when the
  daemon is not running.

### Phase 4: Software LIGHTSYNC

- **4.1 Host-driven effect engine** through 0x8070 in host mode: animations, frame rate, battery-aware behaviour.
- **4.2 Custom effect editor** (keyframes) and presets; sync across devices.
- **4.3 Screen sampler** (Windows DXGI Desktop Duplication, Linux PipeWire via the ScreenCast portal) and **audio
  visualizer** (WASAPI loopback, PipeWire).

### Phase 5: Lua scripting

- **5.1 Lua 5.4 sandbox.** No `io` / `os` / `require`; time and memory limits.
- **5.2 G-series style API.** `OnEvent`, `PressKey` / `ReleaseKey`, `PressMouseButton`, `MoveMouseTo` /
  `MoveMouseRelative`, `Sleep`, `IsMouseButtonPressed`, `GetMKeyState`, `OutputLogMessage`… built on 2.5/2.6. UI editor
  with output pane and API documentation.

### Phase 6: Wireless and receiver

- **6.1 Receiver management.** List pairings, pair (open lock, power-cycle action), unpair, link state; HID++ 1.0
  registers of the Lightspeed receiver. *HW:* re-pair the G502 with the C539.

### Phase 7: Integrations

- **7.1 OBS** through obs-websocket v5 (scenes, recording, streaming, mute) as button actions.
- **7.2 Discord** through the official local RPC (mute/deafen; requires the user's own Discord client id). Public APIs
  only.

### Phase 8: Application and distribution

- **8.1 Application settings.** Start with the system, tray icon, notifications, Italian/English (Qt Linguist), theme.
- **8.2 Packages.** Windows installer + per-user service; Linux `.deb` + AppImage, udev rules, systemd `--user` unit,
  `.desktop` file.
- **8.3 Final feature-matrix verification.** Every row green on Windows and Ubuntu, with the G502 by cable and through
  the receiver.

## 5. Out of scope

- Firmware updates / DFU.
- Hidden or engineering features (0x18xx, 0x1E00, …).
- Logitech accounts, cloud and community profiles.
- G HUB proprietary integrations (Overwolf, game applets).
- Guaranteed support for other devices. The design is generic (capabilities come from the HID++ feature table), so
  other HID++ mice will partly work, but only the G502 LIGHTSPEED is hardware-tested.

## 6. End-to-end verification

- `cmake --preset win-msvc && cmake --build --preset win-msvc && ctest --preset win-msvc` on Windows, and the
  `linux-gcc` equivalents on Ubuntu: unit, IPC, UI and architecture tests green.
- `periph-hwtest` (opt-in, `PERIPH_HW_TEST=1`, G HUB closed) on Windows and Ubuntu: every write test does
  backup → set → read back → restore.
- `docs/testing/hw-checklist.md` filled in by the user for visual and physical checks.
- The matching row of `docs/feature-matrix.md` set to ✅ before each task's commit.
