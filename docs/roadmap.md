# Roadmap

Target: every setting G HUB exposes for the G502 LIGHTSPEED, on Windows 11 and Ubuntu 26.04, firmware updates excluded.
Each task is described in detail in [plan.md](plan.md); this page only tracks progress.

Definition of done for every task:

1. Implemented in core, exposed over IPC, available in `periphctl` and in the Qt UI (from task 0.6 onward).
2. Unit tests (fakes / simulator) and, where relevant, IPC and UI view-model tests.
3. Documentation: `docs/protocol/`, `docs/api/`, `docs/user/` and the matching row of `docs/feature-matrix.md`.
4. Hardware test on the real mouse (backup, set, read back, restore), on Windows and Ubuntu, by cable and through the receiver where relevant.
5. One commit per task.

## Phase 0: Foundations

- [x] 0.1 Repository layout (core/ui split), CMake presets, GoogleTest, `ITransport` + fakes, architecture test
- [ ] 0.2 Feature matrix: inventory of every G HUB setting for the G502
- [ ] 0.3 Robust transport: reader thread, notifications, hotplug, reconnect, cable/wireless switch
- [ ] 0.4 Device registry: stable identity, wired and wireless merged into one logical device
- [ ] 0.5 Daemon + JSON-RPC IPC + client library + `periphctl list|info`
- [ ] 0.6 Qt UI shell (device list, device page, daemon status)
- [ ] 0.7 Onboard memory backup/restore and factory reset (prerequisite for every write)

## Phase 1: Onboard profiles and basic settings

- [ ] 1.1 Onboard memory read/write (0x8100), profile codec with CRC, transactional writes
- [ ] 1.2 Onboard/host mode, active profile, enable/disable/rename/restore profiles
- [ ] 1.3 Sensitivity: 5 DPI levels, default level, DPI shift, live DPI, DPI indicator
- [ ] 1.4 Report rate
- [ ] 1.5 Firmware lighting (0x8070): zones, OFF/FIXED/CYCLE/BREATHING, brightness, low-battery effect, inactivity timeout
- [ ] 1.6 Battery: voltage to percentage (own calibration), charging state, notifications, Powerplay

## Phase 2: Buttons and macros

- [ ] 2.1 Onboard button assignments, BASE and G-Shift layers
- [ ] 2.2 HID usage tables and layout-aware key names
- [ ] 2.3 Onboard macros
- [ ] 2.4 Host-mode button capture (0x8110)
- [ ] 2.5 Input injection (SendInput / uinput)
- [ ] 2.6 Software macro engine and recorder
- [ ] 2.7 Software G-Shift; saving to onboard memory

## Phase 3: Software and per-application profiles

- [ ] 3.1 Profile store (versioned JSON, import/export)
- [ ] 3.2 Active application detection (Windows, X11, GNOME/KDE Wayland)
- [ ] 3.3 Switching rules, persistent profile, onboard sync

## Phase 4: Software LIGHTSYNC

- [ ] 4.1 Host-driven effect engine
- [ ] 4.2 Custom effect editor and presets
- [ ] 4.3 Screen sampler and audio visualizer

## Phase 5: Lua scripting

- [ ] 5.1 Sandboxed Lua 5.4
- [ ] 5.2 G-series style API and editor

## Phase 6: Wireless and receiver

- [ ] 6.1 Receiver pairing management

## Phase 7: Integrations

- [ ] 7.1 OBS (obs-websocket v5)
- [ ] 7.2 Discord (local RPC)

## Phase 8: Application and distribution

- [ ] 8.1 App settings: autostart, tray, notifications, it/en
- [ ] 8.2 Packages: Windows installer, .deb and AppImage
- [ ] 8.3 Final feature-matrix verification
