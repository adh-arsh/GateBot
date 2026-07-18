# Wi‑Fi modes: AP vs client (STA)

GateBot can run in two network modes, toggled from **Admin → Network**.

## Modes

| Mode | Radio | How you reach the device | Internet |
|------|-------|--------------------------|----------|
| **AP (no internet)** | SoftAP `GateBot` / `Star@918` | Join that Wi‑Fi, then open `http://192.168.4.1/` | No |
| **Wi‑Fi (internet)** | Client on your home 2.4 GHz network | Same home Wi‑Fi, open `http://<lan-ip>/` | Yes (router) |

Settings (mode, SSID, password) are stored in NVS flash and survive power-off.

## Boot behavior

1. If saved mode is **sta** and an SSID is stored → try connecting for **15 seconds**.
2. If STA succeeds → HTTP on the LAN IP (SoftAP off).
3. If STA fails → **SoftAP fallback** so you can still open admin and fix Wi‑Fi.

## Captive portal (AP mode)

When someone joins **GateBot** Wi‑Fi, phones often say “No internet.” That is expected.

GateBot runs a **captive portal**:

1. DNS answers every name with `192.168.4.1`
2. Connectivity checks (Android / iOS / Windows) open the **PIN unlock page** at **http://192.168.4.1/**
3. Enter the 6-digit PIN there to open the gate

You can still type `http://192.168.4.1` manually in the browser if the portal doesn’t appear.

## Admin steps

### Stay offline / share with guests (AP)

1. Open `/admin` → Network → **AP · no internet**
2. Copy the **share link** (`http://192.168.4.1/`)
3. Guests: join Wi‑Fi **GateBot**, password **Star@918**, open that link, enter PIN

### Join home Wi‑Fi (STA)

1. Network → **Wi‑Fi · internet**
2. **Scan networks** (2.4 GHz only)
3. Tap an SSID, enter password, **Connect & reboot**
4. Connect your phone to the **same home Wi‑Fi**
5. Open the **Admin URL** shown after reconnect (e.g. `http://192.168.1.42/admin`)

## API

| Method | Path | Body |
|--------|------|------|
| GET | `/api/v1/wifi/status` | — |
| GET | `/api/v1/wifi/scan` | — |
| PUT | `/api/v1/wifi/mode` | `{ "mode": "ap" }` or `{ "mode": "sta" }` |
| PUT | `/api/v1/wifi/sta` | `{ "ssid": "…", "password": "…" }` |

All require admin session. STA connect responds with `rebooting: true` then restarts.

## Notes

- ESP32 does **not** see 5 GHz SSIDs.
- After switching to STA, SoftAP is gone until you toggle AP or STA fails.
- STA mode is the prerequisite for a future outbound WebSocket to Express — see [`websocket-pin-webapp.md`](./websocket-pin-webapp.md).
