# Phase 1 — Hardware bring-up & local SoftAP control

Status: **complete** (validated on wall power with calibrated servo angles).

This phase proves the mechanical press works without home Wi‑Fi. The ESP32 runs its own access point and serves a control page + JSON API.

## Validated setup

| Item | Value |
|------|-------|
| Power | Wall outlet → USB adapter (5V) → ESP32 micro‑USB |
| Charger | Samsung 5V / 2.4A (or any quality 5V / ≥1–2A USB adapter) |
| Controller | ESP32 DevKit |
| Servo | Tower Pro SG90 on **GPIO18** |
| Home angle | **40°** |
| Press angle | **170°** |
| SoftAP SSID | `GateBot` |
| SoftAP password | `Star@918` |
| Control URL | http://192.168.4.1 |
| API base | http://192.168.4.1/api/v1 |

Defaults live in `firmware/include/config.h`:

```c
#define SERVO_HOME_ANGLE 40
#define SERVO_PRESS_ANGLE 170
```

## Goals completed

1. ESP32 powers reliably from a wall USB adapter  
2. SoftAP `GateBot` stays visible (no brownout reboot loop)  
3. Web UI / API can command a press  
4. Servo returns to home after each press  
5. Travel calibrated for the apartment intercom Open button  

## Wiring

```
Wall outlet
    │
 USB adapter (5V / ≥1A)
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

### Power rules

- Plug into a **wall socket via USB adapter** — preferred over a laptop USB port.  
- Do **not** power the servo from the ESP32 **3.3V** pin.  
- Common GND between ESP32 and servo is required.  
- Optional: **470 µF** electrolytic across servo +5V and GND (near the servo) if you see jitter or resets under load.  
- Never wire the board directly to mains AC.

### Why wall power matters

SG90 current spikes can brown out a weak USB port. That caused:

- Servo twitching / “continuous” motion (reboot → re-home loop)  
- SoftAP disappearing  

Firmware mitigations (still use a solid 5V adapter):

- SoftAP starts before any servo motion  
- Servo attaches only when commanded  
- Servo detaches after idle (~600 ms) to cut holding current  

## Mechanical notes

- Mount so the finger sits ~1–2 mm above the Open button at **home = 40°**.  
- **Press = 170°** should depress the button without the servo stalling/buzzing.  
- Non-destructive mount: servo bracket + 3M Command strips + ABS enclosure.

## How to use (day-to-day)

1. Plug the ESP32 into the wall USB adapter.  
2. On your phone, join Wi‑Fi **GateBot** / **Star@918**.  
3. Open **http://192.168.4.1**.  
4. Tap **Open Gate** (calls `POST /api/v1/gate/open`).  
5. Adjust sliders only if remounting changes alignment (`PUT /api/v1/config`).

Serial (115200) still works while USB-serial is connected: `p`, `h`, `home=40`, `press=170`, `status`.

## Build & flash

```bash
export PATH="$PATH:$HOME/.platformio/penv/bin"
cd firmware
pio run -t upload --upload-port /dev/cu.usbserial-XXXX
```

Data-capable USB cable required for flashing. After flashing, you can run from the wall adapter alone.

## API (reusable later on home Wi‑Fi)

See [`api-v1.md`](./api-v1.md). Summary:

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/api/v1/status` | Device + servo state |
| POST | `/api/v1/gate/open` | Press sequence |
| POST | `/api/v1/gate/home` | Move to home |
| PUT | `/api/v1/config` | `{ "homeAngle": 40, "pressAngle": 170 }` |

## Checklist

- [x] ESP32 powers from wall USB adapter  
- [x] SoftAP `GateBot` visible and stable  
- [x] Control page loads at http://192.168.4.1  
- [x] Servo moves only on command (not continuously)  
- [x] Home **40°** / press **170°** calibrated  
- [x] Press returns to home cleanly  
- [x] Survives unplug / replug on wall power  

## What’s next

- Phase 3+: join home Wi‑Fi, Express backend, JWT, React dashboard, OTA  
- Same `/api/v1` contract can be proxied or called from the cloud UI  
