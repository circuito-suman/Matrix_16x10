// src/Config.h
#pragma once

#define btn_pin 35
#define PIN_STCP 5
#define BUZZER_PIN 17

// Row pins (Example mapping)
const int ROWS[10] = { 32, 33, 25, 26, 27, 14, 19, 13, 16, 4 };

// WiFi / OTA
#define WIFI_SSID "Matrix_AP"
#define WIFI_PASS "password123"

// BLE UUIDs (Generate your own if needed)
#define SERVICE_UUID        "ABCD"
#define CHAR_MODE_UUID      "1234"
#define CHAR_CANVAS_UUID    "5678"