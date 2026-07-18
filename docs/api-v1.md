# GateBot API v1

Same JSON API on SoftAP today and on home Wi‑Fi / React later.

**Base URL (SoftAP):** `http://192.168.4.1/api/v1`

## Endpoints

| Method | Path | Body | Description |
|--------|------|------|-------------|
| GET | `/api/v1` | — | List endpoints |
| GET | `/api/v1/health` | — | Liveness |
| GET | `/api/v1/status` | — | Full device + servo state |
| GET | `/api/v1/config` | — | `{ homeAngle, pressAngle, angle }` |
| PUT | `/api/v1/config` | `{ "homeAngle": 90, "pressAngle": 70 }` | Update angles |
| POST | `/api/v1/gate/open` | — | Press sequence |
| POST | `/api/v1/gate/home` | — | Move to home |
| POST | `/api/v1/servo/angle` | `{ "angle": 85 }` | Absolute move 0–180 |

All success responses include `"ok": true`. Errors: `"ok": false, "error": "..."`.

CORS is enabled (`*`) so a future dashboard on another origin can call the device directly during local testing.

## Examples

```bash
# Status
curl http://192.168.4.1/api/v1/status

# Open gate
curl -X POST http://192.168.4.1/api/v1/gate/open

# Set angles
curl -X PUT http://192.168.4.1/api/v1/config \
  -H 'Content-Type: application/json' \
  -d '{"homeAngle":90,"pressAngle":70}'
```

## Browser client

The SoftAP page exposes `window.GateBotAPI`:

```js
await GateBotAPI.openGate()
await GateBotAPI.setConfig({ homeAngle: 90, pressAngle: 68 })
await GateBotAPI.status()

// Later on home Wi‑Fi / backend proxy:
GateBotAPI.baseUrl = 'http://192.168.1.50'  // or ''
```
