#pragma once
#include "Mode.h"
#include "Globals.h"

struct Star {
    float x, y, z;
};

#define STARS_COUNT 20

class ModeStarfield : public Mode {
    Star stars[STARS_COUNT];

public:
    const char* getName() override { return "Starfield"; }

    void setup() override {
        for (int i = 0; i < STARS_COUNT; i++) {
            resetStar(i);
        }
    }
    
    void resetStar(int i) {
        stars[i].x = random(-50, 50);
        stars[i].y = random(-50, 50);
        stars[i].z = random(10, 30); // Start far
    }

    void loop() override {
        clearDisplay();

        for (int i = 0; i < STARS_COUNT; i++) {
            // Move closer
            stars[i].z -= 0.5;

            if (stars[i].z <= 1) resetStar(i);

            // Project 3D to 2D
            int sx = (stars[i].x / stars[i].z) + (MATRIX_WIDTH / 2);
            int sy = (stars[i].y / stars[i].z) + (MATRIX_HEIGHT / 2);

            if (sx >= 0 && sx < MATRIX_WIDTH && sy >= 0 && sy < MATRIX_HEIGHT) {
                canvas.drawPixel(sx, sy, 1);
            } else {
                 if (stars[i].z < 10) resetStar(i); // Off screen? Reset
            }
        }
        
        delay(30);
    }
};
