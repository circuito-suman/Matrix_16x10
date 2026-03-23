#pragma once
#include "Mode.h"
#include "Globals.h"

// Infinite Tunnel Effect
// Concentric rectangles moving outwards.

class ModeTunnel : public Mode {
    float size[4]; // Sizes of 4 rectangles
    float speed = 0.5;

public:
    const char* getName() override { return "Tunnel"; }

    void setup() override {
        // Init sizes spaced out
        // Max size is roughly 16 (height) or 10 (width)
        // Let's track "radius" or "scale" from 0 to 10
        for(int i=0; i<4; i++) {
            size[i] = i * 4.0; 
        }
    }

    void loop() override {
        clearDisplay();
        
        // Z movement
        for(int i=0; i<4; i++) {
            size[i] -= 0.1;
            if (size[i] < 1.0) size[i] = 11.0; 
        }

        int cx = MATRIX_WIDTH / 2;
        int cy = MATRIX_HEIGHT / 2;

        for(int i=0; i<4; i++) {
            float z = size[i];
            
            // Perspective transform: x' = x/z
            // Let's assume original rect is (-4,-6) to (4,6) at z=1
            
            float scale = 6.0 / z; // Adjust 6.0 for FOV
            
            int w = (int)(6 * scale); // Base width
            int h = (int)(10 * scale); // Base height

            int x = cx - (w / 2);
            int y = cy - (h / 2);

            canvas.drawRect(x, y, w, h, 1);
            
            // Connect to center diagonals?
            // If z is large (far away), it's small center rect.
            // If z is small (close), it's large outer rect.
        }
        
        // Diagonals always present gives structure
        canvas.drawPixel(0,0,1); canvas.drawPixel(1,1,1);
        canvas.drawPixel(9,0,1); canvas.drawPixel(8,1,1);
        canvas.drawPixel(0,15,1); canvas.drawPixel(1,14,1);
        canvas.drawPixel(9,15,1); canvas.drawPixel(8,14,1);

        delay(30);
    }
};
