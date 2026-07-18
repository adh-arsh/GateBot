#pragma once

#include <Arduino.h>

enum WifiMode : uint8_t { GB_WIFI_AP = 0, GB_WIFI_STA = 1 };

void wifiManagerBegin();
void wifiManagerApplySaved();  // SoftAP or STA+fallback; call before HTTP server

WifiMode wifiManagerSavedMode();
bool wifiManagerIsStaConnected();
String wifiManagerShareUrl();       // public unlock base URL
String wifiManagerAdminUrl();       // admin page URL
String wifiManagerIp();
String wifiManagerStaSsid();
int wifiManagerRssi();
String wifiManagerStatusJson();     // full status object (no outer ok)
String wifiManagerScanJson();       // {"networks":[...]}

void wifiManagerSaveMode(WifiMode mode);
void wifiManagerSaveSta(const char* ssid, const char* password);
void wifiManagerSetApMode();        // start SoftAP now (no reboot)
bool wifiManagerConnectSta(const char* ssid, const char* password);  // blocking
void wifiManagerRequestReboot();    // ESP.restart after short delay via flag
bool wifiManagerConsumeRebootRequest();

// SoftAP captive portal (DNS hijack so phones open GateBot instead of "No internet")
bool wifiManagerIsApActive();
void wifiManagerStartCaptiveDns();
void wifiManagerStopCaptiveDns();
void wifiManagerProcessDns();
