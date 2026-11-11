# Architecture

## Layers

```
 periph-gui (ui/qt, Qt 6)        periphctl (cli)        periph-probe (tools)
          │                            │                        │
          └──────── libperiph_client ──┘                        │
                         │  JSON-RPC 2.0 over a local socket     │
                       periphd (daemon)                          │
                         │                                       │
                    libperiph_core  ◄────────────────────────────┘
                         │  HID++ over hidapi
                     devices (USB cable, Lightspeed/Unifying receivers)
```

| Target | Directory | Depends on | Qt allowed |
|---|---|---|---|
| `periph_core` | `core/` | hidapi, std | **no** |
| `periph_ipc`, `periph_client` | `ipc/` | core types, std | **no** |
| `periphd` | `daemon/` | core, ipc | **no** |
| `periphctl` | `cli/` | client | **no** |
| `periph-gui` | `ui/qt/` | client, Qt 6 | yes |
| `periph-probe` | `tools/probe/` | core | **no** |

Rules:

1. Only the daemon opens devices. UI and CLI are thin clients of the same API, so anything the UI does is scriptable from the CLI.
2. The UI never links `periph_core`. It sees devices only through `libperiph_client`.
3. Qt appears only under `ui/`. The `architecture.no_qt_outside_ui` test (`cmake/CheckNoQt.cmake`) fails the build if it shows up elsewhere.

`ipc/`, `daemon/`, `cli/` and `ui/` arrive with roadmap tasks 0.5 and 0.6.

## Core (`core/`)

| Header | Responsibility |
|---|---|
| `periph/hid.hpp` | RAII wrapper over hidapi: enumerate, open, read with timeout, write |
| `periph/hidpp.hpp` | `ITransport` interface; `Channel`: frame building, short/long report choice, request/response matching by `sw_id`, HID++ 1.0/2.0 error decoding |
| `periph/hid_transport.hpp` | `HidTransport` (the hidapi `ITransport`), grouping of HID collections into endpoints |
| `periph/device.hpp` | `Device`: ping, feature table, typed readers per feature |

### Transport abstraction

`Channel` only talks to `ITransport`. Production code uses `HidTransport`; tests use `FakeTransport` plus device simulators (`tests/support/fake_g502.*`), so every protocol path can be tested without hardware.

### Platform notes

- **Windows** exposes each HID top-level collection as a separate path. A HID++ interface has a short (usage `FF00:0001`) and a long (`FF00:0002`) collection. `HidTransport` writes long reports to the long collection and reads from both, because HID++ 1.0 errors arrive as short reports.
- **Linux** (hidraw) exposes one node per interface. hidapi lists it once per usage; `group_endpoints` merges these entries.
- **Linux `hid-logitech-dj`.** For Unifying and Lightspeed receivers the kernel adds one hidraw node per paired device
  (e.g. the G502 as `046D:407F`). On that node the kernel overwrites the device index of every request with the
  device's slot, so replies never come back at `0xFF`. `hid::enumerate` reads the slot from the `:N` suffix of
  `HID_PHYS` in `/sys/class/hidraw/hidrawN/device/uevent` (`usb-…/input2:1`) and the kernel name from `HID_NAME`;
  `Endpoint::device_index` then holds the slot. The receiver's own node keeps working in parallel (index `0xFF` for
  the receiver, slots `1..6` for devices).
- **Sleeping wireless devices.** The first request after idle can be lost while the mouse wakes up. `Device::probe`
  retries a timed-out ping (3 attempts by default); HID++ errors are not retried.
- G HUB may run at the same time. Replies are matched by software id (`sw_id`, default `0x0B`), so its traffic is ignored. Tests that write to the device refuse to run while G HUB is active.

## Device addressing

| Connection | Device index |
|---|---|
| USB cable (e.g. G502 LIGHTSPEED `046D:C08D`) | `0xFF` |
| Through a receiver (e.g. Lightspeed `046D:C539`) | paired slot `1..6` |
| Linux `hid-logitech-dj` node of a paired device (e.g. `046D:407F`) | its slot `1..6` (`Endpoint::device_index`) |
| The receiver itself (HID++ 1.0) | `0xFF` |

The feature table, and therefore every feature index, can differ between connections of the same device (the G502
has `0x00C2` only by cable and `0x2121` only wirelessly). Indexes come from the probe of the current connection.

A receiver answers a HID++ 2.0 ping at `0xFF` with HID++ 1.0 error `0x01` (invalid sub-id). This is how probing tells receivers from directly attached devices.
