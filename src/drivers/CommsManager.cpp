#include "CommsManager.h"
#include "Globals.h"
#include "Config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>

// BLE Callbacks
class ModeCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        std::string val = pCharacteristic->getValue();
        if(val.length() > 0) {
            requestedModeIndex = val[0]; // First byte is mode index
            modeChangeRequest = true;
        }
    }
};

class CanvasCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        std::string val = pCharacteristic->getValue();
        if(val.length() > 0) {
            // DIRECTLY WRITE TO CANVAS BUFFER
            // Thread-safe lock
            if (xSemaphoreTake(dispMutex, 10) == pdTRUE) {
                // Ensure we don't overflow buffer
                int len = val.length();
                if (len > 20) len = 20; // 10x16 bits is small, adjust logic as needed
                
                // For simplicity, assuming val is raw bitmap data
                uint8_t* buffer = canvas.getBuffer();
                memcpy(buffer, val.data(), len);
                
                xSemaphoreGive(dispMutex);
            }
        }
    }
};

void setupComms() {
    // 1. WiFi AP & OTA
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    
    ArduinoOTA.onStart([]() {
        // Optional: Stop display timer to prevent crashes during write
    });
    ArduinoOTA.begin();

    // 2. BLE Setup (NimBLE)
    NimBLEDevice::init("Matrix_ESP32");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    NimBLECharacteristic *pModeChar = pService->createCharacteristic(
        CHAR_MODE_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pModeChar->setCallbacks(new ModeCallbacks());

    NimBLECharacteristic *pCanvasChar = pService->createCharacteristic(
        CHAR_CANVAS_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pCanvasChar->setCallbacks(new CanvasCallbacks());

    pService->start();
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

void handleComms() {
    ArduinoOTA.handle();
    // BLE is interrupt/event driven, no loop needed
}