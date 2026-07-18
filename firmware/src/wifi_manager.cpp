#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "wifi_manager.h"
#include "config.h"

static Preferences wifiPrefs;
static const char* WIFI_NS = "wifi";
static WifiMode activeMode = GB_WIFI_AP;
static bool rebootRequested = false;
static DNSServer dnsServer;
static bool dnsActive = false;
static const byte DNS_PORT = 53;

void wifiManagerBegin() {
  wifiPrefs.begin(WIFI_NS, false);
}

WifiMode wifiManagerSavedMode() {
  String m = wifiPrefs.getString("mode", "ap");
  return m.equalsIgnoreCase("sta") ? GB_WIFI_STA : GB_WIFI_AP;
}

static String savedSsid() { return wifiPrefs.getString("ssid", ""); }
static String savedPass() { return wifiPrefs.getString("pass", ""); }

void wifiManagerSaveMode(WifiMode mode) {
  wifiPrefs.putString("mode", mode == GB_WIFI_STA ? "sta" : "ap");
}

void wifiManagerSaveSta(const char* ssid, const char* password) {
  wifiPrefs.putString("ssid", ssid ? ssid : "");
  wifiPrefs.putString("pass", password ? password : "");
  wifiPrefs.putString("mode", "sta");
}

void wifiManagerStopCaptiveDns() {
  if (dnsActive) {
    dnsServer.stop();
    dnsActive = false;
    Serial.println("[wifi] captive DNS stopped");
  }
}

void wifiManagerStartCaptiveDns() {
  wifiManagerStopCaptiveDns();
  IPAddress apIP = WiFi.softAPIP();
  // Reply to EVERY DNS query with SoftAP IP → phone captive portal opens our page
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  dnsActive = true;
  Serial.printf("[wifi] captive DNS on %s:53 (* → SoftAP)\n", apIP.toString().c_str());
}

void wifiManagerProcessDns() {
  if (dnsActive) dnsServer.processNextRequest();
}

bool wifiManagerIsApActive() {
  return activeMode == GB_WIFI_AP;
}

void wifiManagerSetApMode() {
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gateway, subnet);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CLIENTS);
  delay(200);
  activeMode = GB_WIFI_AP;
  Serial.printf("[wifi] SoftAP %s  ssid=%s  ip=%s\n",
                ok ? "OK" : "FAIL", AP_SSID, WiFi.softAPIP().toString().c_str());
  wifiManagerStartCaptiveDns();
}

bool wifiManagerConnectSta(const char* ssid, const char* password) {
  if (!ssid || strlen(ssid) == 0) return false;

  wifiManagerStopCaptiveDns();

  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password ? password : "");

  Serial.printf("[wifi] STA connecting to '%s'…\n", ssid);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_STA_TIMEOUT_MS) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    activeMode = GB_WIFI_STA;
    Serial.printf("[wifi] STA OK  ip=%s  rssi=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }

  Serial.println("[wifi] STA failed");
  return false;
}

void wifiManagerApplySaved() {
  WifiMode mode = wifiManagerSavedMode();
  String ssid = savedSsid();

  if (mode == GB_WIFI_STA && ssid.length() > 0) {
    if (wifiManagerConnectSta(ssid.c_str(), savedPass().c_str())) {
      return;
    }
    Serial.println("[wifi] falling back to SoftAP");
  }
  wifiManagerSetApMode();
}

bool wifiManagerIsStaConnected() {
  return activeMode == GB_WIFI_STA && WiFi.status() == WL_CONNECTED;
}

String wifiManagerIp() {
  if (wifiManagerIsStaConnected()) return WiFi.localIP().toString();
  return WiFi.softAPIP().toString();
}

String wifiManagerShareUrl() {
  return "http://" + wifiManagerIp() + "/";
}

String wifiManagerAdminUrl() {
  return "http://" + wifiManagerIp() + "/admin";
}

String wifiManagerStaSsid() {
  if (wifiManagerIsStaConnected()) return WiFi.SSID();
  return savedSsid();
}

int wifiManagerRssi() {
  if (wifiManagerIsStaConnected()) return WiFi.RSSI();
  return 0;
}

String wifiManagerStatusJson() {
  JsonDocument doc;
  const bool sta = wifiManagerIsStaConnected();
  const bool wantSta = wifiManagerSavedMode() == GB_WIFI_STA;
  doc["mode"] = sta ? "sta" : "ap";
  doc["savedMode"] = wantSta ? "sta" : "ap";
  doc["connected"] = sta || (activeMode == GB_WIFI_AP);
  doc["staConnected"] = sta;
  doc["ssid"] = sta ? WiFi.SSID() : String(AP_SSID);
  doc["staSsid"] = savedSsid();
  doc["ip"] = wifiManagerIp();
  doc["shareUrl"] = wifiManagerShareUrl();
  doc["adminUrl"] = wifiManagerAdminUrl();
  doc["apSsid"] = AP_SSID;
  doc["apPassword"] = AP_PASSWORD;
  doc["rssi"] = wifiManagerRssi();
  doc["fallback"] = wantSta && !sta;
  doc["captivePortal"] = dnsActive;
  String out;
  serializeJson(doc, out);
  return out;
}

String wifiManagerScanJson() {
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);
  JsonDocument doc;
  doc["ok"] = true;
  JsonArray arr = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(doc, out);
  return out;
}

void wifiManagerRequestReboot() {
  rebootRequested = true;
}

bool wifiManagerConsumeRebootRequest() {
  if (!rebootRequested) return false;
  rebootRequested = false;
  return true;
}
