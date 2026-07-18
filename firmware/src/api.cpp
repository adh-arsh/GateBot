/**
 * GateBot HTTP API v1
 *
 * Public:  health, unlock, admin/login
 * Admin:   status, config, gate, servo, pins, admin/me|logout
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "api.h"
#include "auth.h"
#include "config.h"
#include "pin_storage.h"
#include "wifi_manager.h"

static WebServer* apiServer = nullptr;

static uint8_t unlockFailCount = 0;
static unsigned long unlockLockUntilMs = 0;

static void sendJson(int code, const String& body) {
  apiServer->sendHeader("Access-Control-Allow-Origin", "*");
  apiServer->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  apiServer->sendHeader("Access-Control-Allow-Headers", "Content-Type");
  apiServer->sendHeader("Access-Control-Allow-Credentials", "true");
  apiServer->send(code, "application/json", body);
}

static void sendError(int code, const char* message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = message;
  String out;
  serializeJson(doc, out);
  sendJson(code, out);
}

static bool parseJsonBody(JsonDocument& doc) {
  if (apiServer->hasArg("plain") && apiServer->arg("plain").length() > 0) {
    return !deserializeJson(doc, apiServer->arg("plain"));
  }
  return false;
}

static void runPressAndRespond(const char* action) {
  if (pressBusy) {
    sendError(409, "busy");
    return;
  }
  startPress();
  while (pressBusy) {
    servicePress();
    delay(1);
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["action"] = action;
  doc["homeAngle"] = homeAngle;
  doc["pressAngle"] = pressAngle;
  doc["angle"] = currentAngle;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

String deviceStatusJson() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["device"] = "gatebot";
  doc["firmware"] = "2.3.0-local";
  doc["busy"] = pressBusy;
  doc["persisted"] = settingsFromNvs;
  doc["pinCount"] = pinStorageCount();
  doc["ip"] = wifiManagerIp();
  doc["shareUrl"] = wifiManagerShareUrl();
  doc["adminUrl"] = wifiManagerAdminUrl();

  // Nested wifi status
  JsonDocument wifiDoc;
  deserializeJson(wifiDoc, wifiManagerStatusJson());
  doc["wifi"] = wifiDoc;
  doc["mode"] = wifiDoc["mode"];
  doc["ssid"] = wifiDoc["ssid"];

  doc["servo"]["pin"] = SERVO_PIN;
  doc["servo"]["angle"] = currentAngle;
  doc["servo"]["homeAngle"] = homeAngle;
  doc["servo"]["pressAngle"] = pressAngle;
  doc["servo"]["pressHoldMs"] = PRESS_HOLD_MS;
  doc["factory"]["homeAngle"] = SERVO_HOME_ANGLE;
  doc["factory"]["pressAngle"] = SERVO_PRESS_ANGLE;
  String out;
  serializeJson(doc, out);
  return out;
}

static void handleOptions() {
  apiServer->sendHeader("Access-Control-Allow-Origin", "*");
  apiServer->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  apiServer->sendHeader("Access-Control-Allow-Headers", "Content-Type");
  apiServer->sendHeader("Access-Control-Allow-Credentials", "true");
  apiServer->send(204);
}

static void handleApiIndex() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["name"] = "GateBot API";
  doc["version"] = "v1";
  JsonArray endpoints = doc["endpoints"].to<JsonArray>();
  endpoints.add("GET /api/v1/health");
  endpoints.add("POST /api/v1/unlock");
  endpoints.add("POST /api/v1/admin/login");
  endpoints.add("POST /api/v1/admin/logout");
  endpoints.add("GET /api/v1/admin/me");
  endpoints.add("GET /api/v1/status");
  endpoints.add("GET|PUT|DELETE /api/v1/config");
  endpoints.add("POST /api/v1/gate/open");
  endpoints.add("POST /api/v1/gate/home");
  endpoints.add("POST /api/v1/servo/angle");
  endpoints.add("GET|POST /api/v1/pins");
  endpoints.add("DELETE /api/v1/pins?id=N");
  endpoints.add("GET /api/v1/wifi/status");
  endpoints.add("GET /api/v1/wifi/scan");
  endpoints.add("PUT /api/v1/wifi/mode");
  endpoints.add("PUT /api/v1/wifi/sta");
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleHealth() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["status"] = "up";
  doc["uptimeMs"] = millis();
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleUnlock() {
  unsigned long now = millis();
  if (now < unlockLockUntilMs) {
    sendError(429, "too many attempts — try again shortly");
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc) && !apiServer->hasArg("pin")) {
    sendError(400, "JSON body {\"pin\":\"######\"} required");
    return;
  }
  String pin;
  if (!doc["pin"].isNull()) pin = doc["pin"].as<String>();
  else pin = apiServer->arg("pin");
  pin.trim();

  if (pin.length() != PIN_LENGTH) {
    sendError(400, "pin must be 6 digits");
    return;
  }

  if (!pinStorageVerify(pin.c_str())) {
    unlockFailCount++;
    if (unlockFailCount >= PIN_FAIL_LIMIT) {
      unlockLockUntilMs = now + PIN_LOCKOUT_MS;
      unlockFailCount = 0;
      Serial.println("[unlock] lockout started");
      sendError(429, "too many attempts — locked for 30s");
      return;
    }
    sendError(401, "invalid pin");
    return;
  }

  unlockFailCount = 0;
  Serial.println("[unlock] PIN ok — opening gate");
  runPressAndRespond("unlock");
}

static void handleAdminLogin() {
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    sendError(400, "JSON body required");
    return;
  }
  String email = doc["email"] | "";
  String password = doc["password"] | "";
  email.trim();
  if (!authCheckCredentials(email, password)) {
    sendError(401, "invalid credentials");
    return;
  }
  authIssueSession(*apiServer);
  JsonDocument out;
  out["ok"] = true;
  out["email"] = ADMIN_EMAIL;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleAdminLogout() {
  if (!requireAuth(*apiServer)) return;
  authClearSession(*apiServer);
  sendJson(200, "{\"ok\":true}");
}

static void handleAdminMe() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument out;
  out["ok"] = true;
  out["email"] = ADMIN_EMAIL;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleStatus() {
  if (!requireAuth(*apiServer)) return;
  sendJson(200, deviceStatusJson());
}

static void handleGetConfig() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  doc["ok"] = true;
  doc["homeAngle"] = homeAngle;
  doc["pressAngle"] = pressAngle;
  doc["angle"] = currentAngle;
  doc["persisted"] = settingsFromNvs;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static bool applyConfigFromDoc(JsonDocument& doc, String& err) {
  bool changed = false;
  if (!doc["homeAngle"].isNull()) {
    int v = doc["homeAngle"].as<int>();
    if (v < 0 || v > 180) {
      err = "homeAngle must be 0-180";
      return false;
    }
    homeAngle = v;
    changed = true;
  }
  if (!doc["pressAngle"].isNull()) {
    int v = doc["pressAngle"].as<int>();
    if (v < 0 || v > 180) {
      err = "pressAngle must be 0-180";
      return false;
    }
    pressAngle = v;
    changed = true;
  }
  if (!doc["home"].isNull()) {
    homeAngle = constrain(doc["home"].as<int>(), 0, 180);
    changed = true;
  }
  if (!doc["press"].isNull()) {
    pressAngle = constrain(doc["press"].as<int>(), 0, 180);
    changed = true;
  }
  if (!changed) {
    err = "expected homeAngle and/or pressAngle";
    return false;
  }
  return true;
}

static void handlePutConfig() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  String err;

  if (apiServer->hasArg("plain") && apiServer->arg("plain").length() > 0) {
    if (deserializeJson(doc, apiServer->arg("plain"))) {
      sendError(400, "invalid JSON body");
      return;
    }
  } else if (apiServer->hasArg("homeAngle") || apiServer->hasArg("pressAngle") ||
             apiServer->hasArg("home") || apiServer->hasArg("press")) {
    if (apiServer->hasArg("homeAngle")) doc["homeAngle"] = apiServer->arg("homeAngle").toInt();
    if (apiServer->hasArg("pressAngle")) doc["pressAngle"] = apiServer->arg("pressAngle").toInt();
    if (apiServer->hasArg("home")) doc["home"] = apiServer->arg("home").toInt();
    if (apiServer->hasArg("press")) doc["press"] = apiServer->arg("press").toInt();
  } else {
    sendError(400, "JSON body or query params required");
    return;
  }

  if (!applyConfigFromDoc(doc, err)) {
    sendError(400, err.c_str());
    return;
  }

  Serial.printf("[api] config homeAngle=%d pressAngle=%d\n", homeAngle, pressAngle);
  if (!doc["homeAngle"].isNull() || !doc["home"].isNull()) {
    moveServo(homeAngle);
  }
  persistAngles();

  JsonDocument out;
  out["ok"] = true;
  out["homeAngle"] = homeAngle;
  out["pressAngle"] = pressAngle;
  out["angle"] = currentAngle;
  out["persisted"] = settingsFromNvs;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleDeleteConfig() {
  if (!requireAuth(*apiServer)) return;
  resetToFactoryDefaults();
  JsonDocument out;
  out["ok"] = true;
  out["action"] = "reset";
  out["homeAngle"] = homeAngle;
  out["pressAngle"] = pressAngle;
  out["angle"] = currentAngle;
  out["persisted"] = settingsFromNvs;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleGateOpen() {
  if (!requireAuth(*apiServer)) return;
  runPressAndRespond("open");
}

static void handleGateHome() {
  if (!requireAuth(*apiServer)) return;
  moveServo(homeAngle);
  JsonDocument doc;
  doc["ok"] = true;
  doc["action"] = "home";
  doc["angle"] = currentAngle;
  doc["homeAngle"] = homeAngle;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleServoAngle() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  int angle = -1;

  if (apiServer->hasArg("plain") && apiServer->arg("plain").length() > 0) {
    if (deserializeJson(doc, apiServer->arg("plain"))) {
      sendError(400, "invalid JSON body");
      return;
    }
    if (!doc["angle"].isNull()) angle = doc["angle"].as<int>();
  }
  if (angle < 0 && apiServer->hasArg("angle")) {
    angle = apiServer->arg("angle").toInt();
  }
  if (angle < 0 || angle > 180) {
    sendError(400, "angle must be 0-180");
    return;
  }

  moveServo(angle);
  JsonDocument out;
  out["ok"] = true;
  out["action"] = "servo";
  out["angle"] = currentAngle;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleListPins() {
  if (!requireAuth(*apiServer)) return;
  PinEntry entries[PIN_MAX_COUNT];
  int count = 0;
  pinStorageList(entries, PIN_MAX_COUNT, &count);

  JsonDocument doc;
  doc["ok"] = true;
  doc["count"] = count;
  doc["max"] = PIN_MAX_COUNT;
  JsonArray arr = doc["pins"].to<JsonArray>();
  for (int i = 0; i < count; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = entries[i].id;
    o["label"] = entries[i].label;
    o["createdAt"] = entries[i].createdAt;
  }
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleCreatePin() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    sendError(400, "JSON body {\"pin\":\"######\",\"label\":\"...\"} required");
    return;
  }
  String pin = doc["pin"] | "";
  String label = doc["label"] | "";
  pin.trim();
  label.trim();

  char err[64];
  int id = pinStorageCreate(pin.c_str(), label.c_str(), err, sizeof(err));
  if (id < 0) {
    sendError(400, err);
    return;
  }

  JsonDocument out;
  out["ok"] = true;
  out["id"] = id;
  out["label"] = label.length() ? label : ("PIN " + String(id));
  out["pin"] = pin;  // shown once at create
  String body;
  serializeJson(out, body);
  sendJson(201, body);
}

static void handleDeletePin() {
  if (!requireAuth(*apiServer)) return;
  if (!apiServer->hasArg("id")) {
    sendError(400, "id query param required");
    return;
  }
  int id = apiServer->arg("id").toInt();
  if (id < 0 || id >= PIN_MAX_COUNT || !pinStorageDelete((uint8_t)id)) {
    sendError(404, "pin not found");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

static void handleWifiStatus() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  deserializeJson(doc, wifiManagerStatusJson());
  doc["ok"] = true;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleWifiScan() {
  if (!requireAuth(*apiServer)) return;
  sendJson(200, wifiManagerScanJson());
}

static void handleWifiMode() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    sendError(400, "JSON body {\"mode\":\"ap\"|\"sta\"} required");
    return;
  }
  String mode = doc["mode"] | "";
  mode.toLowerCase();
  if (mode == "ap") {
    wifiManagerSaveMode(GB_WIFI_AP);
    wifiManagerSetApMode();
    JsonDocument out;
    deserializeJson(out, wifiManagerStatusJson());
    out["ok"] = true;
    out["rebooting"] = false;
    String body;
    serializeJson(out, body);
    sendJson(200, body);
    return;
  }
  if (mode == "sta") {
    wifiManagerSaveMode(GB_WIFI_STA);
    JsonDocument st;
    deserializeJson(st, wifiManagerStatusJson());
    String ssid = st["staSsid"] | "";
    if (ssid.length() == 0) {
      JsonDocument out;
      deserializeJson(out, wifiManagerStatusJson());
      out["ok"] = true;
      out["rebooting"] = false;
      out["message"] = "Wi-Fi mode selected — scan and connect to a network";
      String body;
      serializeJson(out, body);
      sendJson(200, body);
      return;
    }
    sendJson(200, "{\"ok\":true,\"rebooting\":true,\"message\":\"Rebooting into Wi-Fi mode…\"}");
    wifiManagerRequestReboot();
    return;
  }
  sendError(400, "mode must be ap or sta");
}

static void handleWifiSta() {
  if (!requireAuth(*apiServer)) return;
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    sendError(400, "JSON body {\"ssid\",\"password\"} required");
    return;
  }
  String ssid = doc["ssid"] | "";
  String password = doc["password"] | "";
  ssid.trim();
  if (ssid.length() == 0 || ssid.length() > WIFI_SSID_MAX) {
    sendError(400, "ssid required (max 32 chars)");
    return;
  }
  if (password.length() > WIFI_PASS_MAX) {
    sendError(400, "password too long");
    return;
  }

  wifiManagerSaveSta(ssid.c_str(), password.c_str());
  sendJson(200,
           "{\"ok\":true,\"rebooting\":true,\"message\":\"Saved. Rebooting to join Wi-Fi. "
           "Open admin on your home network using the new IP.\"}");
  wifiManagerRequestReboot();
}

void setupApiRoutes(WebServer& server) {
  apiServer = &server;

  auto opt = []() { handleOptions(); };

  server.on("/api/v1", HTTP_GET, handleApiIndex);
  server.on("/api/v1/", HTTP_GET, handleApiIndex);

  server.on("/api/v1/health", HTTP_GET, handleHealth);
  server.on("/api/v1/health", HTTP_OPTIONS, opt);

  server.on("/api/v1/unlock", HTTP_POST, handleUnlock);
  server.on("/api/v1/unlock", HTTP_OPTIONS, opt);

  server.on("/api/v1/admin/login", HTTP_POST, handleAdminLogin);
  server.on("/api/v1/admin/login", HTTP_OPTIONS, opt);
  server.on("/api/v1/admin/logout", HTTP_POST, handleAdminLogout);
  server.on("/api/v1/admin/logout", HTTP_OPTIONS, opt);
  server.on("/api/v1/admin/me", HTTP_GET, handleAdminMe);
  server.on("/api/v1/admin/me", HTTP_OPTIONS, opt);

  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/status", HTTP_OPTIONS, opt);

  server.on("/api/v1/config", HTTP_GET, handleGetConfig);
  server.on("/api/v1/config", HTTP_PUT, handlePutConfig);
  server.on("/api/v1/config", HTTP_POST, handlePutConfig);
  server.on("/api/v1/config", HTTP_DELETE, handleDeleteConfig);
  server.on("/api/v1/config", HTTP_OPTIONS, opt);

  server.on("/api/v1/gate/open", HTTP_POST, handleGateOpen);
  server.on("/api/v1/gate/open", HTTP_OPTIONS, opt);

  server.on("/api/v1/gate/home", HTTP_POST, handleGateHome);
  server.on("/api/v1/gate/home", HTTP_OPTIONS, opt);

  server.on("/api/v1/servo/angle", HTTP_POST, handleServoAngle);
  server.on("/api/v1/servo/angle", HTTP_OPTIONS, opt);

  server.on("/api/v1/pins", HTTP_GET, handleListPins);
  server.on("/api/v1/pins", HTTP_POST, handleCreatePin);
  server.on("/api/v1/pins", HTTP_DELETE, handleDeletePin);
  server.on("/api/v1/pins", HTTP_OPTIONS, opt);

  server.on("/api/v1/wifi/status", HTTP_GET, handleWifiStatus);
  server.on("/api/v1/wifi/status", HTTP_OPTIONS, opt);
  server.on("/api/v1/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/api/v1/wifi/scan", HTTP_OPTIONS, opt);
  server.on("/api/v1/wifi/mode", HTTP_PUT, handleWifiMode);
  server.on("/api/v1/wifi/mode", HTTP_POST, handleWifiMode);
  server.on("/api/v1/wifi/mode", HTTP_OPTIONS, opt);
  server.on("/api/v1/wifi/sta", HTTP_PUT, handleWifiSta);
  server.on("/api/v1/wifi/sta", HTTP_POST, handleWifiSta);
  server.on("/api/v1/wifi/sta", HTTP_OPTIONS, opt);
}
