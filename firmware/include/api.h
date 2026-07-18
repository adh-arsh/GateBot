#pragma once

#include <WebServer.h>

// Shared device state (owned by main.cpp)
extern int homeAngle;
extern int pressAngle;
extern int currentAngle;
extern bool pressBusy;
extern bool settingsFromNvs;

void moveServo(int angle);
void startPress();
void servicePress();
void persistAngles();
void resetToFactoryDefaults();

String deviceStatusJson();
void setupApiRoutes(WebServer& server);
