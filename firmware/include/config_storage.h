#pragma once

#include <Arduino.h>

// NVS-backed settings. Survives power loss.
// Compile-time values in config.h are used only when nothing is stored yet.

struct GateBotSettings {
  int homeAngle;
  int pressAngle;
  int lastAngle;
  bool fromNvs;  // true if loaded from flash (not factory defaults)
};

void configStorageBegin();
GateBotSettings configStorageLoad();
void configStorageSaveAngles(int homeAngle, int pressAngle);
void configStorageSaveLastAngle(int angle);
void configStorageSaveAll(int homeAngle, int pressAngle, int lastAngle);
void configStorageClear();  // wipe NVS → next boot uses factory defaults
