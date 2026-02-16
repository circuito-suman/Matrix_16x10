#pragma once
#include "Mode.h"
#include "Globals.h"

// This mode does NOTHING in the loop.
// It relies on the BLE Comms Manager to write directly to the canvas memory.
class BleCanvasMode : public Mode {
public:
    void setup() override {
        canvas.fillScreen(0);
        // Draw an "Waiting for App" icon
        canvas.setCursor(0,0);
        canvas.print("APP");
    }
    void loop() override {
        // Do nothing, just display whatever the BLE puts in the buffer
    }
    const char* getName() override { return "App Controlled"; }
};