#pragma once
#include <Arduino.h>

// Persist/load simple calibration values (two floats) using SPIFFS
bool loadCalibration(float &cx, float &cy);
bool saveCalibration(float cx, float cy);
bool clearCalibration();
