#pragma once
#include "Mode.h"
#include "Globals.h"

class PhysicsMode : public Mode {
    float x = 5, y = 0, vy = 0;
public:
    void setup() override {
        x = 5; y = 0; vy = 0;
    }
    
    void loop() override {
        // Simple Bounce Physics
        vy += 0.2; // Gravity
        y += vy;
        if (y > 15) { y = 15; vy *= -0.8; }

        canvas.fillScreen(0);
        canvas.fillCircle((int)x, (int)y, 2, 1);
    }

    const char* getName() override { return "Physics Ball"; }
};