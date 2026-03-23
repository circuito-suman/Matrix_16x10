#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeSpiritLevel : public Mode {
public:
    const char* getName() override { return "Spirit Level"; }

    void setup() override {}

    void loop() override {
        clearDisplay();

        // 1. Center Circle
        canvas.drawCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, 4, 1);
        canvas.drawPixel(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, 1); // Center dot

        // 2. Bubble Physics
        // accX, accY are tilted.
        // If tilted right (positive X), bubble goes left.
        // Sensitivity factor
        float x = (MATRIX_WIDTH/2.0) - (accX / 500.0);
        float y = (MATRIX_HEIGHT/2.0) + (accY / 500.0); // Y might need invert check

        // Clamp to screen
        if (x < 1) x = 1; if (x > MATRIX_WIDTH - 2) x = MATRIX_WIDTH - 2;
        if (y < 1) y = 1; if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;

        // Draw Bubble (2x2 or 3x3)
        // Just a bright pixel or cross
        canvas.drawPixel((int)x, (int)y, 1);
        canvas.drawPixel((int)x+1, (int)y, 1);
        canvas.drawPixel((int)x, (int)y+1, 1);
        canvas.drawPixel((int)x+1, (int)y+1, 1); // 2x2 block

        delay(20);
    }
};
