#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeMarble : public Mode {
    float ballX, ballY, velX, velY;
    unsigned long lastUpdate = 0;
public:
    const char* getName() override { return "Gyro Marble"; }
    
    void setup() override { 
        ballX = 4.5; ballY = 7.5; velX = 0; velY = 0; 
        lastUpdate = millis();
    }
    
    void loop() override {
        // LOCK SPEED: Run physics only every 16ms (approx 60 FPS)
        if (millis() - lastUpdate < 16) return;
        lastUpdate = millis();

        clearDisplay();
        
        // Physics (Tweak these numbers to adjust feel)
        // 0.00005 = Lower sensitivity
        // 0.92 = More friction (stops faster)
        velX = (velX + (accX * 0.00005)) * 0.92;
        velY = (velY + (accY * 0.00005)) * 0.92;
        
        // Speed Limit (Prevents teleporting)
        if (velX > 0.8) velX = 0.8; if (velX < -0.8) velX = -0.8;
        if (velY > 0.8) velY = 0.8; if (velY < -0.8) velY = -0.8;

        ballX += velX; 
        ballY += velY;

        // Bounce Logic
        if(ballX < 0) { ballX=0; velX *= -0.5; } 
        if(ballX > 9) { ballX=9; velX *= -0.5; }
        if(ballY < 0) { ballY=0; velY *= -0.5; } 
        if(ballY > 15){ ballY=15; velY *= -0.5; }
        
        setPixel((int)ballX, (int)ballY, 1);
    }
};