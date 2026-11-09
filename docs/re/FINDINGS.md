# G HUB 2026 (v39.1.2): risultati del reverse engineering

Analisi statica dei file installati (nessun processo è stato eseguito o strumentato).
Scopo: interoperabilità, cioè capire come comunicare con le periferiche Logitech da un client nostro e multipiattaforma.
**Non** riutilizzare codice, asset o file di dati Logitech nel nuovo progetto.

## Architettura osservata

```
lghub.exe (Electron/React UI, resources/app.asar)
   │  WebSocket  ws://localhost:9010, sottoprotocollo "json"
   ▼
lghub_agent.exe (C++ nativo, ~97 MB)  ── router su path, messaggi definiti in protobuf (logi/protocol/*.proto)
   │  HID (hid.dll: HidD_SetOutputReport/GetFeature…), WinUSB, SetupDi/CM_*, WinRT Bluetooth/GATT
   ▼
Periferiche: USB diretto, ricevitori Unifying/Lightspeed/Bolt (HID++ 1.0 + 2.0), Bluetooth
```

Altri componenti: `lghub_system_tray.exe` (WinUI3), `lghub_updater` / `software_manager` (aggiornamenti tramite "depots"),
`sdk_legacy_led_*.dll` (SDK LED per i giochi), `driver_hid_virtual` (tastiera virtuale per le macro, VID_046D&PID_C232).

## Protocollo tra UI e agent (osservato in main.js)

- `new WebSocket("ws://localhost:9010", "json")`
- Messaggi JSON: `{ "path": "/devices/list", "verb": "GET" | "SET" | "SUBSCRIBE" | "UNSUBSCRIBE", "payload": {...} }`
- Gli eventi arrivano con lo stesso `path` (es. `/devices/state/changed`, `/battery/state/changed`)
- I path per dispositivo usano il template `%(id)s`, es. `/battery/%(id)s/state`, `/mouse/%(id)s/dpi_shift`, `/lighting/%(id)s/mode`
- Elenco completo: `agent_ws_paths.txt` (419 path). Gli schemi dei payload sono messaggi protobuf serializzati in JSON (vedi `agent_protos.txt`)

## Livello dispositivo

- Stack interno `devio`: bus `UnifyingBus`, `AccessoryBus`, `PairingBus`, `GattBus`, `WinRTBluetoothBus`, `CombinerBus`
- HID++ 1.0 (`HPP10Command`) per i registri dei ricevitori, HID++ 2.0 per le feature (Root 0x0000 → FeatureSet 0x0001 → feature indicizzate)
- 210 classi feature implementate: `hidpp_features.txt` (es. 0x1004 UnifiedBattery, 0x2201/0x2202 DPI, 0x8060/0x8061 ReportRate,
  0x8070/0x8071 RGB, 0x8081 PerKeyLighting, 0x8100 OnboardProfiles, 0x1B04 SpecialKeys, 0x8123+ feature dei volanti)
- Le cuffie usano feature HID++ della famiglia 0x06xx/0x08xx/0x09xx (Centurion/LogiVoice)

## Database dispositivi

- `C:\ProgramData\LGHUB\depots\<build>\core\data\devices\*.json`: `modelId`, `type`, `modes[].interfaces[].id` (`046d_xxxx`),
  `capabilities` (zone di illuminazione, onboardProfiles, inputSupport…)
- 70 dispositivi in chiaro; la maggior parte dei file è cifrata (header `!\x05! ` + `key-id`), probabilmente per prodotti non ancora annunciati. **Non vanno decifrati.**
- Nel nuovo client le capacità vanno ricavate a runtime dall'enumerazione delle feature HID++, non dal DB di Logitech.

## Domande aperte

- Il layout esatto dei payload protobuf (i descrittori sono incorporati in lghub_agent.exe e si possono estrarre se serve)
- Il comportamento delle singole feature a livello di byte: usare le specifiche HID++ pubbliche e il codice di Solaar/libratbag come riferimento, e verificarlo sui propri dispositivi
