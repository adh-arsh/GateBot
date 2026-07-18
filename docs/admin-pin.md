# Admin panel & PIN unlock

Local SoftAP UX after Phase 1 hardware bring-up.

## Pages

| URL | Who | Purpose |
|-----|-----|---------|
| http://192.168.4.1/ | Visitors | 6-digit PIN keypad → opens gate |
| http://192.168.4.1/admin | Admin | Login, then servo settings + PIN manager |

### Admin credentials

- Email: `r.adharsh@kalpataruprojects.com`
- Password: `Star@918`

Defined in `firmware/include/config.h`. Session cookie (`gatebot_session`) is stored in RAM — cleared on reboot. PINs and servo angles survive power-off (NVS flash).

## Visitor flow

1. Join Wi‑Fi **GateBot** / **Star@918**
2. Open http://192.168.4.1/
3. Enter a valid 6-digit PIN (or use the Unlock button)
4. Device runs the press sequence once

Wrong PIN → error. After **5** failures → **30s** lockout.

## Admin flow

1. Open http://192.168.4.1/admin
2. Sign in with admin email/password
3. **Settings** — same controls as before: Open Gate, Go Home, home/press sliders, factory reset
4. **Access PINs** — create up to **10** PINs (6 digits + label); list/delete without revealing hashes

PINs are stored as SHA-256 hashes. The plain PIN is shown only once in the create response.

## API summary

Full reference: [`api-v1.md`](./api-v1.md).

**Public:** `POST /api/v1/unlock`, `POST /api/v1/admin/login`, `GET /api/v1/health`  
**Admin (cookie):** status, config, gate, servo, pins CRUD, logout

## Firmware modules

| File | Role |
|------|------|
| `firmware/src/auth.cpp` | Session cookie login |
| `firmware/src/pin_storage.cpp` | NVS PIN hashes |
| `firmware/src/api.cpp` | Public unlock + protected routes |
| `firmware/include/web_ui.h` | PIN pad, login, admin UI |
