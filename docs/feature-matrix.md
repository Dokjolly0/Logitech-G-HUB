# Feature matrix: G HUB settings for the G502 LIGHTSPEED

This table is the acceptance checklist for "full parity" (task 0.2). Every setting G HUB 2026 (v39.1.2) exposes for the
G502 LIGHTSPEED has one row, with the mechanism periph uses, the task that implements it and its status. A task is not
done until its rows are ✅ (see [plan.md](plan.md), definition of done). Task 8.3 re-verifies every row on Windows and
Ubuntu, by cable and through the receiver.

Sources:

- the G HUB agent API (`docs/re/agent_ws_paths.txt`, 419 paths, and `docs/re/agent_protos.txt`). The appendix classifies
  every path that has no row, so nothing is left out silently;
- read-only queries on our own G502 (below);
- the G HUB UI walk on Windows. Rows marked ❓ in the **UI** column still have to be confirmed there (procedure at the
  end). ✓ means seen in the UI.

`tests/CMakeLists.txt` runs `docs.feature_matrix` (`cmake/CheckFeatureMatrix.cmake`). It fails when a row has an
unknown status or task, an ID is duplicated, a path does not exist in the agent API, or an agent path is covered
neither by a row nor by the appendix.

## Status legend

| Symbol | Meaning |
|---|---|
| ✅ | done and hardware-tested on both OSes |
| 🟡 | partly done (read-only, or one OS only) |
| ⬜ | planned |
| ➖ | not applicable: G HUB has the path, but the G502 has no matching capability |
| ⛔ | out of scope (plan §5) |

## Device facts used below

Read-only queries (get functions and `memoryRead` only) on 2026-11-11, Ubuntu 26.04, by cable:

| Item | Value |
|---|---|
| Firmware entities (0x0003) | 0: bootloader `BOT` 92.00 build 0008; 1: main firmware `MPM` 17.00 build 0008; 2: type 5, build 00A9 |
| Onboard memory (0x8100 getDescription) | memory model 1, profile format 3, macro format 1, **5 profiles**, 1 factory (OOB) profile, **11 buttons**, 16 sectors × 255 bytes, mechanical layout `0x0A`, various info `0x04` |
| Onboard state | mode 1 (onboard), current profile 1, current DPI index 4 (0x8100 fn 0xB) |
| Profile directory (sector 0x0000) | sectors 1–5 listed; only profile 1 enabled |
| Lighting (0x8070 v4 getInfo) | 2 zones: 0 = location 1 (PRIMARY), 1 = location 2 (LOGO); raw info `02 00 03 00 04` |
| Effects per zone (0x8070 getZoneEffectInfo) | `0x0000` off, `0x0001` fixed, `0x0003` cycle, `0x000A` breathing (same on both zones) |
| Buttons (0x8110) | 11 |
| Sensors (0x2201) | 1; 100–25600 DPI, step 50 |
| Report rate (0x8060) | 125 / 250 / 500 / 1000 Hz |
| Battery (0x1001) | voltage only, charging flag; no 0x1000/0x1004 percentage |
| Not in the feature table | 0x1814/0x1815 (multi-host), 0x2110/0x2150 (SmartShift, thumbwheel), 0x2202 (extended DPI), 0x8061 (extended report rate), 0x8071 (RGB effects), angle snapping / surface tuning features |

## Device and connection

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| DEV-01 | Device list, connection (cable / wireless), online state | `/devices/list` `/devices/state/changed` `/devices/subdevices/list` `/devices/list/known` | enumeration, hotplug, 0x1D4B wireless status, 0x0020 config change | 0.3, 0.4 | 🟡 | ❓ |
| DEV-02 | Cable ↔ wireless switch without losing the device | `/device_mode/changed` `/device_mode/report` `/mouse/{id}/mode_status` | one logical device per serial; feature indexes re-read per connection | 0.3, 0.4 | ⬜ | ❓ |
| DEV-03 | Model name, type, firmware versions | `/devices/model/info` `/mouse/{id}/info` `/devices/driver_info` `/devices/{id}/driver_info` | 0x0005 name/type, 0x0003 firmware info | 0.5 | 🟡 | ❓ |
| DEV-04 | User-given device name | `/devices/{id}/given_name` | host configuration | 0.4 | ⬜ | ❓ |
| DEV-05 | Restore factory defaults | `/onboard_profiles/{id}/restore_defaults` | rewrite onboard memory from the factory profile (not 0x1802 DeviceReset, hidden) | 0.7 | ⬜ | ❓ |
| DEV-06 | Onboard memory backup / restore (periph addition) | — | 0x8100 memory read / write, file on disk | 0.7 | ⬜ | — |

