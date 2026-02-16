#pragma once
#include <Arduino.h>
// src/drivers/CommsManager.h
#pragma once
#include <Arduino.h>

/**
 * @brief Initializes WiFi AP, OTA, and BLE services.
 * This should be called once, typically from the Comms Task (Core 0).
 */
void setupComms();

/**
 * @brief Handles periodic communication tasks.
 * Currently handles OTA updates. BLE is interrupt-driven and runs automatically.
 * This should be called inside the Comms Task loop.
 */
void handleComms();