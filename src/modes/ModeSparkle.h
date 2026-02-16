#pragma once
#include "Mode.h"
#include "Globals.h"

class ModeSparkle : public Mode {
    unsigned long lastUpdate = 0;
public:
    const char* getName() override { return "Sparkle"; }
    void setup() override { clearDisplay(); }
    void loop() override {
        if (millis() - lastUpdate < 30) return; // 30ms speed
        lastUpdate = millis();

        int r = random(10);
        int c = random(16);
        if(getPixel(r,c)) setPixel(r,c,0);
        else setPixel(r,c,1);
    }
};