## Onboard profiles

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| PRF-01 | Onboard memory mode on / off | `/onboard_profiles/{id}/onboard_mode` | 0x8100 setOnboardMode / getOnboardMode | 1.2 | ⬜ | ❓ |
| PRF-02 | Active onboard profile | `/onboard_profiles/{id}/active_profile` `/configuration_profile/{id}/active_profile` `/configuration_profile/{id}/active_profile/changed` | 0x8100 set / getCurrentProfile, profile-change notification | 1.2 | ⬜ | ❓ |
| PRF-03 | Profile list, enable / disable profiles | `/onboard_profiles/{id}/profiles` `/onboard_profiles/{id}/enable_profile` `/onboard_profiles/{id}/disable_profile` `/configuration_profile/{id}/info` | profile directory (sector 0x0000) | 1.1, 1.2 | ⬜ | ❓ |
| PRF-04 | Rename a profile | `/onboard_profiles/{id}/edit_profile` `/configuration_profile/{id}/profile_names` `/devices/{id}/onboard_name_length` | name field of the profile sector | 1.2 | ⬜ | ❓ |
| PRF-05 | Restore a profile to its default | `/onboard_profiles/{id}/restore_default_profile` | copy from the factory (OOB) profile | 1.2 | ⬜ | ❓ |
| PRF-06 | Read / write a whole profile | `/onboard_profiles/{id}/profile` | profile codec with CRC, transactional writes | 1.1 | ⬜ | — |
| PRF-07 | Defragment onboard memory | `/onboard_profiles/{id}/defragment` | sector reallocation | 1.1 | ⬜ | ❓ |
| PRF-08 | Save a software profile to onboard memory ("onboard cards") | `/iprofile/{id}/onboard_cards` `/iprofile/{id}/onboard_card/update` `/iprofile/{id}/onboard_cards/updates` `/iprofile/{id}/onboard_profile/store` `/iprofile/{id}/onboard_profile/names` `/iprofile/{id}/onboard_profile_list` `/iprofile/{id}/onboard_state` `/iprofile/{id}/data_sync` `/iprofile/{id}/imported_profile` | host profile → profile codec | 2.7, 3.3 | ⬜ | ❓ |
| PRF-09 | Profile tags (which settings a profile carries) | `/onboard_profiles/{id}/tag_configuration` `/onboard_profiles/{id}/tag_configuration/all` `/tagged_device_settings/{id}/capability` `/tagged_device_settings/{id}/setting` | host-side bookkeeping of profile fields | 3.1 | ⬜ | ❓ |

