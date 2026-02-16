#pragma once
#include "Mode.h"
#include "Globals.h"
class ModeLife : public Mode {
    uint16_t nextGen[10];
    unsigned long lastUpdate = 0; 
    unsigned long lastReset = 0;
public:
    const char* getName() override { return "Game of Life"; }
    void setup() override { 
        clearDisplay();
        for(int i=0; i<40; i++) setPixel(random(10), random(16), 1);
        lastUpdate = millis();
        lastReset = millis();
    }
    
    void loop() override {
        // LOCK SPEED: Update every 800ms (Almost 1 second)
        if (millis() - lastUpdate < 800) return;
        lastUpdate = millis();
        
        bool changed = false;
        memset(nextGen, 0, sizeof(nextGen)); 

        for (int r=0; r<10; r++) {
            for (int c=0; c<16; c++) {
                int neighbors = 0;
                for (int i=-1; i<=1; i++) {
                    for (int j=-1; j<=1; j++) {
                        if (i==0 && j==0) continue;
                        if (getPixel(r+i, c+j)) neighbors++;
                    }
                }
                bool alive = getPixel(r,c);
                if (alive && (neighbors == 2 || neighbors == 3)) { nextGen[r] |= (1<<c); }
                else if (!alive && neighbors == 3) { nextGen[r] |= (1<<c); changed = true; }
                if (alive) changed = true;
            }
        }
        
        clearDisplay();
        for(int r=0; r<10; r++) {
            for(int c=0; c<16; c++) if((nextGen[r] >> c) & 1) setPixel(r,c,1);
        }
        
        if (!changed || (millis() - lastReset > 15000)) { // Reset every 15s
            setup();
            lastReset = millis();
        }
    }
};