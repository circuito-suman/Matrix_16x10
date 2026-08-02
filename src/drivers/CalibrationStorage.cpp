#include "CalibrationStorage.h"
#include <SPIFFS.h>

static const char *kCalibPath = "/calib.txt";

bool loadCalibration(float &cx, float &cy) {
    if (!SPIFFS.begin(true)) return false;
    if (!SPIFFS.exists(kCalibPath)) return false;
    File f = SPIFFS.open(kCalibPath, FILE_READ);
    if (!f) return false;
    String sx = f.readStringUntil('\n');
    String sy = f.readStringUntil('\n');
    f.close();
    if (sx.length() == 0 || sy.length() == 0) return false;
    cx = sx.toFloat();
    cy = sy.toFloat();
    return true;
}

bool saveCalibration(float cx, float cy) {
    if (!SPIFFS.begin(true)) return false;
    File f = SPIFFS.open(kCalibPath, FILE_WRITE);
    if (!f) return false;
    f.print(cx, 6);
    f.print('\n');
    f.print(cy, 6);
    f.print('\n');
    f.close();
    return true;
}

bool clearCalibration() {
    if (!SPIFFS.begin(true)) return false;
    if (SPIFFS.exists(kCalibPath)) {
        return SPIFFS.remove(kCalibPath);
    }
    return true;
}
