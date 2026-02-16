#include <Arduino.h>
#include <OneButton.h>
#include <Wire.h>
#include "Config.h"
#include "Globals.h"
#include "drivers/DisplayDriver.h"

// --- INCLUDE MODES ---
#include "modes/ModeMarble.h"
#include "modes/ModeSparkle.h"
#include "modes/ModeFluid.h"
#include "modes/ModeHeart.h"
#include "modes/ModeLife.h"
#include "modes/ModePong.h"
#include "modes/ModeSnake.h"
#include "modes/ModeTetris.h"
#include "modes/ModeScroll.h"
#include "modes/ModeMatrix.h"
#include "modes/ModePomodoro.h"

int savedModeIndex = 0;      // To remember where we were
bool isSpecialMode = false;  // To track if we are in the special mode
const int SPECIAL_MODE_ID = 10; // Index of ModeMatrix (or whichever you want)
const int VISIBLE_MODES = 10;
// --- GLOBALS ---
GFXcanvas1 canvas(MATRIX_WIDTH, MATRIX_HEIGHT);
MPU6050 mpu(0x68, &Wire);
float accX = 0, accY = 0;

// NEW: Calibration Variables
float calibX = 0, calibY = 0;

SemaphoreHandle_t dispMutex;
volatile bool modeChangeRequest = false;
volatile int requestedModeIndex = 0;

Mode* currentMode = nullptr;
Mode* allModes[11]; 
const int MODE_COUNT = 11;
int modeIndex = 0;

OneButton btn(btn_pin, true); 

// --- FAST LOGIC ENGINE ---
void taskGameEngine(void * parameter) {
    // 1. Hardware Init
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C Speed
    mpu.initialize();
    
    // --- 2. CALIBRATION SEQUENCE ---
    // Visual Cue: Clear screen
    xSemaphoreTake(dispMutex, portMAX_DELAY);
    canvas.fillScreen(0);
    xSemaphoreGive(dispMutex);

    long sumX = 0, sumY = 0;
    const int SAMPLES = 100;

    // Collect 100 samples (takes ~1 second)
    for(int i=0; i<SAMPLES; i++) {
        int16_t rawX, rawY, rawZ;
        mpu.getAcceleration(&rawX, &rawY, &rawZ);
        
        sumX += rawX;
        sumY += rawY;

        // Blinking Center Dot Animation
        if(i % 10 == 0) {
            xSemaphoreTake(dispMutex, portMAX_DELAY);
            // Toggle center pixel
            bool on = (i / 10) % 2 == 0;
            canvas.drawPixel(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, on ? 1 : 0);
            xSemaphoreGive(dispMutex);
        }
        vTaskDelay(10); // 10ms delay * 100 samples = 1000ms total
    }

    // Calculate Average Offset
    calibX = (float)sumX / SAMPLES;
    calibY = (float)sumY / SAMPLES;

    // Clear Screen after calibration
    xSemaphoreTake(dispMutex, portMAX_DELAY);
    canvas.fillScreen(0);
    xSemaphoreGive(dispMutex);
    // -------------------------------

    // 3. Instantiate Modes
    allModes[0] = new ModeMarble();
    allModes[1] = new ModeSparkle();
    allModes[2] = new ModeFluid();
    allModes[3] = new ModeHeart();
    allModes[4] = new ModeLife();
    allModes[5] = new ModePong();
    allModes[6] = new ModeSnake();
    allModes[7] = new ModeTetris();
    allModes[8] = new ModeScroll();
    allModes[9] = new ModeMatrix();
    allModes[10] = new ModePomodoro();

    currentMode = allModes[0];
    currentMode->setup();

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(true) {
        // A. Read Sensors (Apply Calibration)
        int16_t rawX, rawY, rawZ;
        mpu.getAcceleration(&rawX, &rawY, &rawZ);
        
        // Subtract Calibration Data
        float adjX = rawX - calibX;
        float adjY = rawY - calibY;

        // Simple Low Pass Filter
        accX = (accX * 0.7) + (adjX * 0.3);
        
        // Note: Preserving your original negative sign for Y axis orientation
        // If Y is inverted, we invert the *calibrated* value
        accY = (accY * 0.7) + (-adjY * 0.3); 

        // B. Instant Mode Switching
        if (modeChangeRequest) {
            xSemaphoreTake(dispMutex, portMAX_DELAY);
            modeIndex = requestedModeIndex % MODE_COUNT;
            currentMode = allModes[modeIndex];
            currentMode->setup();
            modeChangeRequest = false;
            xSemaphoreGive(dispMutex);
        }

        btn.tick(); 

        // C. Run Logic
        if (xSemaphoreTake(dispMutex, 5) == pdTRUE) { 
            currentMode->loop();
            xSemaphoreGive(dispMutex);
        }

        // D. Non-blocking Delay
        vTaskDelay(1); 
    }
}

void taskCommsWorker(void * parameter) {
    while(true) vTaskDelay(100); 
}

void nextMode() {
    if (isSpecialMode) return; // skip if in special mode


    int next = modeIndex + 1;
    if (next > (VISIBLE_MODES-1)) {
        next = 0; // Wrap back to Marble
    }

    requestedModeIndex = next;
    modeChangeRequest = true;
}
void resetMode() {
    if (isSpecialMode) return; // skip if in special mode

    modeIndex = 0;
    requestedModeIndex = modeIndex; 
    modeChangeRequest = true;
}

void toggleSpecialMode() {
    if (!isSpecialMode) {
        savedModeIndex = modeIndex;       // 1. Remember current game
        requestedModeIndex = SPECIAL_MODE_ID; // 2. Target the specific mode
        isSpecialMode = true;             // 3. Set flag
    } else {
        requestedModeIndex = savedModeIndex; // 1. Restore old game
        isSpecialMode = false;            // 2. Clear flag
    }
    modeChangeRequest = true; // Trigger the switch in the main loop
}

void setup() {
    // Serial.begin(115200); 
    
    dispMutex = xSemaphoreCreateMutex();
    btn.attachClick(nextMode);
    btn.attachDoubleClick(resetMode);
    btn.attachLongPressStart(toggleSpecialMode);

    // Priority 2 for Game Engine
    xTaskCreatePinnedToCore(taskGameEngine, "Game", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskCommsWorker, "Comms", 4096, NULL, 0, NULL, 0);

    setupDisplayDriver();
}

void loop() { vTaskDelete(NULL); }