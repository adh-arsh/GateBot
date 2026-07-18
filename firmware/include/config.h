#pragma once

// --- Pins ---
#define SERVO_PIN 18
#define LED_PIN 2  // onboard LED on most ESP32 DevKit boards

// --- SoftAP (phone/laptop connects to the ESP32) ---
#define AP_SSID "GateBot"
#define AP_PASSWORD "Star@918"
#define AP_CHANNEL 6
#define AP_MAX_CLIENTS 4

// --- Admin panel (/admin) ---
#define ADMIN_EMAIL "r.adharsh@kalpataruprojects.com"
#define ADMIN_PASSWORD "Star@918"
#define SESSION_COOKIE_NAME "gatebot_session"
#define SESSION_TOKEN_LEN 32

// --- Public PIN unlock ---
#define PIN_LENGTH 6
#define PIN_MAX_COUNT 10
#define PIN_LABEL_MAX 24
#define PIN_FAIL_LIMIT 5
#define PIN_LOCKOUT_MS 30000

// --- Servo travel factory defaults (used only if NVS has no saved state) ---
#define SERVO_HOME_ANGLE 40
#define SERVO_PRESS_ANGLE 170

#define PRESS_HOLD_MS 400
#define SERVO_IDLE_DETACH_MS 600
#define SWEEP_PAUSE_MS 3000

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

// --- Wi-Fi dual mode (AP vs STA client) ---
#define WIFI_STA_TIMEOUT_MS 15000
#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64
