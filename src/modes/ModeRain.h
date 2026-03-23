#pragma once
#include "Mode.h"
#include "Globals.h"

struct Drop {
    float x, y;
    float speed;
    bool active;
    bool splash; // Is splashing
    int splashFrame;
};

#define MAX_DROPS 15

class ModeRain : public Mode {
    Drop drops[MAX_DROPS];

public:
    const char* getName() override { return "Rain"; }

    void setup() override {
        for(int i=0; i<MAX_DROPS; i++) {
            drops[i].active = false;
        }
    }

    void loop() override {
        clearDisplay();

        // 1. Spawn logic
        if (random(0, 100) < 30) { // Increased spawn rate
             // Find slot
             for(int i=0; i<MAX_DROPS; i++) {
                 if (!drops[i].active) {
                     drops[i].x = random(0, MATRIX_WIDTH);
                     drops[i].y = 0;
                     drops[i].speed = random(5, 12) / 10.0;
                     drops[i].active = true;
                     drops[i].splash = false;
                     break;
                 }
             }
        }

        // 2. Physics & Logic
        for(int i=0; i<MAX_DROPS; i++) {
             if (drops[i].active) {
                 if (!drops[i].splash) {
                     // Falling
                     drops[i].y += drops[i].speed;
                     if (drops[i].y >= MATRIX_HEIGHT - 1) {
                         drops[i].y = MATRIX_HEIGHT - 1;
                         drops[i].splash = true;
                         drops[i].splashFrame = 0;
                     }
                 } else {
                     // Splashing animation
                     drops[i].splashFrame++;
                     if (drops[i].splashFrame > 2) { // 3 frames of splash
                         drops[i].active = false;
                     }
                 }
             }
        }

        // 3. Draw
        for(int i=0; i<MAX_DROPS; i++) {
             if (!drops[i].active) continue;

             int x = (int)drops[i].x;
             int y = (int)drops[i].y;

             if (!drops[i].splash) {
                 // Draw Drop
                 if (y < MATRIX_HEIGHT) canvas.drawPixel(x, y, 1);
                 // Trail (makes it look faster)
                 if (y > 0) canvas.drawPixel(x, y-1, 1);
             } else {
                 // Splash Animation
                 // Frame 0: Hit ground (solid pixel)
                 // Frame 1: Spread out low
                 // Frame 2: Spread out high
                 
                 if (drops[i].splashFrame == 0) {
                     canvas.drawPixel(x, MATRIX_HEIGHT-1, 1);
                 } else if (drops[i].splashFrame == 1) {
                     if (x > 0) canvas.drawPixel(x-1, MATRIX_HEIGHT-1, 1);
                     if (x < MATRIX_WIDTH-1) canvas.drawPixel(x+1, MATRIX_HEIGHT-1, 1);
                 } else if (drops[i].splashFrame == 2) {
                     if (x > 0) canvas.drawPixel(x-1, MATRIX_HEIGHT-2, 1);
                     if (x < MATRIX_WIDTH-1) canvas.drawPixel(x+1, MATRIX_HEIGHT-2, 1);
                 }
             }
        }
        
        delay(40);
    }
};
