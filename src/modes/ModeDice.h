#pragma once
#include "Mode.h"
#include "Globals.h"

// Dice Mode
// States: 0=Waiting, 1=Rolling/Shaking, 2=Result

class ModeDice : public Mode {
    int state = 0;
    int roll = 1;
    unsigned long shakingStart = 0;
    unsigned long resultTime = 0;

public:
    const char* getName() override { return "Dice"; }

    void setup() override {
        state = 0;
        roll = 1;
    }

    void loop() override {
        // Detect Shake
        float totalAcc = abs(accX) + abs(accY); // Simple magnitude
        
        if (state == 0) {
            // Waiting
            canvas.setTextSize(1);
            canvas.setCursor(0, 4);
            if ((millis()/500)%2) canvas.print("SHAKE");
            
            if (totalAcc > 4000) {
                state = 1;
                shakingStart = millis();
            }
        } 
        else if (state == 1) {
            // Rolling
            if (millis() - shakingStart > 1500 && totalAcc < 1000) {
                // Done shaking
                state = 2;
                resultTime = millis();
                roll = random(1, 7);
            } else {
                // Flash random numbers
                if ((millis()/100)%2) {
                    canvas.fillScreen(0);
                    drawDice(random(1, 7));
                }
            }
        } 
        else if (state == 2) {
            // Show Result
            canvas.fillScreen(0);
            drawDice(roll);
            
            if (millis() - resultTime > 3000) {
                state = 0;
            }
        }
    }

    void drawDice(int num) {
        // 5x5 box
        // Margin: (10-5)/2 = 2.5 -> start at 2
        // Height: (16-5)/2 = 5.5 -> start at 5
        
        int startX = 2;
        int startY = 5;
        canvas.drawRect(startX, startY, 7, 7, 1);
        
        // Dots
        // Top Left: 0,0
        // Center: 1,1
        // Bottom Right: 2,2
        // We actually have 5x5 inner area (indices 0..4)
        // Let's use 3x3 logical grid inside the 5x5 box
        // Points: (3,6), (5,6), (7,6) ...
        
        if (num % 2 == 1) canvas.drawPixel(5, 8, 1); // Center (1,3,5)
        
        if (num > 1) {
            canvas.drawPixel(3, 6, 1); // TL
            canvas.drawPixel(7, 10, 1); // BR
        }
        
        if (num > 3) {
            canvas.drawPixel(7, 6, 1); // TR
            canvas.drawPixel(3, 10, 1); // BL
        }
        
        if (num == 6) {
            canvas.drawPixel(3, 8, 1); // ML
            canvas.drawPixel(7, 8, 1); // MR
        }
    }
};
