# GateBot API v1

**Base URL (SoftAP):** `http://192.168.4.1/api/v1`

## Public endpoints

| Method | Path | Body | Description |
|--------|------|------|-------------|
| GET | `/api/v1/health` | — | Liveness |
| POST | `/api/v1/unlock` | `{ "pin": "123456" }` | Open gate if PIN matches |
| POST | `/api/v1/admin/login` | `{ "email", "password" }` | Set session cookie |

Wrong PIN → `401`. After 5 failures → `429` lockout for 30s.

## Admin endpoints (session cookie required)

Login at **http://192.168.4.1/admin** then call with `credentials: 'include'`.

| Method | Path | Body | Description |
|--------|------|------|-------------|
| GET | `/api/v1/admin/me` | — | Confirm session |
| POST | `/api/v1/admin/logout` | — | Clear session |
| GET | `/api/v1/status` | — | Device + servo state |
| GET | `/api/v1/config` | — | Angles |
| PUT | `/api/v1/config` | `{ "homeAngle", "pressAngle" }` | Update + save to flash |
| DELETE | `/api/v1/config` | — | Factory angle defaults |
| POST | `/api/v1/gate/open` | — | Press sequence |
| POST | `/api/v1/gate/home` | — | Move home |
| POST | `/api/v1/servo/angle` | `{ "angle": 0-180 }` | Absolute move |
| GET | `/api/v1/pins` | — | List PINs (id, label; no secrets) |
| POST | `/api/v1/pins` | `{ "pin": "123456", "label": "guest" }` | Create (returns PIN once) |
| DELETE | `/api/v1/pins?id=N` | — | Delete PIN |

Success responses include `"ok": true`. Config/status include `"persisted": true|false`.

PINs are stored as SHA-256 hashes in NVS (max 10). Admin session is in RAM (cleared on reboot).

## Pages

| URL | Purpose |
|-----|---------|
| `/` | Public 6-digit PIN keypad |
| `/admin` | Login, then GateBot settings + PIN manager |