## Sensitivity and report rate

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| SEN-01 | Up to 5 DPI levels, 100–25600 step 50 | `/onboard_profiles/{id}/profile` | DPI table of the profile; 0x2201 range | 1.3 | 🟡 | ❓ |
| SEN-02 | Default DPI level | `/onboard_profiles/{id}/profile` | profile field | 1.3 | ⬜ | ❓ |
| SEN-03 | DPI shift (sniper) value | `/mouse/{id}/dpi_shift` | profile field, DPI-shift binding | 1.3 | ⬜ | ❓ |
| SEN-04 | Live DPI and current DPI level | `/mouse/{id}/info` | 0x2201 get / setSensorDpi (read done), 0x8100 get / setCurrentDpiIndex | 1.3 | 🟡 | ❓ |
| SEN-05 | Save the current DPI to the factory profile | `/onboard_profiles/{id}/dpi_save_oob` | to clarify: G HUB behaviour on the OOB profile | 1.3 | ⬜ | ❓ |
| SEN-06 | DPI indicator LEDs | `/mouse/{id}/dpi_always_on` | profile LED / DPI indicator field | 1.3 | ⬜ | ❓ |
| SEN-07 | On-screen DPI change notification | `/notifications/devices/config` | host notification on DPI events | 1.3, 8.1 | ⬜ | ❓ |
| SEN-08 | Angle snapping | `/mouse/{id}/angle_snapping` | no feature on the G502 | — | ➖ | ❓ |
| SEN-09 | DPI calibration, DPI correction | `/mouse/{id}/dpi_tune/start` `/mouse/{id}/dpi_tune/stop` `/mouse/{id}/dpi_tune/progress` `/mouse/{id}/dpi_tune/finished` `/mouse/{id}/dpi_correction` `/mouse/{id}/dpi_corrections` | no feature on the G502 | — | ➖ | ❓ |
| SEN-10 | Surface tuning, hybrid engine | `/mouse/{id}/hybrid_engine` | no feature on the G502 (surface tuning: appendix) | — | ➖ | ❓ |
| RR-01 | Report rate 125 / 250 / 500 / 1000 Hz | `/report_rate/{id}/report_rate_list` `/wireless/{id}/report_rate` | 0x8060 (live, read done) and profile field | 1.4 | 🟡 | ❓ |

## Firmware (onboard) lighting

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| LGT-01 | Effect per zone (PRIMARY, LOGO): off, fixed, cycle, breathing | `/lighting/{id}/firmware/effects` `/lighting/{id}/mode` `/lighting/{id}/state` | 0x8070 setZoneEffect; profile LED section | 1.5 | ⬜ | ❓ |
| LGT-02 | Colour, period / speed, intensity of each effect | `/lighting/{id}/firmware/effects` | 0x8070 effect parameters | 1.5 | ⬜ | ❓ |
| LGT-03 | Lighting brightness | `/lighting/{id}/brightness` `/lighting/{id}/firmware/brightness` | effect intensity / 0x1300 LED control (to confirm) | 1.5 | ⬜ | ❓ |
| LGT-04 | Lighting on / off | `/lighting/{id}/master_switch_on` | all zones off, restore on | 1.5 | ⬜ | ❓ |
| LGT-05 | Turn lighting off after inactivity, power saving | `/lighting/turn_off_for_inactivity` `/lighting/{id}/power_saving` `/lighting/{id}/firmware/passive` `/lighting/{id}/mode/wake` | profile field (inactivity timeout) | 1.5 | ⬜ | ❓ |
| LGT-06 | Low-battery lighting warning | `/lighting/{id}/firmware/battery/warning` `/lighting/{id}/low_battery` `/lighting/{id}/low_battery/dismiss` | profile field / firmware effect | 1.5, 1.6 | ⬜ | ❓ |
| LGT-07 | Start-up and shutdown effects | `/lighting/{id}/firmware/startup` `/lighting/{id}/firmware/shutdown` `/lighting/{id}/firmware/pc_shutdown` | 0x8070 non-volatile capabilities (raw `0x0003`) | 1.5 | ⬜ | ❓ |
| LGT-08 | Keep host lighting alive | `/lighting/{id}/keep_alive` | periodic refresh in host mode | 4.1 | ⬜ | ❓ |

