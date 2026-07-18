# GateBot

Wi-Fi-enabled device that mechanically presses an apartment intercom **Open Gate** button.

## Phase 1 — complete

Local SoftAP control on wall power.

- Overview: [`docs/phase1.md`](docs/phase1.md)
- Admin + PIN unlock: [`docs/admin-pin.md`](docs/admin-pin.md)
- API reference: [`docs/api-v1.md`](docs/api-v1.md)

| Item | Value |
|------|-------|
| Power | Wall outlet → USB adapter (5V) → ESP32 |
| SoftAP | **GateBot** / **Star@918** |
| Unlock | http://192.168.4.1/ — 6-digit PIN keypad |
| Admin | http://192.168.4.1/admin — settings + PIN manager |
| Admin login | `r.adharsh@kalpataruprojects.com` / `Star@918` |
| API | http://192.168.4.1/api/v1 · [`docs/api-v1.md`](docs/api-v1.md) |
| Factory angles | home **40°** / press **170°** (overridable, saved in flash) |

## Wiring

```
Wall USB adapter (5V / ≥1–2A)
         │
     Micro USB
         │
   ESP32 DevKit
  ┌────────────────────┐
  │ VIN (5V) ──────────┼─── Red    (+5V)  ──┐
  │ GND ───────────────┼─── Brown  (GND)  ──┤ SG90
  │ GPIO18 ────────────┼─── Orange (Signal)─┘
  └────────────────────┘
```

Use a wall charger (not a weak laptop port). Optional 470 µF across servo power. Firmware detaches the servo when idle to reduce brownouts.

## Build & upload

```bash
export PATH="$PATH:$HOME/.platformio/penv/bin"
cd firmware
pio run -t upload
pio device monitor   # 115200 baud
```

## Repo layout

```
GateBot/
├── firmware/     ← ESP32 (PlatformIO) — Phase 1
├── docs/         ← phase1.md, admin-pin.md, api-v1.md
├── backend/      ← Phase 3+ (not started)
└── frontend/     ← Phase 5 (not started)
```

## Next phases

3. Express + WebSocket backend  
4. JWT auth  
5. React dashboard  
6. OTA updates  
