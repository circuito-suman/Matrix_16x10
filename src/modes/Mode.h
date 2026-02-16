// src/modes/Mode.h
#pragma once
#include <Adafruit_GFX.h>

class Mode {
public:
    virtual void setup() = 0;
    virtual void loop() = 0; // Returns true if it wants to stay active
    virtual const char* getName() = 0;
    virtual ~Mode() {} // Virtual destructor
};