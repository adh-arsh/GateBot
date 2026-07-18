#pragma once

#include <Arduino.h>

// Shared CSS for GateBot pages (inlined in each page for PROGMEM simplicity).

static const char PIN_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"/>
  <title>GateBot</title>
  <style>
    :root {
      --bg:#0f1412; --panel:#1a2420; --ink:#e8f0ea; --muted:#8fa398;
      --accent:#3dd68c; --danger:#ff6b4a; --line:#2a3830;
      --font:"IBM Plex Sans","Segoe UI",sans-serif;
      --display:"IBM Plex Mono",ui-monospace,monospace;
    }
    *{box-sizing:border-box}
    body{
      margin:0;min-height:100vh;font-family:var(--font);color:var(--ink);
      background:radial-gradient(1200px 600px at 10% -10%,#1e3d30 0%,transparent 55%),
                 radial-gradient(900px 500px at 100% 0%,#243028 0%,transparent 50%),var(--bg);
      display:flex;align-items:center;justify-content:center;padding:24px 16px;
    }
    main{width:100%;max-width:360px}
    .brand{font-family:var(--display);font-size:1.75rem;letter-spacing:.04em;margin:0 0 4px;text-align:center}
    .sub{color:var(--muted);margin:0 0 28px;text-align:center;font-size:.95rem}
    .display{
      font-family:var(--display);font-size:2rem;letter-spacing:.35em;text-align:center;
      padding:18px;border:1px solid var(--line);background:var(--panel);margin-bottom:18px;
      min-height:1.4em;
    }
    .pad{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
    .key{
      border:1px solid var(--line);background:var(--panel);color:var(--ink);
      font-family:var(--display);font-size:1.4rem;padding:18px 0;cursor:pointer;
    }
    .key:active{background:#22302a}
    .key.go{
      grid-column:1/-1;border:0;font-weight:700;letter-spacing:.08em;text-transform:uppercase;
      color:#04140c;background:linear-gradient(180deg,#57e6a0,var(--accent));padding:20px;
    }
    .key.go:disabled{opacity:.55;cursor:wait}
    .msg{min-height:1.4em;margin-top:14px;text-align:center;font-family:var(--display);font-size:.85rem;color:var(--muted)}
    .msg.ok{color:var(--accent)}.msg.err{color:var(--danger)}
  </style>
</head>
<body>
  <main>
    <h1 class="brand">GateBot</h1>
    <p class="sub">Enter 6-digit PIN</p>
    <div class="display" id="disp">••••••</div>
    <div class="pad" id="pad"></div>
    <p class="msg" id="msg">Ready</p>
  </main>
  <script>
    const GateBotAPI = {
      async unlock(pin) {
        const res = await fetch('/api/v1/unlock', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
          body: JSON.stringify({ pin })
        });
        const data = await res.json().catch(() => ({}));
        if (!res.ok || data.ok === false) throw new Error(data.error || ('HTTP ' + res.status));
        return data;
      }
    };
    let pin = '';
    const disp = document.getElementById('disp');
    const msg = document.getElementById('msg');
    const pad = document.getElementById('pad');

    function render() {
      disp.textContent = pin.length ? pin.replace(/./g, '•').padEnd(6, '•').slice(0, 6).split('').map((c,i)=> i < pin.length ? '●' : '•').join('') : '••••••';
      if (pin.length) disp.textContent = pin.split('').map(()=> '●').join('') + '•'.repeat(6 - pin.length);
    }
    function setMsg(t, k) { msg.textContent = t; msg.className = 'msg' + (k ? ' ' + k : ''); }

    async function submit() {
      if (pin.length !== 6) { setMsg('Enter 6 digits', 'err'); return; }
      const btn = document.getElementById('go');
      btn.disabled = true;
      setMsg('Unlocking…');
      try {
        await GateBotAPI.unlock(pin);
        setMsg('Gate opening', 'ok');
        pin = '';
        render();
      } catch (e) {
        setMsg(e.message, 'err');
        pin = '';
        render();
      } finally {
        btn.disabled = false;
      }
    }

    const keys = ['1','2','3','4','5','6','7','8','9','C','0','⌫'];
    keys.forEach(k => {
      const b = document.createElement('button');
      b.className = 'key';
      b.type = 'button';
      b.textContent = k;
      b.onclick = () => {
        if (k === 'C') { pin = ''; render(); setMsg('Cleared'); return; }
        if (k === '⌫') { pin = pin.slice(0, -1); render(); return; }
        if (pin.length >= 6) return;
        pin += k;
        render();
        if (pin.length === 6) submit();
      };
      pad.appendChild(b);
    });
    const go = document.createElement('button');
    go.className = 'key go';
    go.id = 'go';
    go.type = 'button';
    go.textContent = 'Unlock';
    go.onclick = submit;
    pad.appendChild(go);
    render();
  </script>
</body>
</html>
)HTML";

static const char ADMIN_LOGIN_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"/>
  <title>GateBot Admin</title>
  <style>
    :root {
      --bg:#0f1412; --panel:#1a2420; --ink:#e8f0ea; --muted:#8fa398;
      --accent:#3dd68c; --danger:#ff6b4a; --line:#2a3830;
      --font:"IBM Plex Sans","Segoe UI",sans-serif;
      --display:"IBM Plex Mono",ui-monospace,monospace;
    }
    *{box-sizing:border-box}
    body{
      margin:0;min-height:100vh;font-family:var(--font);color:var(--ink);
      background:radial-gradient(1200px 600px at 10% -10%,#1e3d30 0%,transparent 55%),var(--bg);
      display:flex;align-items:center;justify-content:center;padding:24px 16px;
    }
    main{width:100%;max-width:400px}
    .brand{font-family:var(--display);font-size:1.5rem;margin:0 0 4px}
    .sub{color:var(--muted);margin:0 0 24px}
    label{display:block;font-size:.75rem;color:var(--muted);text-transform:uppercase;letter-spacing:.08em;margin:0 0 6px}
    input{
      width:100%;padding:12px 14px;margin-bottom:14px;border:1px solid var(--line);
      background:var(--panel);color:var(--ink);font-size:1rem;
    }
    button{
      width:100%;border:0;padding:16px;font-weight:700;letter-spacing:.06em;text-transform:uppercase;
      color:#04140c;background:linear-gradient(180deg,#57e6a0,var(--accent));cursor:pointer;
    }
    button:disabled{opacity:.55}
    .msg{margin-top:14px;font-family:var(--display);font-size:.85rem;color:var(--muted);min-height:1.3em}
    .msg.err{color:var(--danger)}
  </style>
</head>
<body>
  <main>
    <h1 class="brand">GateBot Admin</h1>
    <p class="sub">Sign in to manage settings &amp; PINs</p>
    <form id="form">
      <label>Email</label>
      <input id="email" type="email" autocomplete="username" required/>
      <label>Password</label>
      <input id="password" type="password" autocomplete="current-password" required/>
      <button type="submit" id="btn">Sign in</button>
    </form>
    <p class="msg" id="msg"></p>
  </main>
  <script>
    const form = document.getElementById('form');
    const msg = document.getElementById('msg');
    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      const btn = document.getElementById('btn');
      btn.disabled = true;
      msg.textContent = 'Signing in…';
      msg.className = 'msg';
      try {
        const res = await fetch('/api/v1/admin/login', {
          method: 'POST',
          credentials: 'include',
          headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
          body: JSON.stringify({
            email: document.getElementById('email').value.trim(),
            password: document.getElementById('password').value
          })
        });
        const data = await res.json().catch(() => ({}));
        if (!res.ok || data.ok === false) throw new Error(data.error || 'Login failed');
        location.href = '/admin';
      } catch (err) {
        msg.textContent = err.message;
        msg.className = 'msg err';
        btn.disabled = false;
      }
    });
  </script>
</body>
</html>
)HTML";

