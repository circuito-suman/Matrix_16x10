#pragma once
#include "Mode.h"
#include "Globals.h"

#define DINO_X 2
#define GROUND_Y MATRIX_HEIGHT - 2
#define DINO_JUMP_FORCE -1.5
#define DINO_GRAVITY 0.25
#define OBSTACLE_SPEED 0.4

struct Obstacle {
    float x;
    int height;
    bool active;
};

class ModeDino : public Mode {
    float dinoY;
    float velocityY;
    Obstacle obstacles[2];
    bool gameRunning = false;
    int score = 0;
    unsigned long lastSpawn = 0;

public:
    const char* getName() override { return "Dino Run"; }

    void setup() override {
        gameRunning = false;
        resetGame();
    }

    bool handleButton() override {
        if (!gameRunning) {
            gameRunning = true;
            resetGame();
        } else {
            // Jump if on ground
            if (dinoY >= GROUND_Y) {
                velocityY = DINO_JUMP_FORCE;
            }
        }
        return true;
    }

    void resetGame() {
        dinoY = GROUND_Y;
        velocityY = 0;
        score = 0;
        
        for (int i = 0; i < 2; i++) {
            obstacles[i].active = false;
            obstacles[i].x = MATRIX_WIDTH + 5;
        }
        lastSpawn = millis();
    }

    void loop() override {
        clearDisplay();

        if (!gameRunning) {
            // Blink "RUN"
            if ((millis() / 500) % 2) {
                canvas.drawChar(0, 4, 'R', 1, 0, 1); // Helper needed for font? 
                // Just simpler animation
                canvas.drawPixel(2, 4, 1); 
                canvas.drawPixel(2, 5, 1); 
                canvas.drawPixel(2, 6, 1); 
            }
             canvas.drawLine(0, GROUND_Y + 1, MATRIX_WIDTH, GROUND_Y + 1, 1);
            return;
        }

        // 1. Physics
        if (dinoY < GROUND_Y) {
            velocityY += DINO_GRAVITY;
            dinoY += velocityY;
        } else {
            dinoY = GROUND_Y;
            velocityY = 0;
        }

        // 2. Obstacles
        // Move
        for (int i = 0; i < 2; i++) {
            if (obstacles[i].active) {
                obstacles[i].x -= OBSTACLE_SPEED;
                if (obstacles[i].x < -2) {
                    obstacles[i].active = false;
                    score++;
                }
            }
        }

        // Spawn
        if (millis() - lastSpawn > random(1500, 3000)) {
            // Find inactive obstacle
            for (int i = 0; i < 2; i++) {
                if (!obstacles[i].active) {
                    obstacles[i].active = true;
                    obstacles[i].x = MATRIX_WIDTH + 2;
                    obstacles[i].height = random(1, 3); // 1 or 2 pixels tall
                    lastSpawn = millis();
                    break;
                }
            }
        }

        // 3. Collision
        for (int i = 0; i < 2; i++) {
            if (obstacles[i].active) {
                // Check X overlap
                if (obstacles[i].x < DINO_X + 1 && obstacles[i].x + 1 > DINO_X) {
                     // Check Y overlap
                     // Obstacle is from GROUND_Y up to height
                     if (dinoY >= GROUND_Y - obstacles[i].height + 1) {
                         gameRunning = false;
                     }
                }
            }
        }

        // 4. Draw
        // Ground
        canvas.drawLine(0, GROUND_Y + 1, MATRIX_WIDTH, GROUND_Y + 1, 1);

        // Dino
        canvas.drawPixel(DINO_X, (int)dinoY, 1);
        // Maybe blink legs
        if ((millis() / 100) % 2 == 0 && dinoY == GROUND_Y) {
            canvas.drawPixel(DINO_X, (int)dinoY, 0); // Blink
        }

        // Obstacles
        for (int i = 0; i < 2; i++) {
            if (obstacles[i].active) {
                int ox = (int)obstacles[i].x;
                if (ox >= 0 && ox < MATRIX_WIDTH) {
                    // Draw cactus
                    canvas.drawLine(ox, GROUND_Y, ox, GROUND_Y - obstacles[i].height + 1, 1);
                }
            }
        }
    }
};
