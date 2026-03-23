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

/**
 * BLE protocol used by Android app:
 * - CHAR_MODE_UUID   (Write, 1 byte): mode index [0..MODE_COUNT-1]
 * - CHAR_CANVAS_UUID (Write, 20 bytes): raw 1-bit framebuffer (10x16/8)
 * - CHAR_TEXT_UUID   (Write): UTF-8 text
 * - CHAR_VERSION_UUID(Read/Notify): semantic firmware version (e.g. v1.0.7)
 * - CHAR_OTA_UUID    (Write): protocol OTA / legacy OTA data chunks
 * - CHAR_CONTROL_UUID(Write): framed protocol commands or legacy text commands
 * - CHAR_ACK_UUID    (Notify/Indicate): protocol ACK/NACK responses
 *
 * ACK/NACK status codes:
 * 0x00 success
 * 0x01 bad frame/CRC
 * 0x02 invalid state
 * 0x03 bad offset
 * 0x04 flash write error
 * 0x05 finalize error
 */