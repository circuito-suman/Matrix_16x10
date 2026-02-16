// src/Globals.h
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <MPU6050.h>

// Display Settings
#define MATRIX_WIDTH 10
#define MATRIX_HEIGHT 16


// --- Global Objects (Defined in main.cpp) ---
extern GFXcanvas1 canvas;  // The drawing surface
extern MPU6050 mpu;        // The sensor
extern float accX;         // Filtered X Acceleration
extern float accY;         // Filtered Y Acceleration
extern int16_t offsetX, offsetY; //Calibration Offsets

// --- Helper Functions for Modes ---
inline void setPixel(int r, int c, bool on) {
    if (r >= 0 && r < MATRIX_WIDTH && c >= 0 && c < MATRIX_HEIGHT) {
        canvas.drawPixel(r, c, on ? 1 : 0);
    }
}

inline bool getPixel(int r, int c) {
    if (r < 0 || r >= MATRIX_WIDTH || c < 0 || c >= MATRIX_HEIGHT) return false;
    return canvas.getPixel(r, c);
}

inline void clearDisplay() {
    canvas.fillScreen(0);
}


// Shared Canvas (The "Screen" in memory)

// Mutex to prevent writing to canvas while it's being cleared/drawn
extern SemaphoreHandle_t dispMutex;

// Flag to trigger a mode switch
extern volatile bool modeChangeRequest;
extern volatile int requestedModeIndex;