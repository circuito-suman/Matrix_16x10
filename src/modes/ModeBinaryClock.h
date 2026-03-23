#pragma once
#include "Mode.h"
#include "Globals.h"
#include <time.h>

class ModeBinaryClock : public Mode {
    // Columns:
    // 0: H1 (0-2)
    // 1: H2 (0-9)
    // 3: M1 (0-5)
    // 4: M2 (0-9)
    // 6: S1 (0-5)
    // 7: S2 (0-9)
    
    // Rows: LSB at bottom (15), MSB going up.

public:
    const char* getName() override { return "Binary Clock"; }

    void setup() override {
        // configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        // For now, assuming time is somewhat valid or just running from 0
    }

    void loop() override {
        clearDisplay();
        
        time_t now;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            // If no time, maybe just use millis/1000
            unsigned long t = millis() / 1000;
            timeinfo.tm_hour = (t / 3600) % 24;
            timeinfo.tm_min = (t / 60) % 60;
            timeinfo.tm_sec = t % 60;
        }

        drawDigit(1, timeinfo.tm_hour / 10);
        drawDigit(2, timeinfo.tm_hour % 10);
        
        drawDigit(4, timeinfo.tm_min / 10);
        drawDigit(5, timeinfo.tm_min % 10);
        
        drawDigit(7, timeinfo.tm_sec / 10);
        drawDigit(8, timeinfo.tm_sec % 10);

        delay(100);
    }
    
    void drawDigit(int col, int val) {
        for (int b = 0; b < 4; b++) {
            if (val & (1 << b)) {
                canvas.drawPixel(col, MATRIX_HEIGHT - 1 - b*2, 1); // Spaced out vertically
            }
        }
    }
};
