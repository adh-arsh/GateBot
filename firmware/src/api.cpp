/**
 * GateBot HTTP API v1
 *
 * Stable JSON contract — SoftAP today, same paths on home Wi‑Fi / React later.
 *
 *   GET  /api/v1              API index
 *   GET  /api/v1/health       Liveness
 *   GET  /api/v1/status       Full device status
 *   GET  /api/v1/config       Servo config
 *   PUT  /api/v1/config       Update config (JSON body or query)
 *   POST /api/v1/gate/open    Run press sequence
 *   POST /api/v1/gate/home    Move to home angle
 *   POST /api/v1/servo/angle  Move to absolute angle { "angle": 0-180 }
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "api.h"
#include "config.h"

static WebServer* apiServer = nullptr;

static void sendJson(int code, const String& body) {
  apiServer->sendHeader("Access-Control-Allow-Origin", "*");
  apiServer->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  apiServer->sendHeader("Access-Control-Allow-Headers", "Content-Type");
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

String deviceStatusJson() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["device"] = "gatebot";
  doc["firmware"] = "2.0.0-local";
  doc["mode"] = "softap";
  doc["ssid"] = AP_SSID;
  doc["ip"] = WiFi.softAPIP().toString();
  doc["busy"] = pressBusy;
  doc["servo"]["pin"] = SERVO_PIN;
  doc["servo"]["angle"] = currentAngle;
  doc["servo"]["homeAngle"] = homeAngle;
  doc["servo"]["pressAngle"] = pressAngle;
  doc["servo"]["pressHoldMs"] = PRESS_HOLD_MS;
  String out;
  serializeJson(doc, out);
  return out;
}

static void handleOptions() {
  apiServer->sendHeader("Access-Control-Allow-Origin", "*");
  apiServer->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  apiServer->sendHeader("Access-Control-Allow-Headers", "Content-Type");
  apiServer->send(204);
}

static void handleApiIndex() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["name"] = "GateBot API";
  doc["version"] = "v1";
  JsonArray endpoints = doc["endpoints"].to<JsonArray>();
  endpoints.add("GET /api/v1/health");
  endpoints.add("GET /api/v1/status");
  endpoints.add("GET /api/v1/config");
  endpoints.add("PUT /api/v1/config");
  endpoints.add("POST /api/v1/gate/open");
  endpoints.add("POST /api/v1/gate/home");
  endpoints.add("POST /api/v1/servo/angle");
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

static void handleStatus() {
  sendJson(200, deviceStatusJson());
}

static void handleGetConfig() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["homeAngle"] = homeAngle;
  doc["pressAngle"] = pressAngle;
  doc["angle"] = currentAngle;
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
  // Accept short aliases used by simple clients
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
  JsonDocument doc;
  String err;

  // Prefer JSON body; fall back to query params for easy curl/testing
  if (apiServer->hasArg("plain") && apiServer->arg("plain").length() > 0) {
    DeserializationError e = deserializeJson(doc, apiServer->arg("plain"));
    if (e) {
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

  JsonDocument out;
  out["ok"] = true;
  out["homeAngle"] = homeAngle;
  out["pressAngle"] = pressAngle;
  out["angle"] = currentAngle;
  String body;
  serializeJson(out, body);
  sendJson(200, body);
}

static void handleGateOpen() {
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
  doc["action"] = "open";
  doc["homeAngle"] = homeAngle;
  doc["pressAngle"] = pressAngle;
  doc["angle"] = currentAngle;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleGateHome() {
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

void setupApiRoutes(WebServer& server) {
  apiServer = &server;

  auto opt = []() { handleOptions(); };

  server.on("/api/v1", HTTP_GET, handleApiIndex);
  server.on("/api/v1/", HTTP_GET, handleApiIndex);

  server.on("/api/v1/health", HTTP_GET, handleHealth);
  server.on("/api/v1/health", HTTP_OPTIONS, opt);

  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/status", HTTP_OPTIONS, opt);

  server.on("/api/v1/config", HTTP_GET, handleGetConfig);
  server.on("/api/v1/config", HTTP_PUT, handlePutConfig);
  server.on("/api/v1/config", HTTP_POST, handlePutConfig);  // alias for simple clients
  server.on("/api/v1/config", HTTP_OPTIONS, opt);

  server.on("/api/v1/gate/open", HTTP_POST, handleGateOpen);
  server.on("/api/v1/gate/open", HTTP_GET, handleGateOpen);  // browser-friendly test
  server.on("/api/v1/gate/open", HTTP_OPTIONS, opt);

  server.on("/api/v1/gate/home", HTTP_POST, handleGateHome);
  server.on("/api/v1/gate/home", HTTP_GET, handleGateHome);
  server.on("/api/v1/gate/home", HTTP_OPTIONS, opt);

  server.on("/api/v1/servo/angle", HTTP_POST, handleServoAngle);
  server.on("/api/v1/servo/angle", HTTP_GET, handleServoAngle);
  server.on("/api/v1/servo/angle", HTTP_OPTIONS, opt);
}
