#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeFluid : public Mode {
    unsigned long lastUpdate = 0;
public:
    const char* getName() override { return "360 Sand"; }
    
    void setup() override { 
        clearDisplay(); 
        lastUpdate = millis();
        // Seed some sand
        for(int i=0; i<30; i++) setPixel(random(10), random(16), 1);
    }

    void loop() override {
        // Add sand continuously
        if (random(100) > 90) setPixel(5, 8, 1); 

        if (millis() - lastUpdate < 30) return;
        lastUpdate = millis();

        // 1. Calculate Gravity Vector from MPU
        int dx = 0, dy = 0;
        
        // MPU Orientation Mapping (Adjust signs based on your specific mounting)
        // Assuming accX is Up/Down tilt, accY is Left/Right tilt
        if (abs(accX) > abs(accY)) {
            // Vertical Dominance
            if (accX > 0) dx = 1;  else dx = -1; // X axis controls 'Down' on matrix width?
            // Actually, usually X is long axis. Let's map standard ESP32:
            // Let's assume accY is Vertical on the matrix (0-15) and accX is Horizontal (0-9)
            // If that's wrong, swap the logic below.
        } 
        
        // Robust Direction Logic:
        // accY large positive -> Gravity Down (+Y)
        // accY large negative -> Gravity Up (-Y)
        // accX large positive -> Gravity Right (+X)
        // accX large negative -> Gravity Left (-X)
        
        if (abs(accY) > abs(accX)) {
             if (accY > 0) dy = 1; else dy = -1; // Vertical Gravity
        } else {
             if (accX > 0) dx = 1; else dx = -1; // Horizontal Gravity
        }

        // 2. Scan Direction (Must scan OPPOSITE to gravity to prevent teleporting)
        int startX = (dx == 1) ? 9 : 0;
        int endX   = (dx == 1) ? -1 : 10;
        int stepX  = (dx == 1) ? -1 : 1;

        int startY = (dy == 1) ? 15 : 0;
        int endY   = (dy == 1) ? -1 : 16;
        int stepY  = (dy == 1) ? -1 : 1;

        // 3. Physics Simulation
        for (int y = startY; y != endY; y += stepY) {
            for (int x = startX; x != endX; x += stepX) {
                
                if (getPixel(x, y)) { // If there is sand here
                    
                    int belowX = x + dx;
                    int belowY = y + dy;

                    // A. Check directly below (in gravity direction)
                    bool canMoveDown = (belowX >= 0 && belowX < 10 && belowY >= 0 && belowY < 16);
                    if (canMoveDown && !getPixel(belowX, belowY)) {
                        setPixel(x, y, 0);
                        setPixel(belowX, belowY, 1);
                        continue;
                    }

                    // B. Check diagonals (Slides)
                    // If gravity is Vertical (dy!=0), check X sides
                    if (dy != 0) {
                        int rX = x + 1; int lX = x - 1;
                        // Try Right Diagonal
                        if (rX < 10 && !getPixel(rX, belowY) && canMoveDown) {
                             setPixel(x, y, 0); setPixel(rX, belowY, 1);
                        }
                        // Try Left Diagonal
                        else if (lX >= 0 && !getPixel(lX, belowY) && canMoveDown) {
                             setPixel(x, y, 0); setPixel(lX, belowY, 1);
                        }
                    }
                    // If gravity is Horizontal (dx!=0), check Y sides
                    else if (dx != 0) {
                        int rY = y + 1; int lY = y - 1;
                         if (rY < 16 && !getPixel(belowX, rY) && canMoveDown) {
                             setPixel(x, y, 0); setPixel(belowX, rY, 1);
                        }
                        else if (lY >= 0 && !getPixel(belowX, lY) && canMoveDown) {
                             setPixel(x, y, 0); setPixel(belowX, lY, 1);
                        }
                    }
                }
            }
        }
    }
};