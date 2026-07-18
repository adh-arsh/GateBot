/**
 * GateBot — SoftAP + API + NVS-persisted calibration
 *
 * Home/press/last angles are saved to flash and restored after power-off.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "config.h"
#include "config_storage.h"
#include "pin_storage.h"
#include "auth.h"
#include "web_ui.h"
#include "api.h"

Servo gateServo;
WebServer server(80);

int homeAngle = SERVO_HOME_ANGLE;
int pressAngle = SERVO_PRESS_ANGLE;
int currentAngle = SERVO_HOME_ANGLE;
bool settingsFromNvs = false;
bool sweepMode = false;
bool pressBusy = false;
bool servoAttached = false;
bool angleDirty = false;  // flush last angle to NVS on detach

unsigned long lastSweepMs = 0;
unsigned long lastBlinkMs = 0;
unsigned long lastServoActivityMs = 0;
bool ledOn = false;
String serialBuf;

enum PressState : uint8_t { PRESS_IDLE, PRESS_DOWN, PRESS_HOLD, PRESS_UP };
PressState pressState = PRESS_IDLE;
unsigned long pressStateMs = 0;

void blinkLed() {
  unsigned long now = millis();
  if (now - lastBlinkMs >= 500) {
    lastBlinkMs = now;
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }
}

void attachServoIfNeeded() {
  if (servoAttached) return;
  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servoAttached = true;
  delay(20);
  Serial.println("[servo] attached");
}

void detachServoIfIdle() {
  if (!servoAttached || pressBusy) return;
  if (millis() - lastServoActivityMs < SERVO_IDLE_DETACH_MS) return;
  gateServo.detach();
  servoAttached = false;
  pinMode(SERVO_PIN, INPUT);
  if (angleDirty) {
    configStorageSaveLastAngle(currentAngle);
    angleDirty = false;
  }
  Serial.println("[servo] detached (idle)");
}

void persistAngles() {
  configStorageSaveAngles(homeAngle, pressAngle);
  configStorageSaveLastAngle(currentAngle);
  settingsFromNvs = true;
  angleDirty = false;
}

void resetToFactoryDefaults() {
  configStorageClear();
  homeAngle = SERVO_HOME_ANGLE;
  pressAngle = SERVO_PRESS_ANGLE;
  currentAngle = SERVO_HOME_ANGLE;
  settingsFromNvs = false;
  angleDirty = false;
  Serial.printf("[nvs] factory home=%d press=%d\n", homeAngle, pressAngle);
}

void moveServo(int angle) {
  angle = constrain(angle, 0, 180);
  attachServoIfNeeded();
  currentAngle = angle;
  lastServoActivityMs = millis();
  angleDirty = true;
  gateServo.write(angle);
  Serial.printf("[servo] angle=%d\n", angle);
}

void startPress() {
  if (pressBusy) return;
  pressBusy = true;
  pressState = PRESS_DOWN;
  pressStateMs = millis();
  Serial.println("[servo] PRESS sequence start");
  moveServo(pressAngle);
}

void servicePress() {
  if (!pressBusy) return;
  unsigned long now = millis();

  switch (pressState) {
    case PRESS_DOWN:
      pressState = PRESS_HOLD;
      pressStateMs = now;
      break;
    case PRESS_HOLD:
      if (now - pressStateMs >= PRESS_HOLD_MS) {
        pressState = PRESS_UP;
        moveServo(homeAngle);
        pressStateMs = now;
      }
      break;
    case PRESS_UP:
      if (now - pressStateMs >= 80) {
        pressState = PRESS_IDLE;
        pressBusy = false;
        lastServoActivityMs = millis();
        configStorageSaveLastAngle(currentAngle);
        angleDirty = false;
        Serial.println("[servo] PRESS sequence done");
      }
      break;
    default:
      pressBusy = false;
      pressState = PRESS_IDLE;
      break;
  }
}

void handleRoot() {
  server.send_P(200, "text/html", PIN_HTML);
}

void handleAdminPage() {
  if (authIsLoggedIn(server)) {
    server.send_P(200, "text/html", ADMIN_HTML);
  } else {
    server.send_P(200, "text/html", ADMIN_LOGIN_HTML);
  }
}

void handleNotFound() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
}

void setupWebServer() {
  const char* collect[] = {"Cookie"};
  server.collectHeaders(collect, 1);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/admin", HTTP_GET, handleAdminPage);
  setupApiRoutes(server);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[http] / = PIN unlock · /admin = settings");
}

void setupAccessPoint() {
  WiFi.persistent(false);
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

  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[wifi] SoftAP %s  ssid=%s  pass=%s  ch=%d\n",
                ok ? "OK" : "FAIL", AP_SSID, AP_PASSWORD, AP_CHANNEL);
  Serial.printf("[wifi] UI  http://%s\n", ip.toString().c_str());
  Serial.printf("[wifi] API http://%s/api/v1\n", ip.toString().c_str());
}

void printStatus() {
  Serial.println("---------- GateBot ----------");
  Serial.printf("  AP SSID    : %s\n", AP_SSID);
  Serial.printf("  AP IP      : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("  clients    : %d\n", WiFi.softAPgetStationNum());
  Serial.printf("  home angle : %d\n", homeAngle);
  Serial.printf("  press angle: %d\n", pressAngle);
  Serial.printf("  last angle : %d\n", currentAngle);
  Serial.printf("  settings   : %s\n", settingsFromNvs ? "saved (NVS)" : "factory");
  Serial.printf("  servo      : %s\n", servoAttached ? "attached" : "detached");
  Serial.printf("  free heap  : %u bytes\n", ESP.getFreeHeap());
  Serial.println("--------------------------------");
}

void printHelp() {
  Serial.println();
  Serial.println("Serial: h | p | s | home=N | press=N | status | resetcfg | help");
  Serial.println("Public: http://192.168.4.1/          (PIN unlock)");
  Serial.println("Admin:  http://192.168.4.1/admin     (settings + PINs)");
  Serial.println();
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  if (cmd == "h" || cmd == "home") {
    moveServo(homeAngle);
  } else if (cmd == "p" || cmd == "press") {
    startPress();
  } else if (cmd == "s" || cmd == "sweep") {
    sweepMode = !sweepMode;
    lastSweepMs = millis();
    Serial.printf("[mode] sweep=%s\n", sweepMode ? "ON" : "OFF");
  } else if (cmd.startsWith("home=")) {
    homeAngle = constrain(cmd.substring(5).toInt(), 0, 180);
    Serial.printf("[cfg] homeAngle=%d\n", homeAngle);
    moveServo(homeAngle);
    persistAngles();
  } else if (cmd.startsWith("press=")) {
    pressAngle = constrain(cmd.substring(6).toInt(), 0, 180);
    Serial.printf("[cfg] pressAngle=%d\n", pressAngle);
    persistAngles();
  } else if (cmd == "resetcfg") {
    resetToFactoryDefaults();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "help" || cmd == "?") {
    printHelp();
  } else {
    Serial.printf("[?] unknown: '%s'  (type help)\n", cmd.c_str());
  }
}

void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length() > 0) {
        handleCommand(serialBuf);
        serialBuf = "";
      }
    } else {
      serialBuf += c;
      if (serialBuf.length() > 64) serialBuf = "";
    }
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(SERVO_PIN, INPUT);

  configStorageBegin();
  pinStorageBegin();
  authBegin();
  GateBotSettings saved = configStorageLoad();
  homeAngle = saved.homeAngle;
  pressAngle = saved.pressAngle;
  currentAngle = saved.lastAngle;
  settingsFromNvs = saved.fromNvs;

  Serial.println();
  Serial.println("====================================");
  Serial.println("  GateBot — PIN unlock + admin");
  Serial.println("====================================");

  setupAccessPoint();
  setupWebServer();

  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(80);
    digitalWrite(LED_PIN, LOW);
    delay(80);
  }

  printStatus();
  printHelp();
  Serial.printf("[boot] pins=%d  admin=/admin\n", pinStorageCount());
  Serial.println("[boot] ready");
}

void loop() {
  server.handleClient();
  servicePress();
  detachServoIfIdle();
  blinkLed();
  pollSerial();

  if (sweepMode && !pressBusy) {
    unsigned long now = millis();
    if (now - lastSweepMs >= SWEEP_PAUSE_MS) {
      lastSweepMs = now;
      startPress();
    }
  }
}