static const char ADMIN_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"/>
  <title>GateBot Admin</title>
  <style>
    :root {
      --bg:#0f1412; --panel:#1a2420; --ink:#e8f0ea; --muted:#8fa398;
      --accent:#3dd68c; --danger:#ff6b4a; --line:#2a3830;
      --font:"IBM Plex Sans","Segoe UI",sans-serif;
      --display:"IBM Plex Mono",ui-monospace,monospace;
    }
    *{box-sizing:border-box}
    body{
      margin:0;min-height:100vh;font-family:var(--font);color:var(--ink);
      background:radial-gradient(1200px 600px at 10% -10%,#1e3d30 0%,transparent 55%),
                 radial-gradient(900px 500px at 100% 0%,#243028 0%,transparent 50%),var(--bg);
    }
    main{max-width:420px;margin:0 auto;padding:28px 20px 48px}
    .brand{font-family:var(--display);font-size:1.75rem;letter-spacing:.04em;margin:0 0 4px}
    .sub{color:var(--muted);margin:0 0 20px;font-size:.95rem}
    .top{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;gap:10px}
    .status{
      display:flex;justify-content:space-between;align-items:center;padding:12px 14px;
      border:1px solid var(--line);background:var(--panel);margin-bottom:12px;
    }
    .dot{width:10px;height:10px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent);display:inline-block;margin-right:8px}
    .dot.off{background:var(--danger);box-shadow:none}
    .meta{font-family:var(--display);font-size:.85rem;color:var(--muted)}
    .open{
      width:100%;border:0;padding:22px 16px;font-size:1.25rem;font-weight:700;letter-spacing:.06em;
      text-transform:uppercase;color:#04140c;background:linear-gradient(180deg,#57e6a0,var(--accent));
      cursor:pointer;margin-bottom:12px;
    }
    .open:disabled{opacity:.55;cursor:wait}
    .row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:16px}
    .btn{border:1px solid var(--line);background:var(--panel);color:var(--ink);padding:14px 10px;font-size:.95rem;cursor:pointer}
    .btn:active{background:#22302a}
    .btn.danger{border-color:#5a3030;color:var(--danger)}
    label{display:block;font-size:.8rem;color:var(--muted);margin-bottom:6px;text-transform:uppercase;letter-spacing:.08em}
    .field{margin-bottom:18px}
    .val{float:right;font-family:var(--display);color:var(--accent)}
    input[type=range]{width:100%;accent-color:var(--accent)}
    input[type=text],input[type=password]{
      width:100%;padding:12px;border:1px solid var(--line);background:var(--panel);color:var(--ink);margin-bottom:10px
    }
    .msg{min-height:1.4em;color:var(--muted);font-family:var(--display);font-size:.85rem}
    .msg.ok{color:var(--accent)}.msg.err{color:var(--danger)}
    h2{font-family:var(--display);font-size:1rem;margin:28px 0 12px;letter-spacing:.04em;border-top:1px solid var(--line);padding-top:20px}
    .pin-list{display:flex;flex-direction:column;gap:8px;margin-top:12px}
    .pin-row{
      display:flex;justify-content:space-between;align-items:center;padding:10px 12px;
      border:1px solid var(--line);background:var(--panel);font-family:var(--display);font-size:.85rem
    }
    .pin-row button{border:1px solid var(--line);background:transparent;color:var(--danger);padding:6px 10px;cursor:pointer}
    .toggle{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:14px}
    .toggle button{border:1px solid var(--line);background:var(--panel);color:var(--ink);padding:12px;cursor:pointer;font-size:.9rem}
    .toggle button.on{border-color:var(--accent);color:var(--accent)}
    .share{
      font-family:var(--display);font-size:.85rem;padding:12px;border:1px solid var(--line);
      background:var(--panel);word-break:break-all;margin-bottom:10px
    }
    .net-list{display:flex;flex-direction:column;gap:6px;max-height:220px;overflow:auto;margin:10px 0}
    .net-row{
      display:flex;justify-content:space-between;align-items:center;padding:10px 12px;
      border:1px solid var(--line);background:var(--panel);cursor:pointer;font-family:var(--display);font-size:.8rem
    }
    .net-row.sel{border-color:var(--accent)}
    .note{font-size:.8rem;color:var(--muted);margin:0 0 12px;line-height:1.4}
  </style>
</head>
<body>
  <main>
    <div class="top">
      <div>
        <h1 class="brand">GateBot</h1>
        <p class="sub">Admin settings</p>
      </div>
      <button class="btn" id="btnLogout" type="button">Logout</button>
    </div>

    <div class="status">
      <div><span class="dot" id="dot"></span><span id="conn">Connecting…</span></div>
      <div class="meta" id="angles">—</div>
    </div>
    <p class="meta" id="persist" style="margin:-4px 0 18px;font-size:.8rem;">—</p>

    <button class="open" id="btnOpen">Open Gate</button>
    <div class="row">
      <button class="btn" id="btnHome">Go Home</button>
      <button class="btn" id="btnRefresh">Refresh</button>
    </div>
    <div class="row" style="margin-bottom:24px">
      <button class="btn" id="btnReset" style="grid-column:1/-1">Reset to factory angles</button>
    </div>

    <div class="field">
      <label>Home angle <span class="val" id="homeVal">—</span></label>
      <input type="range" id="home" min="0" max="180" value="40"/>
    </div>
    <div class="field">
      <label>Press angle <span class="val" id="pressVal">—</span></label>
      <input type="range" id="press" min="0" max="180" value="170"/>
    </div>

    <h2>Network</h2>
    <p class="note" id="wifiNote">Choose AP (share link) or Wi‑Fi (internet / WebSocket-ready).</p>
    <div class="toggle">
      <button type="button" id="btnModeAp">AP · no internet</button>
      <button type="button" id="btnModeSta">Wi‑Fi · internet</button>
    </div>
    <div id="panelAp">
      <label>Share link (join GateBot Wi‑Fi first)</label>
      <div class="share" id="shareUrl">http://192.168.4.1/</div>
      <button class="btn" type="button" id="btnCopyShare" style="width:100%;margin-bottom:8px">Copy share link</button>
    </div>
    <div id="panelSta" style="display:none">
      <div id="staConnected" style="display:none">
        <label>Admin URL (same home Wi‑Fi)</label>
        <div class="share" id="adminUrl">—</div>
        <label>Public unlock URL</label>
        <div class="share" id="unlockUrl">—</div>
        <p class="meta" id="staMeta" style="margin-bottom:12px">—</p>
      </div>
      <button class="btn" type="button" id="btnScan" style="width:100%;margin-bottom:8px">Scan networks</button>
      <div class="net-list" id="netList"></div>
      <label>Selected SSID</label>
      <input type="text" id="staSsid" placeholder="Network name" maxlength="32"/>
      <label>Password</label>
      <input type="password" id="staPass" placeholder="Wi‑Fi password" maxlength="64"/>
      <button class="btn" type="button" id="btnConnect" style="width:100%">Connect &amp; reboot</button>
      <p class="note">ESP32 is 2.4 GHz only. After connect, open the Admin URL on your home Wi‑Fi.</p>
    </div>

    <h2>Access PINs</h2>
    <label>New 6-digit PIN</label>
    <input type="text" id="newPin" maxlength="6" inputmode="numeric" pattern="[0-9]*" placeholder="123456"/>
    <label>Label</label>
    <input type="text" id="newLabel" maxlength="24" placeholder="Guest"/>
    <button class="btn" id="btnCreatePin" type="button" style="width:100%;margin-bottom:8px">Create PIN</button>
    <div class="pin-list" id="pinList"></div>

    <p class="msg" id="msg">Loading…</p>
  </main>
  <script>
    const GateBotAPI = {
      baseUrl: '',
      version: 'v1',
      url(path) { return this.baseUrl.replace(/\/$/, '') + '/api/' + this.version + path; },
      async request(method, path, body) {
        const opts = { method, credentials: 'include', headers: { 'Accept': 'application/json' } };
        if (body !== undefined) {
          opts.headers['Content-Type'] = 'application/json';
          opts.body = JSON.stringify(body);
        }
        const res = await fetch(this.url(path), opts);
        if (res.status === 401) { location.href = '/admin'; throw new Error('unauthorized'); }
        const data = await res.json().catch(() => ({}));
        if (!res.ok || data.ok === false) throw new Error(data.error || ('HTTP ' + res.status));
        return data;
      },
      me() { return this.request('GET', '/admin/me'); },
      logout() { return this.request('POST', '/admin/logout'); },
      status() { return this.request('GET', '/status'); },
      setConfig(cfg) { return this.request('PUT', '/config', cfg); },
      resetConfig() { return this.request('DELETE', '/config'); },
      openGate() { return this.request('POST', '/gate/open'); },
      goHome() { return this.request('POST', '/gate/home'); },
      listPins() { return this.request('GET', '/pins'); },
      createPin(pin, label) { return this.request('POST', '/pins', { pin, label }); },
      deletePin(id) { return this.request('DELETE', '/pins?id=' + id); },
      wifiStatus() { return this.request('GET', '/wifi/status'); },
      wifiScan() { return this.request('GET', '/wifi/scan'); },
      wifiMode(mode) { return this.request('PUT', '/wifi/mode', { mode }); },
      wifiSta(ssid, password) { return this.request('PUT', '/wifi/sta', { ssid, password }); }
    };
    window.GateBotAPI = GateBotAPI;

    const els = {
      msg: document.getElementById('msg'),
      btnOpen: document.getElementById('btnOpen'),
      btnHome: document.getElementById('btnHome'),
      btnRefresh: document.getElementById('btnRefresh'),
      btnReset: document.getElementById('btnReset'),
      btnLogout: document.getElementById('btnLogout'),
      btnCreatePin: document.getElementById('btnCreatePin'),
      home: document.getElementById('home'),
      press: document.getElementById('press'),
      homeVal: document.getElementById('homeVal'),
      pressVal: document.getElementById('pressVal'),
      angles: document.getElementById('angles'),
      persist: document.getElementById('persist'),
      conn: document.getElementById('conn'),
      dot: document.getElementById('dot'),
      pinList: document.getElementById('pinList'),
      newPin: document.getElementById('newPin'),
      newLabel: document.getElementById('newLabel'),
      btnModeAp: document.getElementById('btnModeAp'),
      btnModeSta: document.getElementById('btnModeSta'),
      panelAp: document.getElementById('panelAp'),
      panelSta: document.getElementById('panelSta'),
      shareUrl: document.getElementById('shareUrl'),
      adminUrl: document.getElementById('adminUrl'),
      unlockUrl: document.getElementById('unlockUrl'),
      staMeta: document.getElementById('staMeta'),
      staConnected: document.getElementById('staConnected'),
      netList: document.getElementById('netList'),
      staSsid: document.getElementById('staSsid'),
      staPass: document.getElementById('staPass'),
      btnScan: document.getElementById('btnScan'),
      btnConnect: document.getElementById('btnConnect'),
      btnCopyShare: document.getElementById('btnCopyShare'),
      wifiNote: document.getElementById('wifiNote')
    };

    let uiMode = 'ap';

    function setNetworkUi(w) {
      const mode = w.mode || 'ap';
      const saved = w.savedMode || mode;
      uiMode = saved;
      els.btnModeAp.classList.toggle('on', saved === 'ap');
      els.btnModeSta.classList.toggle('on', saved === 'sta');
      els.panelAp.style.display = saved === 'ap' ? 'block' : 'none';
      els.panelSta.style.display = saved === 'sta' ? 'block' : 'none';
      els.shareUrl.textContent = w.shareUrl || 'http://192.168.4.1/';
      if (w.staConnected) {
        els.staConnected.style.display = 'block';
        els.adminUrl.textContent = w.adminUrl || ('http://' + w.ip + '/admin');
        els.unlockUrl.textContent = w.shareUrl || ('http://' + w.ip + '/');
        els.staMeta.textContent = (w.ssid || w.staSsid || '') + ' · ' + (w.rssi || 0) + ' dBm · ' + (w.ip || '');
      } else {
        els.staConnected.style.display = saved === 'sta' ? 'block' : 'none';
        if (saved === 'sta' && w.fallback) {
          els.adminUrl.textContent = 'Not on Wi‑Fi — SoftAP fallback active';
          els.unlockUrl.textContent = w.shareUrl || 'http://192.168.4.1/';
          els.staMeta.textContent = 'Saved SSID: ' + (w.staSsid || '—') + ' (join failed; using AP)';
        }
      }
      if (w.fallback) {
        els.wifiNote.textContent = 'STA failed — SoftAP fallback. Fix password or scan again.';
      } else if (saved === 'ap') {
        els.wifiNote.textContent = 'AP mode: share the link below. Guests join GateBot Wi‑Fi first.';
      } else {
        els.wifiNote.textContent = 'Wi‑Fi mode: device on home network (ready for future WebSocket).';
      }
    }

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
      if (home != null) { els.home.value = home; els.homeVal.textContent = home + '°'; }
      if (press != null) { els.press.value = press; els.pressVal.textContent = press + '°'; }
      if (home != null && press != null) els.angles.textContent = 'home ' + home + '° / press ' + press + '°';
      if (s.persisted === true) els.persist.textContent = 'Saved in flash — survives power-off';
      else if (s.persisted === false) els.persist.textContent = 'Factory defaults (not saved yet)';
    }

    async function refreshPins() {
      const data = await GateBotAPI.listPins();
      els.pinList.innerHTML = '';
      if (!data.pins || !data.pins.length) {
        els.pinList.innerHTML = '<div class="meta">No PINs yet</div>';
        return;
      }
      data.pins.forEach(p => {
        const row = document.createElement('div');
        row.className = 'pin-row';
        row.innerHTML = '<span>#' + p.id + ' · ' + (p.label || 'PIN') + '</span>';
        const del = document.createElement('button');
        del.type = 'button';
        del.textContent = 'Delete';
        del.onclick = async () => {
          if (!confirm('Delete this PIN?')) return;
          try {
            await GateBotAPI.deletePin(p.id);
            setMsg('PIN deleted', 'ok');
            await refreshPins();
          } catch (e) { setMsg(e.message, 'err'); }
        };
        row.appendChild(del);
        els.pinList.appendChild(row);
      });
    }

    async function refresh() {
      try {
        const s = await GateBotAPI.status();
        applyConfig(s);
        if (s.wifi) setNetworkUi(s.wifi);
        else {
          const w = await GateBotAPI.wifiStatus();
          setNetworkUi(w);
        }
        await refreshPins();
        setOnline(true);
        setMsg(s.persisted ? 'Loaded saved angles' : 'Using factory defaults', 'ok');
      } catch (e) {
        setOnline(false);
        setMsg(e.message, 'err');
      }
    }

    async function setMode(mode) {
      try {
        const r = await GateBotAPI.wifiMode(mode);
        if (r.rebooting) {
          setMsg('Rebooting into Wi‑Fi mode… reconnect on home network.', 'ok');
          return;
        }
        setNetworkUi(Object.assign({}, r, { savedMode: mode, mode: r.mode || 'ap' }));
        setMsg(r.message || (mode === 'ap' ? 'AP mode active' : 'Select a network to connect'), 'ok');
        if (mode === 'sta') {
          els.panelAp.style.display = 'none';
          els.panelSta.style.display = 'block';
          els.btnModeAp.classList.remove('on');
          els.btnModeSta.classList.add('on');
        }

    async function scanWifi() {
      setMsg('Scanning…');
      try {
        const data = await GateBotAPI.wifiScan();
        els.netList.innerHTML = '';
        const nets = data.networks || [];
        if (!nets.length) {
          els.netList.innerHTML = '<div class="meta">No networks found (2.4 GHz only)</div>';
          setMsg('Scan complete', 'ok');
          return;
        }
        nets.sort((a, b) => (b.rssi || 0) - (a.rssi || 0));
        nets.forEach(n => {
          const row = document.createElement('div');
          row.className = 'net-row';
          row.innerHTML = '<span>' + (n.ssid || '(hidden)') + '</span><span>' +
            (n.rssi || 0) + ' dBm' + (n.secure ? ' · 🔒' : '') + '</span>';
          row.onclick = () => {
            document.querySelectorAll('.net-row').forEach(el => el.classList.remove('sel'));
            row.classList.add('sel');
            els.staSsid.value = n.ssid || '';
          };
          els.netList.appendChild(row);
        });
        setMsg('Found ' + nets.length + ' networks', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function connectWifi() {
      const ssid = els.staSsid.value.trim();
      const password = els.staPass.value;
      if (!ssid) { setMsg('Select or enter an SSID', 'err'); return; }
      if (!confirm('Connect to "' + ssid + '" and reboot?')) return;
      try {
        await GateBotAPI.wifiSta(ssid, password);
        setMsg('Saved. Rebooting… Then open Admin URL on your home Wi‑Fi.', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function openGate() {
      els.btnOpen.disabled = true;
      setMsg('Opening…');
      try {
        const s = await GateBotAPI.openGate();
        applyConfig(s);
        setMsg('Gate open complete', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
      finally { els.btnOpen.disabled = false; }
    }

    async function goHome() {
      try {
        const s = await GateBotAPI.goHome();
        applyConfig(s);
        setMsg('At home', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function saveConfig() {
      const homeAngle = Number(els.home.value);
      const pressAngle = Number(els.press.value);
      els.homeVal.textContent = homeAngle + '°';
      els.pressVal.textContent = pressAngle + '°';
      try {
        const s = await GateBotAPI.setConfig({ homeAngle, pressAngle });
        applyConfig(s);
        setMsg('Saved to flash', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function resetConfig() {
      if (!confirm('Reset to factory angles?')) return;
      try {
        const s = await GateBotAPI.resetConfig();
        applyConfig(s);
        setMsg('Factory defaults restored', 'ok');
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function createPin() {
      const pin = els.newPin.value.trim();
      const label = els.newLabel.value.trim();
      if (!/^\d{6}$/.test(pin)) { setMsg('PIN must be 6 digits', 'err'); return; }
      try {
        const s = await GateBotAPI.createPin(pin, label);
        els.newPin.value = '';
        els.newLabel.value = '';
        setMsg('Created PIN #' + s.id + (s.pin ? ' · ' + s.pin : ''), 'ok');
        await refreshPins();
      } catch (e) { setMsg(e.message, 'err'); }
    }

    async function logout() {
      try { await GateBotAPI.logout(); } catch (_) {}
      location.href = '/admin';
    }

    els.btnOpen.addEventListener('click', openGate);
    els.btnHome.addEventListener('click', goHome);
    els.btnRefresh.addEventListener('click', refresh);
    els.btnReset.addEventListener('click', resetConfig);
    els.btnLogout.addEventListener('click', logout);
    els.btnCreatePin.addEventListener('click', createPin);
    els.btnModeAp.addEventListener('click', () => setMode('ap'));
    els.btnModeSta.addEventListener('click', () => setMode('sta'));
    els.btnScan.addEventListener('click', scanWifi);
    els.btnConnect.addEventListener('click', connectWifi);
    els.btnCopyShare.addEventListener('click', async () => {
      try {
        await navigator.clipboard.writeText(els.shareUrl.textContent);
        setMsg('Share link copied', 'ok');
      } catch (_) {
        setMsg(els.shareUrl.textContent, 'ok');
      }
    });
    els.home.addEventListener('input', () => { els.homeVal.textContent = els.home.value + '°'; });
    els.press.addEventListener('input', () => { els.pressVal.textContent = els.press.value + '°'; });
    els.home.addEventListener('change', saveConfig);
    els.press.addEventListener('change', saveConfig);

    (async () => {
      try {
        await GateBotAPI.me();
        await refresh();
      } catch (e) {
        location.href = '/admin';
      }
    })();
  </script>
</body>
</html>
)HTML";
