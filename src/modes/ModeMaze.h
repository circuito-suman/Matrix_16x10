#pragma once
#include "Mode.h"
#include "Globals.h"

// Simple Maze (10x16)
// We'll generate a maze or use a few fixed levels.
// Let's use 2-3 fixed levels for simplicity in flash.

// Level 1: Simple winding path
const uint8_t PROGMEM level1[16][10] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,1,0,1},
    {1,1,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,2,1}, // 2 is Exit
    {1,1,1,1,1,1,1,1,1,1}
};

class ModeMaze : public Mode {
    float ballX, ballY, velX, velY;
    int currentLevel = 0;
    bool finished = false;

public:
    const char* getName() override { return "Maze"; }

    void setup() override {
        resetLevel();
    }
    
    void resetLevel() {
        ballX = 1.5; ballY = 1.5;
        velX = 0; velY = 0;
        finished = false;
    }

    void loop() override {
        clearDisplay();

        if (finished) {
            // Flash "WIN" or just checkmark
            canvas.drawPixel(2, 4, 1);
            canvas.drawPixel(3, 5, 1);
            canvas.drawPixel(4, 6, 1);
            canvas.drawPixel(5, 5, 1);
            canvas.drawPixel(6, 4, 1);
            
            if (millis() % 2000 < 500) resetLevel();
            return;
        }

        // 1. Physics (Tilt)
        float accelX = accX / 1000.0;
        float accelY = accY / 1000.0;

        velX += accelX;
        velY += accelY;
        velX *= 0.8; // Damp
        velY *= 0.8;

        float nextX = ballX + velX;
        float nextY = ballY + velY;

        // 2. Collision
        // Check 4 corners of the ball (which is 1x1 pixel?)
        // Let's treat ball as a point for simplicity on low res
        int checkX = (int)nextX;
        int checkY = (int)nextY;

        if (checkX >= 0 && checkX < MATRIX_WIDTH && checkY >= 0 && checkY < MATRIX_HEIGHT) {
             uint8_t cell = pgm_read_byte(&(level1[checkY][checkX]));
             if (cell == 1) {
                 // Wall
                 velX = 0; velY = 0;
             } else if (cell == 2) {
                 // Exit
                 finished = true;
             } else {
                 ballX = nextX;
                 ballY = nextY;
             }
        } else {
            // Screen Edges
            if (nextX < 0 || nextX >= MATRIX_WIDTH) velX = 0;
            if (nextY < 0 || nextY >= MATRIX_HEIGHT) velY = 0;
            // Clamp
            if (ballX < 0) ballX = 0;
            if (ballX >= MATRIX_WIDTH) ballX = MATRIX_WIDTH - 0.1;
            if (ballY < 0) ballY = 0;
            if (ballY >= MATRIX_HEIGHT) ballY = MATRIX_HEIGHT - 0.1;
        }

        // Draw Maze
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                uint8_t cell = pgm_read_byte(&(level1[y][x]));
                if (cell == 1) canvas.drawPixel(x, y, 1);
                else if (cell == 2) { // Blink Exit
                    if ((millis()/200)%2) canvas.drawPixel(x, y, 1);
                }
            }
        }

        // Draw Ball
        canvas.drawPixel((int)ballX, (int)ballY, 1);
    }
};
