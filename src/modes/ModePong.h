#pragma once
#include "Mode.h"
#include "Globals.h"
class ModePong : public Mode {
    float bx, by, bvx, bvy, aiPos, playPos;
    unsigned long lastUpdate = 0;
public:
    const char* getName() override { return "Pong"; }
    
    void setup() override {
        bx = 7.5; by = 4.5; bvx = 0.25; bvy = 0.15; // Slower start speed
        aiPos = 3; playPos = 3;
        lastUpdate = millis();
    }
    
    void loop() override {
        // LOCK SPEED: Update game every 30ms (33 FPS)
        if (millis() - lastUpdate < 30) return;
        lastUpdate = millis();

        clearDisplay();
        bx += bvx; by += bvy;
        
        // Player Control (Tilt Y) - Smoother movement
        int targetY = map(constrain((int)accX, -4000, 4000), -4000, 4000, 0, 7);
        playPos = (playPos * 0.7) + (targetY * 0.3); 
        
        // AI Control (Slightly imperfect to make it beatable)
        if (by > aiPos + 1.5) aiPos += 0.2;
        else if (by < aiPos + 1.5) aiPos -= 0.2;
        aiPos = constrain(aiPos, 0, 7);
        
        // Walls
        if (by < 0 || by > 9) bvy *= -1;

        // Paddles Logic
        if (bx < 1) { 
            if (by >= aiPos - 1 && by <= aiPos + 4) { bx = 1; bvx = abs(bvx) * 1.05; } 
            else if (bx < -2) { setup(); } // Reset if missed
        }
        if (bx > 14) { 
            if (by >= playPos - 1 && by <= playPos + 4) { bx = 14; bvx = -abs(bvx) * 1.05; } 
            else if (bx > 17) { setup(); } // Reset if missed
        }

        // Draw Paddles
        for (int i = 0; i < 3; i++) setPixel((int)aiPos + i, 0, 1);
        for (int i = 0; i < 3; i++) setPixel((int)playPos + i, 15, 1);
        // Draw Ball
        setPixel((int)constrain(by, 0, 9), (int)constrain(bx, 0, 15), 1); 
    }
};