#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeScanner : public Mode {
    float pos = 0;
    float speed = 0.8;
    int tail[6]; // Store previous positions for tail effect
    
public:
    const char* getName() override { return "Cylon Scanner"; }

    void setup() override {
        pos = 0;
        speed = 0.5;
        for(int i=0; i<6; i++) tail[i] = -1;
    }

    void loop() override {
        clearDisplay();
        
        // Move (Sine wave is smoother)
        // pos goes 0 to MATRIX_HEIGHT-1 and back
        
        pos += speed;
        if (pos >= MATRIX_HEIGHT - 1.5) {
             pos = MATRIX_HEIGHT - 1.5;
             speed = -abs(speed); // Go UP
        } else if (pos <= 0.5) {
             pos = 0.5;
             speed = abs(speed); // Go DOWN
        }
        
        int y = (int)pos;
        
        // Shift Tail
        for(int i=5; i>0; i--) tail[i] = tail[i-1];
        tail[0] = y;

        // Draw
        int width = 8;
        int startX = (MATRIX_WIDTH - width) / 2;

        // Draw Head (Full width bar)
        for(int x=startX; x<startX+width; x++) {
             canvas.drawPixel(x, y, 1);
        }

        // Draw Tail
        for(int i=1; i<4; i++) {
            int ty = tail[i];
            if (ty < 0 || ty >= MATRIX_HEIGHT) continue;
            if (ty == y) continue;

            // Dither pattern based on index
            // i=1 (fresh tail): skip every 2nd pixel?
            // i=2: skip every 2nd pixel but offset
            // i=3: center dots only
            
            for(int x=startX; x<startX+width; x++) {
                if (i == 1) {
                    if ((x+ty)%2 == 0) canvas.drawPixel(x, ty, 1);
                } else if (i == 2) {
                    if ((x+ty)%3 == 0) canvas.drawPixel(x, ty, 1);
                } else {
                    if (x > startX + 2 && x < startX + width - 2)
                        if ((x+ty)%2 != 0) canvas.drawPixel(x, ty, 1);
                }
            }
        }
        
        delay(30);
    }
};
