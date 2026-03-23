#pragma once
#include "Mode.h"
#include "Globals.h"

// Breakout Constants
#define PADDLE_W 3
#define BRICKS_ROWS 3

class ModeBreakout : public Mode {
    float ballX, ballY;
    float velX, velY;
    float paddleX;
    bool bricks[BRICKS_ROWS][MATRIX_WIDTH];
    bool gameRunning = false;
    int score = 0;

public:
    const char* getName() override { return "Breakout"; }

    void setup() override {
        gameRunning = false;
        resetGame();
    }

    bool handleButton() override {
        if (!gameRunning) {
            gameRunning = true; // Launch ball
            velY = -0.3; // Launch upwards
            velX = (random(0, 20) - 10) / 20.0;
            return true;
        }
        return false; // If running, button does nothing (let mode switch happen? Or pause?)
        // Actually, let's keep it simple: button launches. If running, button switches mode.
    }

    void resetGame() {
        // Reset Ball
        ballX = MATRIX_WIDTH / 2.0;
        ballY = MATRIX_HEIGHT - 2;
        velX = 0;
        velY = 0;
        gameRunning = false;
        paddleX = (MATRIX_WIDTH - PADDLE_W) / 2.0;

        // Reset Bricks
        for (int r = 0; r < BRICKS_ROWS; r++) {
            for (int c = 0; c < MATRIX_WIDTH; c++) {
                bricks[r][c] = true;
            }
        }
    }

    void loop() override {
        clearDisplay();

        // 1. Paddle Movement (MPU)
        // MPU Tilt: accX controls paddle
        float tilt = accX / 5000.0; // Adjust sensitivity
        paddleX += tilt;
        
        // Clamp Paddle
        if (paddleX < 0) paddleX = 0;
        if (paddleX > MATRIX_WIDTH - PADDLE_W) paddleX = MATRIX_WIDTH - PADDLE_W;

        // 2. Ball Physics
        if (gameRunning) {
            ballX += velX;
            ballY += velY;

            // Wall Collisions
            if (ballX <= 0 || ballX >= MATRIX_WIDTH - 1) velX *= -1;
            if (ballY <= 0) velY *= -1; // Top wall bounce

            // Paddle Collision
            if (ballY >= MATRIX_HEIGHT - 2 &&
                ballX >= paddleX && ballX <= paddleX + PADDLE_W) {
                velY *= -1;
                // Add some English based on where it hit the paddle
                velX += (ballX - (paddleX + PADDLE_W/2.0)) * 0.1;
            }

            // Bottom Boundary (Game Over)
            if (ballY >= MATRIX_HEIGHT) {
                gameRunning = false;
                // Maybe flash screen?
                resetGame();
            }

            // Brick Collision
            int checkX = (int)ballX;
            int checkY = (int)ballY;
            if (checkY < BRICKS_ROWS && checkY >= 0) {
               if (bricks[checkY][checkX]) {
                   bricks[checkY][checkX] = false;
                   velY *= -1;
                   score++;
               }
            }
        }

        // 3. Draw
        // Bricks
        for (int r = 0; r < BRICKS_ROWS; r++) {
            for (int c = 0; c < MATRIX_WIDTH; c++) {
                if (bricks[r][c]) canvas.drawPixel(c, r, 1);
            }
        }

        // Paddle
        for (int i = 0; i < PADDLE_W; i++) {
            canvas.drawPixel((int)paddleX + i, MATRIX_HEIGHT - 1, 1);
        }

        // Ball
        canvas.drawPixel((int)ballX, (int)ballY, 1);
    }
};
