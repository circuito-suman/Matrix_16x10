#pragma once
#include "Mode.h"
#include "Globals.h"

// Simple Plasma effect
class ModePlasma : public Mode {
    float time = 0;

public:
    const char* getName() override { return "Plasma"; }

    void setup() override {
        time = 0;
    }

    void loop() override {
        clearDisplay();
        
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x++) {
                // Calculate plasma value
                float v1 = sin(x * 0.5 + time);
                float v2 = sin(y * 0.5 + time);
                float v3 = sin((x + y) * 0.5 + time);
                float v4 = sin(sqrt((x * x) + (y * y)) * 0.2 + time);
                
                float v = (v1 + v2 + v3 + v4);
                
                // For monochrome, we threshold
                // v ranges roughly -4 to 4
                if (v > 1.0) {
                    canvas.drawPixel(x, y, 1);
                }
            }
        }
        
        time += 0.1;
        delay(30);
    }
};
