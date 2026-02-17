#ifndef _RESOURCEMONITOR_H
#define _RESOURCEMONITOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>

class ResourceMonitor {
public:
    ResourceMonitor();
    ~ResourceMonitor();
    
    void begin();
    void setPort(uint16_t port);
    void setSamplingInterval(unsigned long interval);

private:
    uint16_t port = 8080;
    unsigned long samplingInterval = 1000;
    AsyncWebServer* server = nullptr;
    AsyncEventSource* events = nullptr;
    
    // Resource Variables
    uint32_t freeHeap = 0;
    uint32_t freePSRAM = 0;
    uint32_t psramSize = 0;
    
    // CPU Calculation Variables
    uint64_t lastCpuCycles = 0;
    unsigned long lastCpuTime = 0;
    float cpuUsage = 0.0f;

    // Internal methods
    void startAP();
    void setupWebServer();
    void monitorTask();
    static void monitorTaskStatic(void* pvParameter);
    
    void sendESPInfo(AsyncWebServerRequest* request);
    void sendResourceStats(AsyncEventSourceClient* client);
    bool checkResources();
    String formatBytes(size_t bytes);
    String formatPercentage(float percent);
};

#endif