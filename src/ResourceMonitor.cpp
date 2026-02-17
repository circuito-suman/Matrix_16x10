#include "ResourceMonitor.h"

ResourceMonitor::ResourceMonitor() {}

ResourceMonitor::~ResourceMonitor() {
    if (events) delete events;
    if (server) {
        server->end();
        delete server;
    }
}

void ResourceMonitor::begin() {
    Serial.printf("ResourceMonitor >> Starting AP mode monitoring...\n");
    
    startAP();
    setupWebServer();
    
    // Start monitoring task on Core 1 (App Core)
    xTaskCreatePinnedToCore(
        monitorTaskStatic,   // Function
        "Resource Monitor",  // Name
        4096,                // Stack size
        this,                // Parameter
        1,                   // Priority
        nullptr,             // Handle
        0                    // Core ID
    );
    
    Serial.printf("ResourceMonitor >> Web server running at http://%s:%u\n", 
                  WiFi.softAPIP().toString().c_str(), port);
}

void ResourceMonitor::setPort(uint16_t p) {
    port = p;
}

void ResourceMonitor::setSamplingInterval(unsigned long interval) {
    samplingInterval = interval;
}

void ResourceMonitor::startAP() {
    WiFi.mode(WIFI_AP);
    // You can change the SSID and Password here
    WiFi.softAP("Matrix", "12345678");
    
    Serial.printf("ResourceMonitor >> AP started: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Print basic chip info
    Serial.printf("ResourceMonitor >> Chip: %s Rev:%d Cores:%d Freq:%dMHz\n",
                  ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getChipCores(), ESP.getCpuFreqMHz());
                  
    if (psramFound()) {
        psramSize = ESP.getPsramSize();
        Serial.printf("ResourceMonitor >> PSRAM: %s\n", formatBytes(psramSize).c_str());
    }
}

void ResourceMonitor::setupWebServer() {
    server = new AsyncWebServer(port);
    
    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    
    // SSE endpoint for real-time updates
    events = new AsyncEventSource("/events");
    server->addHandler(events);
    
    // API endpoint for static info
    server->on("/espinfo", HTTP_GET, [this](AsyncWebServerRequest* request) {
        sendESPInfo(request);
    });
    
    // Main Dashboard HTML
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Resource Monitor</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
  body{font-family:Arial,sans-serif;padding:20px;background:#1a1a1a;color:#fff;}
  h1{text-align:center;font-size:24px;margin-bottom:20px;}
  .meter{display:block;height:20px;background:#333;border-radius:10px;overflow:hidden;margin-top:5px;}
  .fill{width:0%;height:100%;background:linear-gradient(90deg,#4caf50,#2196f3);transition:width 0.5s ease-out;}
  .card{background:#2d2d2d;padding:15px;margin-bottom:15px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}
  .data-row{display:flex;justify-content:space-between;font-size:14px;margin-bottom:5px;}
  .label{color:#aaa;}
  .value{font-weight:bold;}
</style>
</head>
<body>
  <h1>ESP32 Monitor</h1>
  <div id='heap' class='card'></div>
  <div id='psram' class='card'></div>
  <div id='cpu' class='card'></div>
  <div id='uptime' class='card' style='text-align:center;color:#888;font-size:12px;'>Waiting for data...</div>

<script>
const events = new EventSource('/events');
events.onmessage = function(event) {
    const data = JSON.parse(event.data);
    
    // Update Heap
    let heapPct = (data.free_heap / data.heap_size) * 100;
    updateCard('heap', 'Free Heap', formatBytes(data.free_heap), heapPct);

    // Update PSRAM (if available)
    if(data.psram_size > 0) {
        let psramPct = (data.free_psram / data.psram_size) * 100;
        updateCard('psram', 'Free PSRAM', formatBytes(data.free_psram), psramPct);
    } else {
        document.getElementById('psram').style.display = 'none';
    }

    // Update CPU
    // Note: CPU usage on ESP32 is tricky to measure accurately without hooks.
    // This value represents cycle consumption vs wall clock.
    document.getElementById('cpu').innerHTML = 
        `<div class='data-row'><span class='label'>CPU Usage</span><span class='value'>${data.cpu_usage}%</span></div>
         <div class='meter'><div class='fill' style='width:${data.cpu_usage}%'></div></div>
         <div class='data-row' style='margin-top:5px;font-size:12px;'><span class='label'>Freq</span><span>${data.cpu_freq} MHz</span></div>`;

    // Update Uptime
    document.getElementById('uptime').innerText = 'Uptime: ' + (data.uptime / 1000).toFixed(1) + 's';
};

function updateCard(id, title, valueStr, percent) {
    const el = document.getElementById(id);
    el.innerHTML = `<div class='data-row'><span class='label'>${title}</span><span class='value'>${valueStr}</span></div>
                    <div class='meter'><div class='fill' style='width:${percent}%'></div></div>`;
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}
</script>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    server->begin();
}

void ResourceMonitor::monitorTaskStatic(void* pvParameter) {
    static_cast<ResourceMonitor*>(pvParameter)->monitorTask();
}

void ResourceMonitor::monitorTask() {
    // Initialize timing variables
    lastCpuCycles = ESP.getCycleCount();
    lastCpuTime = millis();
    
    unsigned long lastActivity = millis();
    const unsigned long activityTimeout = 5000;
    
    while (true) {
        bool hasChanges = checkResources();
        
        // Push updates if data changed OR if it's been a while (keepalive)
        if (hasChanges || (millis() - lastActivity > activityTimeout)) {
            sendResourceStats(nullptr); // Broadcast to all
            lastActivity = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(samplingInterval));
    }
}

bool ResourceMonitor::checkResources() {
    bool changed = false;
    
    // Check Heap
    uint32_t newHeap = ESP.getFreeHeap();
    if (newHeap != freeHeap) {
        freeHeap = newHeap;
        changed = true;
    }
    
    // Check PSRAM
    if (psramFound()) {
        uint32_t newPSRAM = ESP.getFreePsram();
        if (newPSRAM != freePSRAM) {
            freePSRAM = newPSRAM;
            changed = true;
        }
    }
    
    // Calculate CPU Usage
    // Note: This calculates 'busy cycles' roughly. 
    // Ideally, FreeRTOS idle hooks are needed for precise 'load' %, 
    // but this gives a general indication of CPU cycle consumption.
    uint64_t currentCycles = ESP.getCycleCount();
    unsigned long currentTime = millis();
    
    if (currentTime > lastCpuTime) {
        uint64_t cyclesDiff = currentCycles - lastCpuCycles;
        uint64_t timeDiffUs = (currentTime - lastCpuTime) * 1000ULL;
        float cyclesPerSec = (ESP.getCpuFreqMHz() * 1000000.0f);
        
        // Avoid division by zero
        if (timeDiffUs > 0) {
            float calculatedUsage = (cyclesDiff / (float)timeDiffUs) / (cyclesPerSec / 1000000.0f) * 100.0f;
            // Normalize (simple clamp)
            if (calculatedUsage > 100.0f) calculatedUsage = 100.0f;
            if (calculatedUsage < 0.0f) calculatedUsage = 0.0f;
            
            // Only update if change is significant (> 1%)
            if (abs(calculatedUsage - cpuUsage) > 1.0f) {
                cpuUsage = calculatedUsage;
                changed = true;
            }
        }
    }
    
    lastCpuCycles = currentCycles;
    lastCpuTime = currentTime;
    
    return changed;
}

void ResourceMonitor::sendResourceStats(AsyncEventSourceClient* client) {
    String json = "{";
    json += "\"free_heap\":" + String(freeHeap) + ",";
    json += "\"heap_size\":" + String(ESP.getHeapSize()) + ",";
    json += "\"free_psram\":" + String(freePSRAM) + ",";
    json += "\"psram_size\":" + String(psramSize) + ",";
    json += "\"cpu_usage\":" + String(cpuUsage, 1) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"uptime\":" + String(millis());
    json += "}";
    
    if (client) {
        client->send(json.c_str());
    } else {
        events->send(json.c_str(), "message", millis());
    }
}

void ResourceMonitor::sendESPInfo(AsyncWebServerRequest* request) {
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    
    String json = "{";
    json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cores\":" + String(ESP.getChipCores()) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"heap_size\":" + String(ESP.getHeapSize()) + ",";
    json += "\"psram_size\":" + String(psramSize);
    json += "}";
    
    request->send(200, "application/json", json);
}

String ResourceMonitor::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else return String(bytes / 1024.0 / 1024.0, 1) + " MB";
}

String ResourceMonitor::formatPercentage(float percent) {
    return String(percent, 1) + "%";
}