#pragma once
#include "Mode.h"
#include "Globals.h"

// Standard Tetromino Shapes
const uint16_t SHAPES[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // I
    {0xCC00, 0xCC00, 0xCC00, 0xCC00}, // O
    {0x44C0, 0x8E00, 0xC880, 0xE200}, // J
    {0x22C0, 0xE800, 0xC440, 0x2E00}, // L
    {0x6C00, 0x4620, 0x06C0, 0x8C40}, // S
    {0x4E00, 0x4640, 0x0E40, 0x4C40}, // T
    {0xC600, 0x2640, 0x0C60, 0x4C80}  // Z
};

class ModeTetris : public Mode {
    uint16_t board[16]; // Stores the static pile (bitmask)
    int px, py, pRot, pType;
    unsigned long lastFall, lastMove, lastRot;
    bool gameOver;

public:
    const char* getName() override { return "Tetris Gyro"; }

    void setup() override {
        memset(board, 0, sizeof(board));
        spawn();
        gameOver = false;
        lastFall = millis();
    }

    void spawn() {
        pType = random(7);
        pRot = 0;
        px = 3; py = -3; // Start high
    }

    bool check(int x, int y, int rot) {
        uint16_t bitShape = SHAPES[pType][rot];
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (bitShape & (0x8000 >> (r * 4 + c))) { // Test bit
                    int wx = x + c;
                    int wy = y + r;
                    if (wx < 0 || wx > 9 || wy > 15) return true; // Walls/Floor
                    if (wy >= 0 && (board[wy] & (1 << wx))) return true; // Collision with pile
                }
            }
        }
        return false;
    }

    void lock() {
        uint16_t bitShape = SHAPES[pType][pRot];
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (bitShape & (0x8000 >> (r * 4 + c))) {
                    int wy = py + r;
                    int wx = px + c;
                    if (wy >= 0 && wy < 16) board[wy] |= (1 << wx);
                }
            }
        }
        
        // Line Clear
        for (int y = 0; y < 16; y++) {
            if (board[y] == 0x3FF) { // 1111111111 (Binary for 10 ones)
                // Shift down
                for (int k = y; k > 0; k--) board[k] = board[k-1];
                board[0] = 0;
            }
        }
        spawn();
        if (check(px, py, pRot)) { gameOver = true; memset(board, 0, sizeof(board)); }
    }

    void loop() override {
        if (gameOver) { setup(); return; }

        // INPUT: Move (Tilt Y)
        if (millis() - lastMove > 100) {
            int dir = 0;
            if (accX < -3000) dir = -1;
            if (accX > 3000) dir = 1;
            if (dir != 0 && !check(px + dir, py, pRot)) px += dir;
            lastMove = millis();
        }

        // INPUT: Rotate (Tilt X Backwards)
        if (millis() - lastRot > 400 && accX > 4000) {
            int nRot = (pRot + 1) % 4;
            if (!check(px, py, nRot)) pRot = nRot;
            lastRot = millis();
        }

        // GRAVITY
        if (millis() - lastFall > 500) {
            if (!check(px, py + 1, pRot)) py++;
            else lock();
            lastFall = millis();
        }

        // RENDER
        clearDisplay();
        // Draw Board
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                if (board[y] & (1 << x)) setPixel(x, y, 1);
            }
        }
        // Draw Piece
        uint16_t bitShape = SHAPES[pType][pRot];
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (bitShape & (0x8000 >> (r * 4 + c))) {
                    setPixel(px + c, py + r, 1);
                }
            }
        }
    }
};