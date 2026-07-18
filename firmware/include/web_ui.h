#pragma once

#include <Arduino.h>

// Embedded control page — talks only to GateBot API v1 (reusable on home Wi‑Fi).
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"/>
  <title>GateBot</title>
  <style>
    :root {
      --bg: #0f1412;
      --panel: #1a2420;
      --ink: #e8f0ea;
      --muted: #8fa398;
      --accent: #3dd68c;
      --danger: #ff6b4a;
      --line: #2a3830;
      --font: "IBM Plex Sans", "Segoe UI", sans-serif;
      --display: "IBM Plex Mono", ui-monospace, monospace;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: var(--font);
      color: var(--ink);
      background:
        radial-gradient(1200px 600px at 10% -10%, #1e3d30 0%, transparent 55%),
        radial-gradient(900px 500px at 100% 0%, #243028 0%, transparent 50%),
        var(--bg);
    }
    main { max-width: 420px; margin: 0 auto; padding: 28px 20px 48px; }
    .brand {
      font-family: var(--display);
      font-size: 1.75rem;
      letter-spacing: 0.04em;
      margin: 0 0 4px;
    }
    .sub { color: var(--muted); margin: 0 0 28px; font-size: 0.95rem; }
    .status {
      display: flex; justify-content: space-between; align-items: center;
      padding: 12px 14px; border: 1px solid var(--line);
      background: var(--panel); margin-bottom: 20px;
    }
    .dot {
      width: 10px; height: 10px; border-radius: 50%;
      background: var(--accent); box-shadow: 0 0 10px var(--accent);
      display: inline-block; margin-right: 8px;
    }
    .dot.off { background: var(--danger); box-shadow: none; }
    .meta { font-family: var(--display); font-size: 0.85rem; color: var(--muted); }
    .open {
      width: 100%; border: 0; padding: 22px 16px;
      font-size: 1.25rem; font-weight: 700; letter-spacing: 0.06em;
      text-transform: uppercase; color: #04140c;
      background: linear-gradient(180deg, #57e6a0, var(--accent));
      cursor: pointer; margin-bottom: 12px;
    }
    .open:active { transform: translateY(1px); }
    .open:disabled { opacity: 0.55; cursor: wait; }
    .row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 24px; }
    .btn {
      border: 1px solid var(--line); background: var(--panel);
      color: var(--ink); padding: 14px 10px; font-size: 0.95rem; cursor: pointer;
    }
    .btn:active { background: #22302a; }
    label {
      display: block; font-size: 0.8rem; color: var(--muted);
      margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.08em;
    }
    .field { margin-bottom: 18px; }
    .val { float: right; font-family: var(--display); color: var(--accent); }
    input[type=range] { width: 100%; accent-color: var(--accent); }
    .msg {
      min-height: 1.4em; color: var(--muted);
      font-family: var(--display); font-size: 0.85rem;
    }
    .msg.ok { color: var(--accent); }
    .msg.err { color: var(--danger); }
    .api {
      margin-top: 28px; padding-top: 16px; border-top: 1px solid var(--line);
      font-family: var(--display); font-size: 0.72rem; color: var(--muted);
      line-height: 1.55;
    }
  </style>
</head>
<body>
  <main>
    <h1 class="brand">GateBot</h1>
    <p class="sub">API-driven local control</p>

    <div class="status">
      <div><span class="dot" id="dot"></span><span id="conn">Connecting…</span></div>
      <div class="meta" id="angles">—</div>
    </div>

    <button class="open" id="btnOpen">Open Gate</button>
    <div class="row">
      <button class="btn" id="btnHome">Go Home</button>
      <button class="btn" id="btnRefresh">Refresh</button>
    </div>

    <div class="field">
      <label>Home angle <span class="val" id="homeVal">—</span></label>
      <input type="range" id="home" min="0" max="180" value="90"/>
    </div>
    <div class="field">
      <label>Press angle <span class="val" id="pressVal">—</span></label>
      <input type="range" id="press" min="0" max="180" value="70"/>
    </div>

    <p class="msg" id="msg">Loading API…</p>

    <div class="api">
      API v1 · same host as this page<br/>
      POST /api/v1/gate/open · PUT /api/v1/config
    </div>
  </main>
  <script>
    /**
     * Reusable GateBot client.
     * SoftAP: leave baseUrl empty (same origin → http://192.168.4.1)
     * Home Wi‑Fi later: GateBotAPI.baseUrl = 'http://gatebot.local' or your backend.
     */
    const GateBotAPI = {
      baseUrl: '',
      version: 'v1',

      url(path) {
        return this.baseUrl.replace(/\/$/, '') + '/api/' + this.version + path;
      },

      async request(method, path, body) {
        const opts = {
          method,
          headers: { 'Accept': 'application/json' }
        };
        if (body !== undefined) {
          opts.headers['Content-Type'] = 'application/json';
          opts.body = JSON.stringify(body);
        }
        const res = await fetch(this.url(path), opts);
        const data = await res.json().catch(() => ({}));
        if (!res.ok || data.ok === false) {
          throw new Error(data.error || ('HTTP ' + res.status));
        }
        return data;
      },

      health() { return this.request('GET', '/health'); },
      status() { return this.request('GET', '/status'); },
      getConfig() { return this.request('GET', '/config'); },
      setConfig(cfg) { return this.request('PUT', '/config', cfg); },
      openGate() { return this.request('POST', '/gate/open'); },
      goHome() { return this.request('POST', '/gate/home'); },
      setAngle(angle) { return this.request('POST', '/servo/angle', { angle }); }
    };

    // Expose for console / future apps
    window.GateBotAPI = GateBotAPI;

    const els = {
      msg: document.getElementById('msg'),
      btnOpen: document.getElementById('btnOpen'),
      btnHome: document.getElementById('btnHome'),
      btnRefresh: document.getElementById('btnRefresh'),
      home: document.getElementById('home'),
      press: document.getElementById('press'),
      homeVal: document.getElementById('homeVal'),
      pressVal: document.getElementById('pressVal'),
      angles: document.getElementById('angles'),
      conn: document.getElementById('conn'),
      dot: document.getElementById('dot')
    };

    function setMsg(text, kind) {
      els.msg.textContent = text;
      els.msg.className = 'msg' + (kind ? ' ' + kind : '');
    }

    function setOnline(ok) {
      els.dot.classList.toggle('off', !ok);
      els.conn.textContent = ok ? 'API online' : 'API offline';
    }

    function applyConfig(s) {
      const home = s.homeAngle ?? s.servo?.homeAngle;
      const press = s.pressAngle ?? s.servo?.pressAngle;
      if (home != null) {
        els.home.value = home;
        els.homeVal.textContent = home + '°';
      }
      if (press != null) {
        els.press.value = press;
        els.pressVal.textContent = press + '°';
      }
      if (home != null && press != null) {
        els.angles.textContent = 'home ' + home + '° / press ' + press + '°';
      }
    }

    async function refresh() {
      try {
        const s = await GateBotAPI.status();
        applyConfig(s);
        setOnline(true);
        setMsg('Synced via GET /api/v1/status', 'ok');
      } catch (e) {
        setOnline(false);
        setMsg(e.message, 'err');
      }
    }

    async function openGate() {
      els.btnOpen.disabled = true;
      setMsg('POST /api/v1/gate/open …');
      try {
        const s = await GateBotAPI.openGate();
        applyConfig(s);
        setOnline(true);
        setMsg('Gate open complete', 'ok');
      } catch (e) {
        setMsg(e.message, 'err');
      } finally {
        els.btnOpen.disabled = false;
      }
    }

    async function goHome() {
      try {
        const s = await GateBotAPI.goHome();
        applyConfig(s);
        setMsg('At home (POST /api/v1/gate/home)', 'ok');
      } catch (e) {
        setMsg(e.message, 'err');
      }
    }

    async function saveConfig() {
      const homeAngle = Number(els.home.value);
      const pressAngle = Number(els.press.value);
      els.homeVal.textContent = homeAngle + '°';
      els.pressVal.textContent = pressAngle + '°';
      try {
        const s = await GateBotAPI.setConfig({ homeAngle, pressAngle });
        applyConfig(s);
        setMsg('Saved PUT /api/v1/config', 'ok');
      } catch (e) {
        setMsg(e.message, 'err');
      }
    }

    els.btnOpen.addEventListener('click', openGate);
    els.btnHome.addEventListener('click', goHome);
    els.btnRefresh.addEventListener('click', refresh);
    els.home.addEventListener('input', () => {
      els.homeVal.textContent = els.home.value + '°';
    });
    els.press.addEventListener('input', () => {
      els.pressVal.textContent = els.press.value + '°';
    });
    els.home.addEventListener('change', saveConfig);
    els.press.addEventListener('change', saveConfig);

    refresh();
  </script>
</body>
</html>
)HTML";
