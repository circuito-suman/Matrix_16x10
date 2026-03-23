#ifndef MODE_LANDER_H
#define MODE_LANDER_H

#include "Mode.h"
#include "Globals.h"

class ModeLander : public Mode {
private:
    float posX, posY;
    float velX, velY;
    float gravity;
    float thrust;
    bool landed;
    bool crashed;
    bool surveying; // Preview mode before launch

    // Terrain: 10 columns
    uint8_t terrain[10];
    int padStart; // Index where pad starts
    int padWidth; // How wide the pad is
    
    // Fuel
    float fuel;

    void generateTerrain() {
        // Randomize terrain
        for(int i=0; i<10; i++) {
            terrain[i] = random(1, 6); // Height 1-5
        }
        
        // Create a landing pad (flat area)
        padWidth = random(2, 4); // 2 or 3 pixels wide
        padStart = random(1, 10 - padWidth);
        
        uint8_t padHeight = random(1, 4);
        for(int i=0; i<padWidth; i++) {
            terrain[padStart + i] = padHeight;
        }
    }

    void reset() {
        generateTerrain();
        posX = 5.0;
        posY = 0.0;
        velX = 0.0;
        velY = 0.0;
        gravity = 0.05; // Pulls down
        thrust = -0.15; // Pushes up (Stronger against gravity)
        landed = false;
        crashed = false;
        surveying = true;
        fuel = 100.0;
    }

public:
    const char* getName() override {
        return "Moon Lander";
    }

    void setup() override {
        reset();
    }

    bool handleButton() override {
        // Handling is done in loop for continuous thrust?
        // No, we use click events for actions.
        if (landed || crashed) {
            reset();
            return true;
        } else if (surveying) {
            surveying = false; // Start game
            return true;
        } else {
            // In game: Click = Thrust burst
            velY += thrust * 2.0; 
            fuel -= 5;
            return true; // We handled the button
        }
        return false;
    }

    void loop() override {
        canvas.fillScreen(0);

        // 1. Draw Terrain
        for (int x = 0; x < 10; x++) {
            int h = terrain[x];
            canvas.drawFastVLine(x, 16 - h, h, 1);
            
            // Highlight pad (blink if surveying)
            if (x >= padStart && x < padStart + padWidth) {
                 if (!surveying || (millis() / 200) % 2 == 0) {
                     canvas.drawPixel(x, 16 - h, 1);
                 }
            }
        }

        // 2. HUD / State Logic
        if (surveying) {
            // Draw "RDY" or arrow
            canvas.drawPixel(5, 2, 1);
            canvas.drawPixel(4, 3, 1);
            canvas.drawPixel(6, 3, 1);
            return; 
        }

        if (landed) {
            // Draw smile
            canvas.drawPixel(3, 3, 1);
            canvas.drawPixel(7, 3, 1);
            canvas.drawPixel(4, 6, 1);
            canvas.drawPixel(5, 6, 1);
            canvas.drawPixel(6, 6, 1);
            return;
        }

        if (crashed) {
            // Explosion visual
            int r = (millis() / 100) % 5;
            canvas.drawCircle((int)posX, (int)posY, r, 1);
            return;
        }

        // 3. Physics
        
        // Gravity
        velY += gravity;

        // Steering (Tilt X)
        // accX: -16000 to +16000 approx for 1G.
        // Calibrated accX is centered around 0.
        // Tilt left (negative X?) -> velX decreases.
        float tilt = accX / 4000.0; // Scaled
        velX += (tilt * -0.05); // Invert if needed based on orientation
        
        // Drag
        velX *= 0.95;
        
        // Apply Physics
        posX += velX;
        posY += velY;

        // 4. Collision Detection
        
        // Screen bounds
        if (posX < 0) { posX = 0; velX = -velX * 0.5; }
        if (posX > 9) { posX = 9; velX = -velX * 0.5; }
        
        // Ground Collision
        int boxX = (int)posX;
        int boxY = (int)posY;
        
        if (boxX >= 0 && boxX < 10) {
            int groundHeight = terrain[boxX];
            int groundY = 16 - groundHeight;
            
            if (posY >= groundY - 1) { // Hit ground
                // Check if on pad
                // Check center of lander vs pad range
                bool onPad = (boxX >= padStart && boxX < padStart + padWidth);
                
                // Check speed
                bool softLanding = (velY < 0.8) && (abs(velX) < 0.5);
                
                if (onPad && softLanding) {
                    landed = true;
                } else {
                    crashed = true;
                }
                
                // Stop movement
                velX = 0;
                velY = 0;
                posY = groundY - 1;
            }
        }
        
        // Draw Lander
        canvas.drawPixel((int)posX, (int)posY, 1);
        
        // Draw Exhaust particle
        if (velY < 0) { // Moving up/thrusting visual
             canvas.drawPixel((int)posX, (int)posY + 1, (millis()%2) ? 1:0);
        }
        
        delay(30); // Slow down game loop
    }
};

#endif
