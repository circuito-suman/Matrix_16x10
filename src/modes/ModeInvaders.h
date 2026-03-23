#pragma once
#include "Mode.h"
#include "Globals.h"

// Invader Constants
#define PLAYER_Y MATRIX_HEIGHT - 1
#define ALIEN_ROWS 2
#define ALIEN_COLS 6
#define BULLET_SPEED 0.5

struct Bullet {
    float x, y;
    bool active;
};

struct Alien {
    int x, y;
    bool active;
};

class ModeInvaders : public Mode {
    float playerX;
    Bullet playerBullet;
    Alien aliens[ALIEN_ROWS][ALIEN_COLS];
    bool gameRunning = false;
    int direction = 1;
    unsigned long lastMove = 0;
    int moveInterval = 500; // ms

public:
    const char* getName() override { return "Invaders"; }

    void setup() override {
        resetGame();
    }

    bool handleButton() override {
        if (!gameRunning) {
            resetGame();
            gameRunning = true;
        } else {
            // Shoot
            if (!playerBullet.active) {
                playerBullet.x = playerX;
                playerBullet.y = PLAYER_Y - 1;
                playerBullet.active = true;
            }
        }
        return true;
    }

    void resetGame() {
        playerX = MATRIX_WIDTH / 2.0;
        playerBullet.active = false;
        gameRunning = false;
        direction = 1;
        moveInterval = 500;

        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                aliens[r][c].x = c * 2 + 1; // Spaced out
                aliens[r][c].y = r * 2 + 1;
                aliens[r][c].active = true;
            }
        }
    }

    void loop() override {
        clearDisplay();

        if (!gameRunning) {
            // Attract Screen or Game Over
             canvas.setCursor(0, 4);
             canvas.print("INV");
             return;
        }

        // 1. Player Movement (MPU)
        float tilt = accX / 3000.0; 
        playerX += tilt;
        if (playerX < 0) playerX = 0;
        if (playerX >= MATRIX_WIDTH) playerX = MATRIX_WIDTH - 1;

        // 2. Bullet Physics
        if (playerBullet.active) {
            playerBullet.y -= BULLET_SPEED;
            if (playerBullet.y < 0) playerBullet.active = false;

            // Collision with Aliens
            for (int r = 0; r < ALIEN_ROWS; r++) {
                for (int c = 0; c < ALIEN_COLS; c++) {
                    if (aliens[r][c].active) {
                        if (abs(playerBullet.x - aliens[r][c].x) < 1.0 && 
                            abs(playerBullet.y - aliens[r][c].y) < 1.0) {
                            aliens[r][c].active = false;
                            playerBullet.active = false;
                            // Increase speed
                            moveInterval -= 10;
                            if (moveInterval < 50) moveInterval = 50;
                        }
                    }
                }
            }
        }

        // 3. Alien Movement
        if (millis() - lastMove > moveInterval) {
            lastMove = millis();
            bool edgeHit = false;

            // Check edges
            for (int r = 0; r < ALIEN_ROWS; r++) {
                for (int c = 0; c < ALIEN_COLS; c++) {
                    if (!aliens[r][c].active) continue;
                    
                    if (direction == 1 && aliens[r][c].x >= MATRIX_WIDTH - 1) edgeHit = true;
                    if (direction == -1 && aliens[r][c].x <= 0) edgeHit = true;
                }
            }

            if (edgeHit) {
                direction *= -1;
                // Move down
                for (int r = 0; r < ALIEN_ROWS; r++) {
                    for (int c = 0; c < ALIEN_COLS; c++) {
                        aliens[r][c].y++;
                        if (aliens[r][c].y >= PLAYER_Y) {
                            gameRunning = false; // Game Over
                        }
                    }
                }
            } else {
                // Move sideways
                for (int r = 0; r < ALIEN_ROWS; r++) {
                    for (int c = 0; c < ALIEN_COLS; c++) {
                        aliens[r][c].x += direction;
                    }
                }
            }
        }

        // 4. Draw
        // Player
        canvas.drawPixel((int)playerX, PLAYER_Y, 1);
        
        // Bullet
        if (playerBullet.active) canvas.drawPixel((int)playerBullet.x, (int)playerBullet.y, 1);

        // Aliens
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (aliens[r][c].active) {
                    canvas.drawPixel(aliens[r][c].x, aliens[r][c].y, 1);
                }
            }
        }
    }
};
