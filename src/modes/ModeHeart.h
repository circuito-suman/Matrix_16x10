#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeHeart : public Mode {
public:
    const char* getName() override { return "Floating Heart"; }
    void setup() override {}
    void loop() override {
        clearDisplay();
        int hx = 4 + (accX / 3000); 
        int hy = 7 + (accY / 3000);
        setPixel(hx, hy, 1); 
        setPixel(hx, hy-1, 1); setPixel(hx, hy+1, 1);
        setPixel(hx-1, hy-1, 1); setPixel(hx-1, hy+1, 1);
        setPixel(hx+1, hy, 1);
    }
};