#pragma once
#include "Mode.h"
#include "Globals.h"

// Game of 15 (Sliding Puzzle)
// We need a 3x4 grid for a 10x16 display? 
// 10 width / 3 approx 3.
// 16 height / 4 = 4.
// Let's do a 3x3 grid of tiles. Each tile 3x3 pixels.
// Total size 9x9 pixels. Fits easily.

class ModePuzzle : public Mode {
    int grid[3][3];
    int emptyX, emptyY;
    bool solved = false;
    unsigned long lastMove = 0;

public:
    const char* getName() override { return "15 Puzzle"; }

    void setup() override {
        // Init solved state
        int n = 1;
        for(int y=0; y<3; y++) {
            for(int x=0; x<3; x++) {
                grid[y][x] = n++;
            }
        }
        grid[2][2] = 0; // Empty slot
        emptyX = 2; emptyY = 2;
        
        // Shuffle
        for(int i=0; i<100; i++) {
            // Pick random direction
            int r = random(4);
            int dx = 0, dy = 0;
            if (r==0) dy=-1; else if (r==1) dy=1; else if (r==2) dx=-1; else dx=1;
            moveEmpty(dx, dy);
        }
        solved = false;
    }

    bool handleButton() override {
         setup();
         return true;
    }

    void moveEmpty(int dx, int dy) {
        int nx = emptyX + dx;
        int ny = emptyY + dy;
        
        if (nx >= 0 && nx < 3 && ny >= 0 && ny < 3) {
            // Swap
            grid[emptyY][emptyX] = grid[ny][nx];
            grid[ny][nx] = 0;
            emptyX = nx;
            emptyY = ny;
        }
    }

    void loop() override {
        canvas.fillScreen(0);
        
        // Handle Tilt
        float thresh = 4000;
        if (millis() - lastMove > 300) {
            // Check tilt
            if (accX > thresh) { moveEmpty(-1, 0); lastMove = millis(); }
            else if (accX < -thresh) { moveEmpty(1, 0); lastMove = millis(); }
            
            if (accY < -thresh) { moveEmpty(0, -1); lastMove = millis(); }
            else if (accY > thresh) { moveEmpty(0, 1); lastMove = millis(); }
        }

        // Draw 3x3 Grid
        int startX = 0; 
        int startY = 2; 

        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                int val = grid[y][x];
                if (val == 0) continue; // Empty slot
                
                int px = startX + (x * 4); 
                int py = startY + (y * 4); 
                
                // Draw Patterns
                if (val == 1) canvas.drawPixel(px, py, 1);
                else if (val == 2) canvas.drawPixel(px+2, py, 1);
                else if (val == 3) canvas.drawPixel(px, py+2, 1);
                else if (val == 4) canvas.drawPixel(px+2, py+2, 1);
                else if (val == 5) canvas.drawPixel(px+1, py+1, 1);
                else if (val == 6) canvas.drawFastHLine(px, py, 3, 1);
                else if (val == 7) canvas.drawFastHLine(px, py+2, 3, 1);
                else if (val == 8) canvas.drawRect(px, py, 3, 3, 1);
            }
        }
        
        delay(50);
    }
};
};
