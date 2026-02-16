#include "DisplayDriver.h"
#include "../Globals.h" 
#include <SPI.h>

// --- Pins ---
#define PIN_DATA 23 
#define PIN_CLK  18 
#define PIN_LATCH 5
const int ROWS[10] = {32, 33, 25, 26, 27, 14, 19, 13, 16, 4};

// --- Macros ---
#define FAST_HIGH(pin) (pin < 32 ? (GPIO.out_w1ts = (1UL << pin)) : (GPIO.out1_w1ts.val = (1UL << (pin - 32))))
#define FAST_LOW(pin)  (pin < 32 ? (GPIO.out_w1tc = (1UL << pin)) : (GPIO.out1_w1tc.val = (1UL << (pin - 32))))

volatile int currentRow = 0;
hw_timer_t * timer = NULL;

void IRAM_ATTR onTimer() {
    // 1. Turn off rows
    for (int i = 0; i < 10; i++) FAST_HIGH(ROWS[i]);

    // 2. Get Data from Global Canvas
    uint16_t rowBits = 0;
    for (int c = 0; c < 16; c++) {
        if (canvas.getPixel(currentRow, c)) {
            rowBits |= (1 << c);
        }
    }
    rowBits = ~rowBits; // Invert for Active Low logic

    // 3. Send Data (Byte 1 Normal MSB, Byte 2 Reversed)
    // Low Byte (Cols 0-7)
    for(int b = 7; b >= 0; b--) {
        if((rowBits >> b) & 1) FAST_HIGH(PIN_DATA); else FAST_LOW(PIN_DATA);
        FAST_HIGH(PIN_CLK); FAST_LOW(PIN_CLK);
    }
    // High Byte (Cols 8-15) - Reversed Order
    for(int b = 8; b <= 15; b++) {
        if((rowBits >> b) & 1) FAST_HIGH(PIN_DATA); else FAST_LOW(PIN_DATA);
        FAST_HIGH(PIN_CLK); FAST_LOW(PIN_CLK);
    }

    // 4. Latch & Activate Row
    FAST_HIGH(PIN_LATCH); FAST_LOW(PIN_LATCH);
    FAST_LOW(ROWS[currentRow]);

    // 5. Increment
    currentRow++;
    if (currentRow >= 10) currentRow = 0;
}

void setupDisplayDriver() {
    pinMode(PIN_DATA, OUTPUT);
    pinMode(PIN_CLK, OUTPUT);
    pinMode(PIN_LATCH, OUTPUT);
    for (int i = 0; i < 10; i++) {
        pinMode(ROWS[i], OUTPUT);
        FAST_HIGH(ROWS[i]); 
    }
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000, true); 
    timerAlarmEnable(timer);
}