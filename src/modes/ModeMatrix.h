#pragma once
#include "Mode.h"
#include "Globals.h"

enum MatrixDir { RAIN_DOWN, RAIN_UP, RAIN_RIGHT, RAIN_LEFT };

class ModeMatrix : public Mode {
    unsigned long lastUpdate = 0;    // Physics timer
    unsigned long tiltStartTime = 0; // Direction hold timer
    
    MatrixDir currentDir = RAIN_DOWN;
    MatrixDir pendingDir = RAIN_DOWN;

public:
    const char* getName() override { return "Water Matrix 4-Way"; }

    void setup() override { 
        clearDisplay(); 
        lastUpdate = millis();
        tiltStartTime = millis();
    }
    
    void loop() override {
        handleInput();

        // Speed Control (Run every 60ms)
        if (millis() - lastUpdate < 60) return;
        lastUpdate = millis();

        // Update based on current direction
        switch (currentDir) {
            case RAIN_DOWN:  updateDown();  break;
            case RAIN_UP:    updateUp();    break;
            case RAIN_RIGHT: updateLeft(); break;
            case RAIN_LEFT:  updateRight();  break;
        }
    }

private:
    // --- 1. INPUT LOGIC (Same as Scroll) ---
    void handleInput() {
        MatrixDir detectedDir = currentDir;

        // Detect Tilt (Threshold 3000)
        // Adjust signs if your sensor is mounted differently
        if (accX > 3000)      detectedDir = RAIN_LEFT;  // Tilt Left
        else if (accX < -3000) detectedDir = RAIN_RIGHT; // Tilt Right
        else if (accY < -3000) detectedDir = RAIN_UP;    // Tilt Forward
        else if (accY > 3000)  detectedDir = RAIN_DOWN;  // Tilt Back

        if (detectedDir != currentDir) {
            if (detectedDir == pendingDir) {
                if (millis() - tiltStartTime > 2000) {
                    currentDir = detectedDir;
                    clearDisplay(); // Clear screen on change
                }
            } else {
                pendingDir = detectedDir;
                tiltStartTime = millis();
            }
        } else {
            pendingDir = currentDir;
            tiltStartTime = millis(); 
        }
    }

    // --- 2. PHYSICS HELPERS ---

    void updateDown() {
        // Shift Everything Down (Start from bottom row up)
        for (int y = 15; y > 0; y--) {
            for(int x = 0; x < 10; x++) setPixel(x, y, getPixel(x, y-1));
        }
        // Clear Top
        for(int x = 0; x < 10; x++) setPixel(x, 0, 0);
        // Spawn New Drops at Top
        if(random(10) > 6) setPixel(random(10), 0, 1);
    }

    void updateUp() {
        // Shift Everything Up (Start from top row down)
        for (int y = 0; y < 15; y++) {
            for(int x = 0; x < 10; x++) setPixel(x, y, getPixel(x, y+1));
        }
        // Clear Bottom
        for(int x = 0; x < 10; x++) setPixel(x, 15, 0);
        // Spawn New Drops at Bottom
        if(random(10) > 6) setPixel(random(10), 15, 1);
    }

    void updateRight() {
        // Shift Everything Right (Start from right col left)
        for (int x = 9; x > 0; x--) {
            for(int y = 0; y < 16; y++) setPixel(x, y, getPixel(x-1, y));
        }
        // Clear Left Edge
        for(int y = 0; y < 16; y++) setPixel(0, y, 0);
        // Spawn New Drops at Left
        if(random(10) > 6) setPixel(0, random(16), 1);
    }

    void updateLeft() {
        // Shift Everything Left (Start from left col right)
        for (int x = 0; x < 9; x++) {
            for(int y = 0; y < 16; y++) setPixel(x, y, getPixel(x+1, y));
        }
        // Clear Right Edge
        for(int y = 0; y < 16; y++) setPixel(9, y, 0);
        // Spawn New Drops at Right
        if(random(10) > 6) setPixel(9, random(16), 1);
    }
};