#pragma once
#include <Ticker.h> // Standard ESP32 Timer Library
#include "Mode.h"
#include "Globals.h"
#include "Config.h"

// --- CONFIGURATION ---
#define WORK_MINS 20
#define BREAK_MINS 5
#define TOTAL_CYCLES 5

// LED MAPPING
#define LEDS_PER_WORK 20
#define LEDS_PER_BREAK 10 
#define PIXELS_PER_CYCLE 30 

class ModePomodoro : public Mode {
    Ticker timer; // The hardware timer object
    
    // Volatile is crucial! It tells the compiler this variable 
    // changes unexpectedly (inside an interrupt)
    volatile unsigned long totalSeconds = 0; 
    
    int currentCycle = 0;
    bool isBreak = false;
    
    // State tracking to prevent re-triggering sound
    int lastProcessedMinute = -1; 

public:
    const char* getName() override { return "Pomodoro Pro"; }

    // --- STATIC CALLBACK ---
    // Timers need a static function to jump into the class instance
    static void onTimerTick(ModePomodoro* instance) {
        instance->totalSeconds++;
    }

    void setup() override {
        pinMode(BUZZER_PIN, OUTPUT);
        
        // Reset Logic
        totalSeconds = 0;
        currentCycle = 0;
        isBreak = false;
        lastProcessedMinute = -1;
        
        clearDisplay();
        
        // Start Timer: Call 'onTimerTick', passing 'this' instance, every 1.0 seconds
        timer.attach(1.0, onTimerTick, this);
        
        // Ready Beep
        tone(BUZZER_PIN, 2000, 100);
    }
    
    // Cleanup is important! Stop the timer if we switch modes.
    // Since Mode.h might not have a destructor or 'exit' method, 
    // we rely on the fact that 'setup()' is called when entering.
    // Ideally, add a 'teardown()' to your Mode system later.
    // For now, we detach in setup() to be safe or rely on object destruction.
    // Note: If you destroy this object (delete mode), the timer MUST be detached.
    ~ModePomodoro() {
        timer.detach();
    }

    void loop() override {
        // --- 1. CALCULATE TIME ---
        // Convert raw seconds into Pomodoro logic
        // We make a local copy to ensure math consistency
        unsigned long currentSecs = totalSeconds;
        
        int currentMinute = currentSecs / 60;
        
        // Only run logic if we entered a new minute
        if (currentMinute > lastProcessedMinute) {
            lastProcessedMinute = currentMinute;
            
            // --- LOGIC: WORK PHASE ---
            if (!isBreak) {
                // Check if Work is Done
                if (currentMinute >= WORK_MINS) {
                    beepWorkFinished();
                    isBreak = true;
                    // Reset Seconds Counter for the new phase
                    // We subtract the time consumed so far to keep the timer running smoothly
                    noInterrupts(); // Pause interrupts briefly to write safely
                    totalSeconds = 0;
                    currentMinute = 0;
                    lastProcessedMinute = 0;
                    interrupts();
                }
            } 
            // --- LOGIC: BREAK PHASE ---
            else {
                // Check if Break is Done
                if (currentMinute >= BREAK_MINS) {
                    currentCycle++;
                    isBreak = false;
                    
                    // Reset Counter
                    noInterrupts();
                    totalSeconds = 0;
                    currentMinute = 0;
                    lastProcessedMinute = 0;
                    interrupts();

                    if (currentCycle >= TOTAL_CYCLES) {
                        playVictoryMelody();
                        // Reset Whole Game
                        currentCycle = 0;
                    } else {
                        playBackToWorkMelody();
                    }
                }
            }
            
            // Force redraw on minute change
            drawProgress(currentMinute); 
        }
        
        // Optional: Blink logic can run every loop without affecting time
        if((millis() % 1000) < 500) {
             drawBlink(currentMinute);
        } else {
             // Redraw static state to "turn off" blink
             drawProgress(currentMinute);
        }
    }

private:
    void drawProgress(int minute) {
        // This function draws the base state (non-blinking parts)
        
        // 1. Draw COMPLETED Cycles
        for (int c = 0; c < currentCycle; c++) {
            int startPixel = c * PIXELS_PER_CYCLE;
            for (int i = 0; i < PIXELS_PER_CYCLE; i++) setPixelLinear(startPixel + i, 1);
        }

        // 2. Draw CURRENT Cycle
        int currentStart = currentCycle * PIXELS_PER_CYCLE;
        
        if (!isBreak) {
            // Fill up to current minute
            for (int i = 0; i < minute; i++) setPixelLinear(currentStart + i, 1);
            // Ensure the rest of this block is off (clearing previous blinks)
            setPixelLinear(currentStart + minute, 0); 
        } else {
            // Fill Work block
            for (int i = 0; i < LEDS_PER_WORK; i++) setPixelLinear(currentStart + i, 1);
            
            // Fill Break block up to current minute
            int breakPixels = minute * 2;
            for (int i = 0; i < breakPixels; i++) setPixelLinear(currentStart + LEDS_PER_WORK + i, 1);
             // Ensure blink spot is off
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels, 0);
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels + 1, 0);
        }
        // matrix.show() is called in main loop
    }
    
    void drawBlink(int minute) {
        int currentStart = currentCycle * PIXELS_PER_CYCLE;
        
        if (!isBreak) {
            // Blink current work minute
            setPixelLinear(currentStart + minute, 1);
        } else {
            // Blink current break segment (2 pixels)
            int breakPixels = minute * 2;
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels, 1);
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels + 1, 1);
        }
    }

    void setPixelLinear(int index, uint8_t color) {
        if (index >= 160) return;
        // Adjust for your specific Matrix Layout
        int y = index / 10; // for 10 rows, then remainder gives us the column
        int x = index % 10; // for 10 columns
        
        // Invert Y so index 0 starts at the Left instead of Right
        // (15 - y) flips the horizontal position
        y = 15 - y;

        if(x < 0 || x >= 10 || y < 0 || y >= 16) return; // Bounds check
        setPixel(x, y, color);
    }

    // --- SOUNDS (Non-blocking ideally, but blocking is okay for alerts) ---
    void beepWorkFinished() {
        tone(BUZZER_PIN, 1000, 200); delay(250);
        tone(BUZZER_PIN, 1000, 200); 
    }

    void playBackToWorkMelody() {
        tone(BUZZER_PIN, 600, 150); delay(150);
        tone(BUZZER_PIN, 1200, 300); 
    }

    void playVictoryMelody() {
        int melody[] = {523, 659, 784, 1046, 784, 1046}; 
        int duration[] = {150, 150, 150, 400, 150, 600};
        for (int i = 0; i < 6; i++) {
            tone(BUZZER_PIN, melody[i], duration[i]);
            delay(duration[i] * 1.3);
        }
    }
};