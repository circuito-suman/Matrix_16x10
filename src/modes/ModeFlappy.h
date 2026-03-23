#pragma once
#include "Mode.h"
#include "Globals.h"

#define BIRD_X 2
#define PIPE_GAP 4
#define PIPE_WIDTH 2
#define GRAVITY 0.3
#define JUMP_FORCE -1.2
#define PIPE_SPEED 0.3

struct Pipe {
    float x;
    int gapY;
    bool visible;
};

class ModeFlappy : public Mode {
    float birdY;
    float velocityY;
    Pipe pipes[3]; // Max 3 pipes on screen
    int score = 0;
    bool gameOver = false;
    unsigned long lastUpdate = 0;

public:
    const char* getName() override { return "Flappy Bird"; }

    void setup() override {
        resetGame();
    }

    // Button makes bird jump
    bool handleButton() override {
        if (gameOver) {
            resetGame();
        } else {
            velocityY = JUMP_FORCE;
        }
        return true; // Always consume button in this mode
    }

    void resetGame() {
        birdY = MATRIX_HEIGHT / 2.0;
        velocityY = 0;
        score = 0;
        gameOver = false;
        
        // Init pipes off screen
        for (int i = 0; i < 3; i++) {
            pipes[i].x = MATRIX_WIDTH + (i * 8); // Spacing
            pipes[i].gapY = random(2, MATRIX_HEIGHT - PIPE_GAP - 2);
            pipes[i].visible = true;
        }
    }

    void loop() override {
        // Limit Frame Rate for physics consistency
        if (millis() - lastUpdate < 30) return;
        lastUpdate = millis();

        clearDisplay();

        if (gameOver) {
            // Blink Score or Game Over
            if ((millis() / 500) % 2 == 0) {
                 // Draw Skull or X
                 canvas.drawLine(2, 4, 8, 10, 1);
                 canvas.drawLine(8, 4, 2, 10, 1);
            }
            return;
        }

        // 1. Physics
        velocityY += GRAVITY;
        birdY += velocityY;

        // 2. Pipe Movement
        for (int i = 0; i < 3; i++) {
            pipes[i].x -= PIPE_SPEED;
            if (pipes[i].x < -PIPE_WIDTH) {
                pipes[i].x = MATRIX_WIDTH + 4; // Wrap around
                pipes[i].gapY = random(2, MATRIX_HEIGHT - PIPE_GAP - 2);
                score++;
            }
        }

        // 3. Collision
        // Floor/Ceiling
        if (birdY < 0 || birdY >= MATRIX_HEIGHT) {
            gameOver = true;
        }

        // Pipes
        for (int i = 0; i < 3; i++) {
            if (pipes[i].x < BIRD_X + 1 && pipes[i].x + PIPE_WIDTH > BIRD_X) {
                // Should use strict checking, but simple AABB is mostly fine
                // Bird is 1 pixel at (BIRD_X, (int)birdY)
                if ((int)birdY < pipes[i].gapY || (int)birdY >= pipes[i].gapY + PIPE_GAP) {
                     gameOver = true;
                }
            }
        }

        // 4. Draw
        // Bird
        canvas.drawPixel(BIRD_X, (int)birdY, 1);

        // Pipes
        for (int i = 0; i < 3; i++) {
            int px = (int)pipes[i].x;
            if (px < MATRIX_WIDTH && px + PIPE_WIDTH > 0) {
                 // Top Pipe
                 canvas.fillRect(px, 0, PIPE_WIDTH, pipes[i].gapY, 1);
                 // Bottom Pipe
                 canvas.fillRect(px, pipes[i].gapY + PIPE_GAP, PIPE_WIDTH, MATRIX_HEIGHT - (pipes[i].gapY + PIPE_GAP), 1);
            }
        }
    }
};
