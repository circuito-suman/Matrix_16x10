#pragma once
#include "Mode.h"
#include "Globals.h"
#include <math.h>

// 3D Wireframe Cube
class ModeCube : public Mode {
    float angleX = 0;
    float angleY = 0;
    float angleZ = 0;

    struct Point3D { float x, y, z; };
    
    // Cube Vertices (Center at 0,0,0)
    Point3D vertices[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
    };

    // Edges connecting vertices
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // Back Face
        {4,5}, {5,6}, {6,7}, {7,4}, // Front Face
        {0,4}, {1,5}, {2,6}, {3,7}  // Connecting Lines
    };

public:
    const char* getName() override { return "3D Cube"; }

    void setup() override {
        angleX = 0;
        angleY = 0;
        angleZ = 0;
    }

    void loop() override {
        clearDisplay();
        
        // Auto rotate
        angleX += 0.04;
        angleY += 0.06;
        angleZ += 0.02;

        float scale;
        // Pulse size slightly
        scale = 3.5 + sin(millis()/1000.0) * 0.5;

        Point3D projected[8]; 
        
        // Precompute sin/cos for speed if needed, but float is fast enough on ESP32
        float cX = cos(angleX); float sX = sin(angleX);
        float cY = cos(angleY); float sY = sin(angleY);
        float cZ = cos(angleZ); float sZ = sin(angleZ);

        for (int i = 0; i < 8; i++) {
            Point3D v = vertices[i];

            // 1. Rotate X
            float y1 = v.y * cX - v.z * sX;
            float z1 = v.y * sX + v.z * cX;
            v.y = y1; v.z = z1;

            // 2. Rotate Y
            float x2 = v.x * cY + v.z * sY;
            float z2 = -v.x * sY + v.z * cY;
            v.x = x2; v.z = z2;

            // 3. Rotate Z
            float x3 = v.x * cZ - v.y * sZ;
            float y3 = v.x * sZ + v.y * cZ;
            v.x = x3; v.y = y3;

            // 4. Project (Orthographic)
            projected[i].x = v.x;
            projected[i].y = v.y;
        }

        // 5. Draw Edges
        int centerX = MATRIX_WIDTH / 2;
        int centerY = MATRIX_HEIGHT / 2;

        for (int i = 0; i < 12; i++) {
            // Edges connect vertices by index
            // e.g. edge 0 connects vertex 0 and 1...
            // Wait, my edge defs use indices into vertices array. That's fine.
            int idx1 = -1, idx2 = -1;
            
            // Re-define edges properly just in case
            // Back face: 0-1, 1-2, 2-3, 3-0
            if(i<4) { idx1=i; idx2=(i+1)%4; }
            // Front face: 4-5, 5-6, 6-7, 7-4
            else if(i<8) { idx1=4+(i-4); idx2=4+((i-4)+1)%4; }
            // Connecting: 0-4, 1-5, 2-6, 3-7
            else { idx1=i-8; idx2=i-4; }
            
            int x1 = (int)(projected[idx1].x * scale) + centerX;
            int y1 = (int)(projected[idx1].y * scale) + centerY;
            int x2 = (int)(projected[idx2].x * scale) + centerX;
            int y2 = (int)(projected[idx2].y * scale) + centerY;

            canvas.drawLine(x1, y1, x2, y2, 1);
        }
        
        delay(30);
    }
};
