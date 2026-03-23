#pragma once
#include "Mode.h"
#include "Globals.h"

struct Firework {
    float x, y;
    float vx, vy;
    bool exploded;
    int life;
};

#define MAX_PARTICLES 40

class ModeFireworks : public Mode {
    Firework particles[MAX_PARTICLES];

public:
    const char* getName() override { return "Fireworks"; }

    void setup() override {
        for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    }

    void loop() override {
        clearDisplay();

        // 1. Create new firework occasionally
        if (random(0, 30) == 0) {
            // Find slot for rocket
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (particles[i].life <= 0) {
                    particles[i].x = random(2, MATRIX_WIDTH - 2);
                    particles[i].y = MATRIX_HEIGHT - 1;
                    particles[i].vy = -random(30, 45) / 10.0; // Shoot up
                    particles[i].vx = random(-10, 10) / 20.0;
                    particles[i].exploded = false;
                    particles[i].life = 50; // Use life for tracking rocket phase
                    break;
                }
            }
        }

        // 2. Physics
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].life > 0) {
                // Rocket Phase
                if (!particles[i].exploded) {
                    particles[i].x += particles[i].vx;
                    particles[i].y += particles[i].vy;
                    particles[i].vy += 0.1; // Gravity (slower for rocket)

                    // Apex reached? Explode
                    if (particles[i].vy >= 0 || particles[i].y < random(2, 6)) {
                        explode(i);
                    }
                } 
                // Particle Phase
                else {
                    particles[i].x += particles[i].vx;
                    particles[i].y += particles[i].vy;
                    particles[i].vy += 0.2; // Gravity
                    particles[i].life--;
                }

                // Draw
                int px = (int)particles[i].x;
                int py = (int)particles[i].y;
                if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
                    canvas.drawPixel(px, py, 1);
                }
            }
        }
        
        delay(30);
    }

    void explode(int parentIndex) {
        float x = particles[parentIndex].x;
        float y = particles[parentIndex].y;

        // Kill parent
        particles[parentIndex].life = 0;

        // Spawn children
        int count = random(8, 15);
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (count <= 0) break;
            if (particles[i].life <= 0) {
                particles[i].x = x;
                particles[i].y = y;
                float angle = random(0, 360) * PI / 180.0;
                float speed = random(5, 15) / 10.0;
                particles[i].vx = cos(angle) * speed;
                particles[i].vy = sin(angle) * speed;
                particles[i].exploded = true; // Is a particle
                particles[i].life = random(10, 25);
                count--;
            }
        }
    }
};
