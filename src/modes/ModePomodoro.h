#pragma once
#include <Ticker.h> 
#include <Arduino.h>
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
    Ticker timer; 
    volatile unsigned long totalSeconds = 0; 
    
    int currentCycle = 0;
    bool isBreak = false;
    int lastProcessedMinute = -1; 

public:
    const char* getName() override { return "Pomodoro Pro"; }

    static void onTimerTick(ModePomodoro* instance) {
        instance->totalSeconds++;
    }

    void setup() override {
        // --- 1. SAFE BUZZER INIT ---
        // Initialize the specific channel defined in Globals.h
        ledcSetup(BUZZER_CHANNEL, 2000, 8); 
        ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
        ledcWrite(BUZZER_CHANNEL, 0); // Ensure silence initially
        
        // Reset Logic
        totalSeconds = 0;
        currentCycle = 0;
        isBreak = false;
        lastProcessedMinute = -1;
        
        clearDisplay(); // Ensure you have this function or use canvas.fillScreen(0)
        
        // Start Timer
        timer.attach(1.0, onTimerTick, this);
        
        // Ready Beep (Using safe function)
        playTone(2000, 100);
    }
    
    ~ModePomodoro() {
        timer.detach();
        // Force Silence on Exit so it doesn't get stuck on
        ledcWrite(BUZZER_CHANNEL, 0);
    }

    void loop() override {
        // --- 1. CALCULATE TIME ---
        unsigned long currentSecs = totalSeconds;
        int currentMinute = currentSecs / 60;
        
        // Draw Updates
        // Note: moved draw logic outside the minute check slightly so we can handle blinking
        // But logic processing happens only on minute change
        
        if (currentMinute > lastProcessedMinute) {
            // Logic Triggered only when a minute passes
            
            // --- LOGIC: WORK PHASE ---
            if (!isBreak) {
                if (currentMinute >= WORK_MINS) {
                    beepWorkFinished();
                    isBreak = true;
                    // Reset Counters safely
                    resetCounters();
                    currentMinute = 0; // Local sync
                }
            } 
            // --- LOGIC: BREAK PHASE ---
            else {
                if (currentMinute >= BREAK_MINS) {
                    currentCycle++;
                    isBreak = false;
                    resetCounters();
                    currentMinute = 0; // Local sync

                    if (currentCycle >= TOTAL_CYCLES) {
                        playVictoryMelody();
                        currentCycle = 0;
                    } else {
                        playBackToWorkMelody();
                    }
                }
            }
            lastProcessedMinute = currentMinute;
        }
        
        // --- DRAWING ---
        // Blink logic: On for 500ms, Off for 500ms
        if((millis() % 1000) < 500) {
             drawBlink(currentMinute);
        } else {
             drawProgress(currentMinute);
        }
        
        // IMPORTANT: If your main loop doesn't handle display show(), do it here
        // If using the DisplayDriver task, this is not needed.
    }

private:
    // Helper to reset counters atomically
    void resetCounters() {
        noInterrupts();
        totalSeconds = 0;
        lastProcessedMinute = -1; // -1 so the loop picks up 0 immediately if needed
        interrupts();
    }

    // --- DRAWING HELPERS ---
    void drawProgress(int minute) {
        // Clear canvas logic if needed, or rely on overwriting
        // For efficiency, we usually clear first or ensure we write 0s to unused spots
        
        // 1. Draw COMPLETED Cycles
        for (int c = 0; c < currentCycle; c++) {
            int startPixel = c * PIXELS_PER_CYCLE;
            for (int i = 0; i < PIXELS_PER_CYCLE; i++) setPixelLinear(startPixel + i, 1);
        }

        // 2. Draw CURRENT Cycle
        int currentStart = currentCycle * PIXELS_PER_CYCLE;
        
        if (!isBreak) {
            // Fill Work pixels up to current minute
            for (int i = 0; i < LEDS_PER_WORK; i++) {
                setPixelLinear(currentStart + i, (i < minute) ? 1 : 0);
            }
        } else {
            // Draw all Work pixels (Completed)
            for (int i = 0; i < LEDS_PER_WORK; i++) setPixelLinear(currentStart + i, 1);
            
            // Draw Break pixels
            int breakPixels = minute * 2; // Assuming 2 LEDs per break minute
            for (int i = 0; i < LEDS_PER_BREAK; i++) {
                setPixelLinear(currentStart + LEDS_PER_WORK + i, (i < breakPixels) ? 1 : 0);
            }
        }
    }
    
    void drawBlink(int minute) {
        // Draw the base first to avoid clearing everything
        drawProgress(minute);

        int currentStart = currentCycle * PIXELS_PER_CYCLE;
        
        if (!isBreak) {
            // Blink the specific pixel for the current minute
            setPixelLinear(currentStart + minute, 1);
        } else {
            // Blink the break segment
            int breakPixels = minute * 2;
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels, 1);
            setPixelLinear(currentStart + LEDS_PER_WORK + breakPixels + 1, 1);
        }
    }

    void setPixelLinear(int index, uint8_t color) {
        if (index >= (MATRIX_WIDTH * MATRIX_HEIGHT)) return;
        
        // Map linear index to your Snake/Spiral layout
        int y = index / 10; 
        int x = index % 10; 
        
        // Orientation adjustment
        y = 15 - y;

        if(x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
        
        // Use Global canvas
        canvas.drawPixel(x, y, color);
    }
    
    void clearDisplay() {
        canvas.fillScreen(0);
    }

    // --- SAFE SOUND FUNCTIONS (Replaces tone()) ---
    
    void playTone(int freq, int durationMs) {
        // 1. Set Frequency
        ledcWriteTone(BUZZER_CHANNEL, freq);
        
        // 2. Set Volume (Duty Cycle 50%)
        ledcWrite(BUZZER_CHANNEL, 128); 
        
        // 3. Wait (Blocking is okay here for short beeps)
        delay(durationMs);
        
        // 4. Stop Sound
        ledcWrite(BUZZER_CHANNEL, 0);
    }

    void beepWorkFinished() {
        playTone(1000, 200); 
        delay(250);
        playTone(1000, 200); 
    }

    void playBackToWorkMelody() {
        playTone(600, 150); 
        delay(150);
        playTone(1200, 300); 
    }

    void playVictoryMelody() {
        int melody[] = {523, 659, 784, 1046, 784, 1046}; 
        int duration[] = {150, 150, 150, 400, 150, 600};
        for (int i = 0; i < 6; i++) {
            playTone(melody[i], duration[i]);
            delay(50); // Small gap between notes
        }
    }
};