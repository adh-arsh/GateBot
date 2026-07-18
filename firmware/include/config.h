#pragma once

// --- Pins ---
#define SERVO_PIN 18
#define LED_PIN 2  // onboard LED on most ESP32 DevKit boards

// --- SoftAP (phone/laptop connects to the ESP32) ---
#define AP_SSID "GateBot"
#define AP_PASSWORD "Star@918"
#define AP_CHANNEL 6
#define AP_MAX_CLIENTS 4

// --- Servo travel factory defaults (used only if NVS has no saved state) ---
// Tuned values from the web UI are stored in flash and survive power-off.
#define SERVO_HOME_ANGLE 40
#define SERVO_PRESS_ANGLE 170

// How long to hold the press before returning home (ms)
#define PRESS_HOLD_MS 400

// Detach servo after idle to cut holding current (stops brownouts / buzzing)
#define SERVO_IDLE_DETACH_MS 600

// How long to wait between automatic test cycles in SWEEP mode (ms)
#define SWEEP_PAUSE_MS 3000

// PWM channel range for SG90
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400
