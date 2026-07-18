#include <Preferences.h>
#include "config_storage.h"
#include "config.h"

static Preferences prefs;
static const char* NVS_NS = "gatebot";

void configStorageBegin() {
  prefs.begin(NVS_NS, false);
}

GateBotSettings configStorageLoad() {
  GateBotSettings s;
  s.homeAngle = SERVO_HOME_ANGLE;
  s.pressAngle = SERVO_PRESS_ANGLE;
  s.lastAngle = SERVO_HOME_ANGLE;
  s.pressHoldMs = PRESS_HOLD_MS;
  s.fromNvs = false;

  const bool hasHome = prefs.isKey("home");
  const bool hasPress = prefs.isKey("press");
  const bool hasLast = prefs.isKey("last");
  const bool hasHold = prefs.isKey("holdMs");

  if (hasHome || hasPress || hasLast || hasHold) {
    s.fromNvs = true;
    if (hasHome) s.homeAngle = constrain(prefs.getInt("home", SERVO_HOME_ANGLE), 0, 180);
    if (hasPress) s.pressAngle = constrain(prefs.getInt("press", SERVO_PRESS_ANGLE), 0, 180);
    if (hasLast) s.lastAngle = constrain(prefs.getInt("last", s.homeAngle), 0, 180);
    else s.lastAngle = s.homeAngle;
    if (hasHold) {
      s.pressHoldMs = constrain(prefs.getInt("holdMs", PRESS_HOLD_MS),
                                PRESS_HOLD_MS_MIN, PRESS_HOLD_MS_MAX);
    }
  }

  Serial.printf("[nvs] load home=%d press=%d last=%d holdMs=%d source=%s\n",
                s.homeAngle, s.pressAngle, s.lastAngle, s.pressHoldMs,
                s.fromNvs ? "flash" : "factory");
  return s;
}

void configStorageSaveAngles(int homeAngle, int pressAngle) {
  homeAngle = constrain(homeAngle, 0, 180);
  pressAngle = constrain(pressAngle, 0, 180);
  prefs.putInt("home", homeAngle);
  prefs.putInt("press", pressAngle);
  Serial.printf("[nvs] saved home=%d press=%d\n", homeAngle, pressAngle);
}

void configStorageSavePressHold(int pressHoldMs) {
  pressHoldMs = constrain(pressHoldMs, PRESS_HOLD_MS_MIN, PRESS_HOLD_MS_MAX);
  prefs.putInt("holdMs", pressHoldMs);
  Serial.printf("[nvs] saved holdMs=%d\n", pressHoldMs);
}

void configStorageSaveLastAngle(int angle) {
  angle = constrain(angle, 0, 180);
  // Skip write if unchanged to reduce NVS wear
  if (prefs.isKey("last") && prefs.getInt("last", -1) == angle) return;
  prefs.putInt("last", angle);
  Serial.printf("[nvs] saved last=%d\n", angle);
}

void configStorageSaveAll(int homeAngle, int pressAngle, int lastAngle, int pressHoldMs) {
  configStorageSaveAngles(homeAngle, pressAngle);
  configStorageSavePressHold(pressHoldMs);
  configStorageSaveLastAngle(lastAngle);
}

void configStorageClear() {
  prefs.clear();
  Serial.println("[nvs] cleared — factory defaults on next load");
}
