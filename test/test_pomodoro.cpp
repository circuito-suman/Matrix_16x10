#include <Arduino.h>
#include <unity.h>
#include "Globals.h"
#include "Config.h"
#include "modes/ModePomodoro.h"
#include "drivers/DisplayDriver.cpp" // Needed to actually light up LEDs

// 1. Create the necessary global objects
GFXcanvas1 canvas(MATRIX_WIDTH, MATRIX_HEIGHT); 
SemaphoreHandle_t dispMutex; // Required if your DisplayDriver uses it
ModePomodoro* pomodoro;

void setUp(void) {
    // Initialize Hardware
    Serial.begin(115200);
    
    // Create the mutex (lock) just in case DisplayDriver needs it
    dispMutex = xSemaphoreCreateMutex();
    
    // Initialize your LED Matrix Driver
    // (This ensures pixels actually light up on the board)
    setupDisplayDriver(); 

    // Initialize the Mode
    pomodoro = new ModePomodoro();
    pomodoro->setup();
}

void tearDown(void) {
    delete pomodoro;
    // Optional: Turn off LEDs after test
    canvas.fillScreen(0);
    // You might need a function to force-clear the physical display here
}

void test_pomodoro_fast_forward(void) {
    Serial.println(">>> STARTING VISUAL TEST <<<");

    for (int minute = 0; minute <= 160; minute++) { // Just test first 5 mins
        // 1. Simulate 60 seconds
        for(int s=0; s<60; s++) {
             ModePomodoro::onTimerTick(pomodoro);
        }

        // 2. Run logic to draw to canvas
        pomodoro->loop();

        bool canvasHasData = false;
        for(int i=0; i < (MATRIX_WIDTH * MATRIX_HEIGHT); i++) {
            if(canvas.getBuffer()[i] > 0) {
                canvasHasData = true;
                break;
            }
        }

        TEST_ASSERT_TRUE_MESSAGE(canvasHasData, "Canvas should not be empty after a minute passes");

        Serial.printf("Minute %d simulated. Canvas updated.\n", minute);
        delay(500); 
    }
}

void setup() {
    delay(2000); 
    UNITY_BEGIN();
    
    RUN_TEST(test_pomodoro_fast_forward);
    
    UNITY_END();
}

void loop() {
}