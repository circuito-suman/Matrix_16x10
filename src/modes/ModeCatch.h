#pragma once
#include "Mode.h"
#include "Globals.h"

// Game where a dot appears, and you must tilt to move a cursor to catch it.
// Gets faster over time.

class ModeCatch : public Mode {
    float cursorX, cursorY;
    int targetX, targetY;
    int score = 0;
    unsigned long startTime;
    int timeLeft = 30; // 30 seconds game

public:
    const char* getName() override { return "Catch The Dot"; }

    void setup() override {
        cursorX = MATRIX_WIDTH / 2.0;
        cursorY = MATRIX_HEIGHT / 2.0;
        score = 0;
        spawnTarget();
        startTime = millis();
    }

    void spawnTarget() {
        targetX = random(0, MATRIX_WIDTH);
        targetY = random(0, MATRIX_HEIGHT);
    }

    void loop() override {
        clearDisplay();
        
        unsigned long elapsed = (millis() - startTime) / 1000;
        int remaining = timeLeft - elapsed;

        if (remaining <= 0) {
            // Game Over
            // Show Score blinking
            if ((millis()/500)%2) {
                // Draw total pixels = score
                for(int i=0; i<score; i++) {
                    canvas.drawPixel(i%10, i/10, 1);
                }
            }
            return;
        }

        // 1. Move Cursor
        // Add sensitivity
        // Assuming accX is +/- 16000 or similar
        float dx = (accX / 2000.0);
        float dy = -(accY / 2000.0); // Inver Y usually
        
        cursorX += dx;
        cursorY += dy;

        // Clamp
        if (cursorX < 0) cursorX = 0;
        if (cursorX >= MATRIX_WIDTH) cursorX = MATRIX_WIDTH - 0.1;
        if (cursorY < 0) cursorY = 0;
        if (cursorY >= MATRIX_HEIGHT) cursorY = MATRIX_HEIGHT - 0.1;

        // 2. Check Capture (Distance < 1.0)
        // Simple distance check
        float dist = sqrt(pow(cursorX - targetX, 2) + pow(cursorY - targetY, 2));
        if (dist < 1.5) {
            score++;
            // New Target
            targetX = random(1, MATRIX_WIDTH-1);
            targetY = random(1, MATRIX_HEIGHT-2); // Avoid bottom bar
            // Make louder beep? (Not implemented here)
        }

        // 3. Draw Target
        canvas.drawPixel(targetX, targetY, 1);
        
        // 4. Draw Cursor (Cross)
        int cx = (int)cursorX;
        int cy = (int)cursorY;
        canvas.drawPixel(cx, cy, 1);
        if(cx > 0) canvas.drawPixel(cx-1, cy, 1);
        if(cx < MATRIX_WIDTH-1) canvas.drawPixel(cx+1, cy, 1);
        if(cy > 0) canvas.drawPixel(cx, cy-1, 1);
        if(cy < MATRIX_HEIGHT-1) canvas.drawPixel(cx, cy+1, 1);

        // 5. Draw Time Bar
        int barWidth = map(remaining, 0, 30, 0, MATRIX_WIDTH);
        canvas.drawLine(0, MATRIX_HEIGHT-1, barWidth, MATRIX_HEIGHT-1, 1);
    }
};
