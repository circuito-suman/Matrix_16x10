// src/Config.h
#pragma once

#define btn_pin 35
#define PIN_STCP 5
#define BUZZER_PIN 17

// Row pins (Example mapping)
//const int ROWS[10] = { 32, 33, 25, 26, 27, 14, 19, 13, 16, 4 };

// WiFi / OTA
#define WIFI_SSID "Matrix_AP"
#define WIFI_PASS "password123"

// BLE UUIDs (Generate your own if needed)
#define SERVICE_UUID        "ABCD"
#define CHAR_MODE_UUID      "1234"
#define CHAR_CANVAS_UUID    "5678"
#define CHAR_TEXT_UUID      "00009ABC-0000-1000-8000-00805F9B34FB"
#define CHAR_VERSION_UUID   "00009ABD-0000-1000-8000-00805F9B34FB"
#define CHAR_OTA_UUID       "00009ABE-0000-1000-8000-00805F9B34FB"
#define CHAR_CONTROL_UUID   "00009ABF-0000-1000-8000-00805F9B34FB"
#define CHAR_ACK_UUID       "00009AC0-0000-1000-8000-00805F9B34FB"

#define FW_SEMVER           "v1.0.7"