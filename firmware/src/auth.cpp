#include <Arduino.h>
#include <ArduinoJson.h>
#include "auth.h"
#include "config.h"

static char sessionToken[SESSION_TOKEN_LEN + 1] = {0};
static bool sessionActive = false;

static void sendUnauthorized(WebServer& server) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Credentials", "true");
  server.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\"}");
}

void authBegin() {
  sessionActive = false;
  memset(sessionToken, 0, sizeof(sessionToken));
}

static void generateToken(char* out, size_t len) {
  static const char* hex = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    out[i] = hex[esp_random() & 0x0F];
  }
  out[len] = '\0';
}

static String extractCookie(WebServer& server, const char* name) {
  if (!server.hasHeader("Cookie")) return "";
  String cookie = server.header("Cookie");
  String key = String(name) + "=";
  int start = cookie.indexOf(key);
  if (start < 0) return "";
  start += key.length();
  int end = cookie.indexOf(';', start);
  if (end < 0) end = cookie.length();
  String val = cookie.substring(start, end);
  val.trim();
  return val;
}

bool authCheckCredentials(const String& email, const String& password) {
  return email.equalsIgnoreCase(ADMIN_EMAIL) && password.equals(ADMIN_PASSWORD);
}

bool authIsLoggedIn(WebServer& server) {
  if (!sessionActive || sessionToken[0] == '\0') return false;
  String cookie = extractCookie(server, SESSION_COOKIE_NAME);
  return cookie.length() == SESSION_TOKEN_LEN && cookie.equals(sessionToken);
}

void authIssueSession(WebServer& server) {
  generateToken(sessionToken, SESSION_TOKEN_LEN);
  sessionActive = true;
  String cookie = String(SESSION_COOKIE_NAME) + "=" + sessionToken +
                  "; Path=/; HttpOnly; SameSite=Lax";
  server.sendHeader("Set-Cookie", cookie);
  Serial.println("[auth] session issued");
}

void authClearSession(WebServer& server) {
  sessionActive = false;
  memset(sessionToken, 0, sizeof(sessionToken));
  String cookie = String(SESSION_COOKIE_NAME) +
                  "=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax";
  server.sendHeader("Set-Cookie", cookie);
  Serial.println("[auth] session cleared");
}

bool requireAuth(WebServer& server) {
  if (authIsLoggedIn(server)) return true;
  sendUnauthorized(server);
  return false;
}
