#pragma once
#include "Mode.h"
#include "Globals.h"

// Game: React
// Wait for screen to flash, then tilt/press button as fast as possible.
// Measure reaction time.

class ModeReact : public Mode {
    unsigned long waitStart;
    unsigned long flashTime;
    unsigned long reactTime;
    int state = 0; // 0=Wait, 1=Ready(Random delay), 2=Flash, 3=Result
    int score = 0;

public:
    const char* getName() override { return "Reaction Test"; }

    void setup() override {
        state = 0;
    }

    void loop() override {
        canvas.fillScreen(0);
        
        // Wait State (0: Idle)
        if (state == 0) {
            // Blink centered dot
            if ((millis()/500)%2 == 0) canvas.drawPixel(4, 7, 1);
        }
        else if (state == 1) { // Waiting for Random Flash
            // Hidden Timer Check
            if (millis() > flashTime) {
                state = 2; // Trigger Flash
                flashTime = millis(); 
            }
        }
        else if (state == 2) { // FLASH!
            canvas.fillScreen(1);
        }
        else if (state == 3) { // Result
            // Draw Score Bar graph
            // 1 pixel = 10ms. 
            // 200ms = 20 px (2 rows) - Fast
            // 500ms = 50 px (5 rows) - Slow
            
            int pixels = score / 10; 
            if (pixels > 160) pixels = 160;
            
            // Fill from bottom up
            for(int i=0; i<pixels; i++) {
                int x = i % 10;
                int y = 15 - (i / 10);
                if (y >= 0) canvas.drawPixel(x, y, 1);
            }
        }
        
        delay(10);
    }
    
    // Fix: declare handleButton inside public or fix order
    // Order was correct but I am replacing loop/handleButton block.
    
    bool handleButton() override {
        if (state == 0) {
            // Start Game
            state = 1; 
            flashTime = millis() + random(2000, 6000);
            return true;
        } else if (state == 1) {
            // Too Early! (False start)
            state = 3;
            score = 1000; // Max penalty
            return true;
        } else if (state == 2) {
            // Correct Reaction!
            reactTime = millis() - flashTime;
            state = 3; 
            score = (int)reactTime;
            return true;
        } else if (state == 3) {
            // Reset to allow retry
            state = 0;
            return true;
        }
        return false;
    }
};
