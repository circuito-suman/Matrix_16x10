#pragma once
#include "Mode.h"
#include "Globals.h"
class ModeSnake : public Mode {
    int sx[64], sy[64], len, dir, foodX, foodY;
    unsigned long lastMove;
public:
    const char* getName() override { return "Snake"; }
    void setup() override {
        len=4; sx[0]=5; sy[0]=8; dir=0; foodX=3; foodY=3;
        lastMove=millis();
    }
    void loop() override {
        // Read Input CONSTANTLY (No timer here, so it feels responsive)
        if(accX > 3500) dir=0; else if(accX < -3500) dir=1; 
        else if(accY > 3500) dir=2; else if(accY < -3500) dir=3; 

        // Update Game Logic on Timer
        if (millis() - lastMove > 150) {
            lastMove = millis();
            clearDisplay();
            for (int i=len-1; i>0; i--) { sx[i]=sx[i-1]; sy[i]=sy[i-1]; }
            
            if(dir==0) sx[0]++; if(dir==1) sx[0]--;
            if(dir==2) sy[0]++; if(dir==3) sy[0]--;
            
            if(sx[0]>9) sx[0]=0; if(sx[0]<0) sx[0]=9;
            if(sy[0]>15) sy[0]=0; if(sy[0]<0) sy[0]=15;

            if (sx[0] == foodX && sy[0] == foodY) {
                len++; foodX = random(10); foodY = random(16);
            }
            setPixel(foodX, foodY, 1); 
            for (int i=0; i<len; i++) setPixel(sx[i], sy[i], 1); 
        }
    }
};