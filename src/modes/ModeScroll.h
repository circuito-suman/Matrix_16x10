#pragma once
#include "Mode.h"
#include "Globals.h"
#include "Fonts.h"
// Directions
enum ScrollDir { SCROLL_LEFT, SCROLL_RIGHT, SCROLL_UP, SCROLL_DOWN };

class ModeScroll : public Mode {
    // State Variables
    int offset;
    unsigned long lastUpdate = 0;       // For scroll speed
    unsigned long tiltStartTime = 0;    // For 2-second hold timer
    
    ScrollDir currentDir = SCROLL_LEFT; // Default Direction
    ScrollDir pendingDir = SCROLL_LEFT; // Direction currently being held

    // Text Configuration
    const char* message = "circuito_suman"; 
    int msgLen;
    int totalLengthPixels;

public:
    const char* getName() override { return "4-Way Scroller"; }
    
    void setup() override { 
        msgLen = strlen(message);
        totalLengthPixels = msgLen * 6; // 6 pixels per char (5 width + 1 space)
        resetOffset();
        lastUpdate = millis();
        tiltStartTime = millis();
    }

    void resetOffset() {
        // Set starting position based on direction
        if (currentDir == SCROLL_LEFT || currentDir == SCROLL_UP) {
            offset = -16; // Start just off-screen (beginning)
        } else {
            offset = totalLengthPixels; // Start far off-screen (end)
        }
    }
    
    void loop() override {
        handleInput();
        
        // Speed Control (Runs every 80ms)
        if (millis() - lastUpdate < 80) return;
        lastUpdate = millis();

        updatePosition();
        render();
    }

private:
    // --- 1. INPUT LOGIC (2-Second Hold) ---
    void handleInput() {
        ScrollDir detectedDir = currentDir; // Default to no change

        // Detect Tilt (Threshold ~3000)
        if (accY < -3000) detectedDir = SCROLL_LEFT;
        else if (accY > 3000) detectedDir = SCROLL_RIGHT;
        else if (accX < -3000) detectedDir = SCROLL_UP;
        else if (accX > 3000) detectedDir = SCROLL_DOWN;

        // Timer Logic
        if (detectedDir != currentDir) {
            if (detectedDir == pendingDir) {
                // User is holding the new direction
                if (millis() - tiltStartTime > 2000) {
                    // 2 Seconds passed! Switch direction.
                    currentDir = detectedDir;
                    resetOffset();
                    
                    // visual feedback (optional flash)
                    clearDisplay();
                    delay(200); 
                }
            } else {
                // New direction detected, start timer
                pendingDir = detectedDir;
                tiltStartTime = millis();
            }
        } else {
            // User went back to neutral or current direction
            pendingDir = currentDir;
            tiltStartTime = millis(); 
        }
    }

    // --- 2. POSITION UPDATE LOGIC ---
    void updatePosition() {
        // Forward Scroll (Left or Up) -> Increment offset
        if (currentDir == SCROLL_LEFT || currentDir == SCROLL_UP) {
            offset++;
            if (offset > totalLengthPixels + 16) offset = -16;
        } 
        // Backward Scroll (Right or Down) -> Decrement offset
        else {
            offset--;
            if (offset < -16) offset = totalLengthPixels;
        }
    }

    // --- 3. RENDER LOGIC ---
    void render() {
        clearDisplay();

        for (int charIdx = 0; charIdx < msgLen; charIdx++) {
            char c = message[charIdx];
            int fontIdx = c - 32; // Map ASCII to font array

            // Calculate 'Virtual' position in the text string
            int textPos = (charIdx * 6) - offset;
            
            // Optimization: Skip characters definitely off-screen
            if (textPos < -6 || textPos > 16) continue;

            // Loop through columns of the character
            for (int col = 0; col < 5; col++) {
                byte colData = font5x7[fontIdx][col];
                
                // Draw based on Orientation
                if (currentDir == SCROLL_LEFT || currentDir == SCROLL_RIGHT) {
                    // --- HORIZONTAL MODE (Along Y-Axis of Matrix) ---
                    // Text runs along the 16-pixel length.
                    // X-axis (0-9) is the height of the letters.
                    
                    int screenY = textPos + col; // The scrolling axis
                    if (screenY >= 0 && screenY < 16) {
                        for (int r = 0; r < 8; r++) {
                            if ((colData >> r) & 1) {
                                // Center vertically on X-axis (Width 10, Font 8 -> Start at 1)
                                setPixel(8 - r, screenY, 1); 
                            }
                        }
                    }
                } 
                else {
                    // --- VERTICAL MODE (Along X-Axis of Matrix) ---
                    // Text runs along the 10-pixel width.
                    // Y-axis (0-15) is the height of the letters.
                    // effectively rotating text 90 degrees to fit.

                    int screenX = textPos + col; // The scrolling axis
                    if (screenX >= 0 && screenX < 10) {
                        for (int r = 0; r < 8; r++) {
                            if ((colData >> r) & 1) {
                                // Center vertically on Y-axis (Height 16, Font 8 -> Start at 4)
                                setPixel(screenX, 4 + r, 1);
                            }
                        }
                    }
                }
            }
        }
    }
};