## Software LIGHTSYNC

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| SWL-01 | Presets and animations driven by the PC | `/lighting/effect/presets` `/lighting/effect/prefabs` `/lighting/effect/info` | host effect engine over 0x8070 | 4.1 | ⬜ | ❓ |
| SWL-02 | Custom effect editor, save / delete effects | `/lighting/{id}/custom/effects` `/lighting/{id}/custom/effect/save` `/lighting/effect/save` `/lighting/effect/delete` | keyframe editor, own format | 4.2 | ⬜ | ❓ |
| SWL-03 | Colour palettes | `/lighting/palette/custom` `/lighting/palette/gsignature` `/lighting/palette/mono` `/lighting/palette/pastel` `/lighting/palette/vibrant` | own palettes (no Logitech data) | 4.2 | ⬜ | ❓ |
| SWL-04 | Lighting preview | `/lighting/viewer` `/lighting/viewer/state` `/lighting/viewer/update` | UI preview of the device | 4.2 | ⬜ | ❓ |
| SWL-05 | Screen sampler | (`screen_sampler.proto`) | DXGI / PipeWire capture | 4.3 | ⬜ | ❓ |
| SWL-06 | Audio visualizer | (`software_lighting.proto`) | WASAPI loopback / PipeWire | 4.3 | ⬜ | ❓ |
| SWL-07 | Windows Dynamic Lighting (LampArray) | `/lighting/lamparray/state` | to clarify: whether G HUB exposes it for the G502 | 4.1 | ⬜ | ❓ |
| SWL-08 | Lighting SDK for games | `/integration_manager/settings/led_sdk_enabled` | Logitech LED SDK: proprietary game integration | — | ⛔ | ❓ |

## Buttons and macros

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| BTN-01 | Assignment of each of the 11 controls | `/assignment/list` `/unified_profiles/assignments` `/unified_profiles/assignments/removed` | onboard button table (0x8100); software: 2.4 + 2.5 | 2.1 | ⬜ | ❓ |
| BTN-02 | G-Shift layer | `/input/gshift` | onboard G-Shift table; software G-Shift | 2.1, 2.7 | ⬜ | ❓ |
| BTN-03 | Keys with modifiers, layout-aware names | `/assignment/list` | HID usage tables | 2.1, 2.2 | ⬜ | ❓ |
| BTN-04 | Mouse and special functions (DPI up / down / cycle / shift, profile cycle, G-Shift) | `/assignment/list` | onboard special-function codes | 2.1 | ⬜ | ❓ |
| BTN-05 | Disable a control | `/disable_key/{id}/disabled` | "disabled" binding | 2.1 | ⬜ | ❓ |
| BTN-06 | Restore a default assignment | `/unified_profiles/assignment/default` | factory profile button table | 2.1 | ⬜ | ❓ |
| BTN-07 | Software-only actions (commands, media, system, launch application, text) | `/assignment/list` | host capture (0x8110 / diverted) + injection | 2.4, 2.5, 2.6 | ⬜ | ❓ |
| MAC-01 | Macro types: no repeat, repeat while held, toggle, sequence | `/macros/validate` | software macro engine | 2.6 | ⬜ | ❓ |
| MAC-02 | Record keystrokes and delays | `/macros/validate` | recorder (low-level hook / evdev) | 2.6 | ⬜ | ❓ |
| MAC-03 | Onboard macros | `/onboard_profiles/{id}/macros/validate` | macro format 1 codec, sector allocator | 2.3 | ⬜ | ❓ |

## Software and per-application profiles

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| SWP-01 | Desktop and per-game profiles: create, duplicate, rename, delete | `/unified_profiles/activities` `/unified_profiles/activity` `/unified_profiles/activity/duplicate` `/unified_profiles/statuses` `/profiles/update` `/profile/active` | versioned JSON profile store | 3.1 | ⬜ | ❓ |
| SWP-02 | Automatic switching by active application | `/profiles/auto_switching` `/unified_profiles/activity/manual` `/unified_profiles/activity/deactivate` | active-window detection + rules | 3.2, 3.3 | ⬜ | ❓ |
| SWP-03 | Persistent profile (lock) | `/profile/persistent` `/profiles/persistent_features` | switching rules | 3.3 | ⬜ | ❓ |
| SWP-04 | Application scan, add an application, disable one | `/applications/all` `/applications/scan` `/applications/scan/add` `/application/disable` | own scan (Steam libraries, manual), no Logitech database | 3.2 | ⬜ | ❓ |
| SWP-05 | Import / export profiles | `/profile/prepare_for_publish` | JSON export / import (no community upload) | 3.1 | ⬜ | ❓ |
| SWP-06 | Migration of legacy profiles | `/unified_profiles/legacy_migration/progress` `/unified_profiles/legacy_migration/state` | nothing to migrate in periph | — | ➖ | — |
| LUA-01 | Lua script per profile: create, rename, duplicate, delete, edit, output, documentation | `/profile/script_id` `/script/new` `/script/name` `/script/duplicate` `/script/delete` `/script/description` `/script/editor/content` `/script/editor/output` `/script/documentation` | Lua 5.4 sandbox, G-series API | 5.1, 5.2 | ⬜ | ❓ |

## Battery, power and wireless

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| PWR-01 | Battery level and charging state | `/battery/{id}/state` `/battery/state/changed` | 0x1001 voltage (read done) → own curve, notifications | 1.6 | 🟡 | ❓ |
| PWR-02 | Low-battery notifications | `/notifications/devices/config` | host notification | 1.6, 8.1 | ⬜ | ❓ |
| PWR-03 | Sleep timer, eco mode | `/battery/{id}/sleep_timer` `/battery/{id}/eco_mode` | no public feature on the G502 | — | ➖ | ❓ |
| PWR-04 | Powerplay charging pad | `/charge_pad/{id}/charge_control` `/charge_pad/{id}/paired_device` `/charge_pad/{id}/pairing_disconnect` `/devices/charge_pad/charging/enable` | Powerplay detection (no pad available to test) | 1.6 | ⬜ | ❓ |
| RCV-01 | Receivers and their paired devices | `/receivers/list` `/receivers/state/changed` `/devices/{id}/slots/state` | HID++ 1.0 receiver registers | 6.1 | ⬜ | ❓ |
| RCV-02 | Pair / unpair a device | `/receivers/pair` `/receivers/compatible_devices` `/devices/pairable` | pairing lock, power-cycle action | 6.1 | ⬜ | ❓ |

## Integrations

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| INT-01 | OBS / Streamlabs actions | (`streamlabs.proto`) | obs-websocket v5 | 7.1 | ⬜ | ❓ |
| INT-02 | Discord actions | (`discord.proto`) | Discord local RPC | 7.2 | ⬜ | ❓ |

## Application settings

| ID | Setting | G HUB agent path | Mechanism | Task | Status | UI |
|---|---|---|---|---|---|---|
| APP-01 | Start with the system, tray icon, theme | (`system_tray.proto`) | autostart, tray | 8.1 | ⬜ | ❓ |
| APP-02 | Notifications | `/notifications/config` `/notifications/audio_prompt/languages` | host notifications | 8.1 | ⬜ | ❓ |
| APP-03 | Language (it / en) | `/resources/localized_packages` `/resources/localized_string` | Qt Linguist | 8.1 | ⬜ | ❓ |
| APP-04 | Local settings backup / restore | `/local_settings_backup/list` `/local_settings_backup/save` `/local_settings_backup/restore` `/local_settings_backup/delete` | copy of the profile store | 3.1 | ⬜ | ❓ |

## Appendix: agent paths without a row

Each prefix covers every agent path that starts with it. ❓ in the reason means the classification still needs a look
in the UI walk.

| Prefix | Class | Reason |
|---|---|---|
| `/accounts/` | ⛔ | Logitech accounts |
| `/community/` | ⛔ | community profiles |
| `/beta_testing/` | ⛔ | G HUB beta programme |
| `/updates/` | ⛔ | G HUB self-update |
| `/dfu/` | ⛔ | firmware updates |
| `/devices/{id}/blocker` | ⛔ | update / driver blockers |
| `/crash_reporting/` | ⛔ | G HUB diagnostics |
| `/event_tracing/` | ⛔ | G HUB diagnostics |
| `/logging/` | ⛔ | G HUB diagnostics |
| `/overwolf/` | ⛔ | proprietary integration |
| `/lps/` | ⛔ | Logitech plugin service and marketplace |
| `/gl/` | ⛔ | G HUB game launcher |
| `/glauncher_detail/` | ⛔ | G HUB game launcher |
| `/mini_games/` | ⛔ | G HUB mini games |
| `/pipl/` | ⛔ | privacy onboarding |
| `/resources/release_notes` | ⛔ | G HUB release notes |
| `/cards/updated` | ➖ | G HUB UI internals |
| `/settings/error` | ➖ | G HUB UI internals |
| `/devices/resources` | ➖ | device images (Logitech assets) |
| `/devices/{id}/device_image_overlay` | ➖ | device images (Logitech assets) |
| `/devices/{id}/persistent_data` | ➖ | G HUB internal storage |
| `/devices/{id}/security_privacy_dialog/load` | ➖ | G HUB UI internals |
| `/driver_registry/restart_device` | ➖ | Windows driver restart |
| `/section/subsection` | ➖ | string artefact, not an endpoint |
| `/node_modules/` | ➖ | string artefact, not an endpoint |
| `/tmp/` | ➖ | string artefact, not an endpoint |
| `/usr/bin/` | ➖ | string artefact (macOS helpers) |
| `/audio/` | ➖ | headsets |
| `/centurion/` | ➖ | headsets |
| `/devices/{id}/centurion/` | ➖ | headsets |
| `/microphone/` | ➖ | microphones |
| `/mixline/` | ➖ | audio |
| `/test/record_playback` | ➖ | microphone test |
| `/uvc/` | ➖ | webcams |
| `/display/` | ➖ | devices with a display |
| `/illumination_light/` | ➖ | Litra lights |
| `/analog/` | ➖ | analog keyboards |
| `/keyboard/` | ➖ | keyboards |
| `/swappable_switches/` | ➖ | keyboards |
| `/input/{id}/fkc/` | ➖ | keyboards |
| `/input/{id}/fn_inversion` | ➖ | keyboards |
| `/input/{id}/mstate` | ➖ | M-keys (keyboards) |
| `/input/mstate/changed` | ➖ | M-keys (keyboards) |
| `/devices/{id}/donotdisturb_state` | ➖ | keyboards / headsets ❓ |
| `/wheel/` | ➖ | racing wheels |
| `/pedals/` | ➖ | pedals |
| `/gear_stick/` | ➖ | shifters |
| `/axis_monitor/` | ➖ | wheels / joysticks |
| `/axis_response_curve/` | ➖ | wheels / joysticks |
| `/banded_axis/` | ➖ | wheels / joysticks |
| `/rpm_indicator/` | ➖ | wheels |
| `/rpm_led_pattern/` | ➖ | wheels |
| `/gaming_attachments/` | ➖ | wheel attachments |
| `/hosts_info/` | ➖ | multi-host (no 0x1814/0x1815) |
| `/multibattery/` | ➖ | multi-battery devices |
| `/surface_tuning/` | ➖ | no surface tuning on the G502 ❓ |
| `/scarif/` | ➖ | unknown device family ❓ |
| `/input_tracker/` | ➖ | input test tool ❓ |
| `/devices/driver_info` | ➖ | Windows driver info, see DEV-03 ❓ |

## UI walk (Windows, G HUB closed afterwards)

To turn ❓ into ✓ (or move a row to ➖), with the G502 connected by cable and then through the receiver:

1. Home: device card, battery, profile selector, device name. → DEV, PWR, SWP-01.
2. Device page, **Sensitivity (DPI)**: levels, default, shift, report rate, any extra toggle. → SEN, RR.
3. **LIGHTSYNC**: every preset, animation and option for PRIMARY and LOGO, brightness, sync. → LGT, SWL.
4. **Assignments**: every category (commands, keys, actions, macros, system), G-Shift toggle, every control. → BTN, MAC.
5. Device **settings** (gear): onboard memory mode, onboard profiles, inactivity, battery, DPI indicator, reset. → PRF, LGT-05, SEN-06.
6. Profile management: games and applications, persistent profile, scripting, import / export. → SWP, LUA.
7. G HUB settings: start-up, notifications, language, integrations, backup. → APP, INT, SWL-08.

Write any setting not in the table as a new row; tick the **UI** column in the same commit.
