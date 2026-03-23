#include "CommsManager.h"
#include "Globals.h"
#include "Config.h"
#include "MatrixProtocolCodec.h"
#include "MatrixOtaUpdateHandler_ArduinoESP32.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>
#include <array>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

static constexpr uint8_t STATUS_OK = 0x00;
static constexpr uint8_t STATUS_BAD_FRAME_OR_CRC = 0x01;
static constexpr uint8_t STATUS_INVALID_STATE = 0x02;
static constexpr uint8_t STATUS_BAD_OFFSET = 0x03;
static constexpr uint8_t STATUS_FLASH_WRITE_ERROR = 0x04;
static constexpr uint8_t STATUS_FINALIZE_ERROR = 0x05;

enum class Source {
    Mode,
    Canvas,
    Text,
    Version,
    Ota,
    Control,
};

struct PendingWrite {
    Source source;
    std::vector<uint8_t> data;
};

struct OtaSession {
    bool active = false;
    bool legacy = false;
    uint32_t totalSize = 0;
    uint16_t chunkSize = 0;
    uint32_t nextOffset = 0;
    uint32_t bytesWritten = 0;
};

static NimBLEServer* gServer = nullptr;
static NimBLECharacteristic* gVersionChar = nullptr;
static NimBLECharacteristic* gAckChar = nullptr;
static SemaphoreHandle_t gQueueMutex = nullptr;
static std::deque<PendingWrite> gQueue;
static OtaSession gOta;
static MatrixOtaUpdateHandlerArduinoEsp32 gOtaHandler;
static bool gNeedsReboot = false;
static std::string gLastText;
static constexpr int APP_CONTROLLED_MODE_ID = 11;
static constexpr int SCROLL_MODE_ID = 8;
static constexpr const char *kFirmwareModesPipe =
    "Marble|Sparkle|Fluid|Heart|Life|Pong|Snake|Tetris|4-Way Scroller|Matrix|Pomodoro|App Controlled";

static const char* getFwVersion() {
    return FW_SEMVER;
}

static void queueWrite(Source source, const std::string& value) {
    if (value.empty() || gQueueMutex == nullptr) return;

    PendingWrite item;
    item.source = source;
    item.data.assign(value.begin(), value.end());

    if (xSemaphoreTake(gQueueMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (gQueue.size() < 64) {
            gQueue.push_back(std::move(item));
        }
        xSemaphoreGive(gQueueMutex);
    }
}

static void requestModeChange(int index) {
    requestedModeIndex = index;
    modeChangeRequest = true;
}

static void activateAppControlledMode() {
    if (activeModeIndex != APP_CONTROLLED_MODE_ID) {
        requestedModeIndex = APP_CONTROLLED_MODE_ID;
        modeChangeRequest = true;
    }
}

static void applyScrollText(const std::string &text) {
    if (xSemaphoreTake(dispMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        appScrollText = String(text.c_str());
        xSemaphoreGive(dispMutex);
    } else {
        appScrollText = String(text.c_str());
    }
}

static void applyScrollDirectionCommand(const std::string &command) {
    if (command == "DIR:AUTO") {
        appScrollDirectionOverride = false;
        Serial.println("[BLE] scroll direction override disabled (AUTO)");
        return;
    }

    int dir = -1;
    if (command == "DIR:LEFT") dir = 0;
    else if (command == "DIR:RIGHT") dir = 1;
    else if (command == "DIR:UP") dir = 2;
    else if (command == "DIR:DOWN") dir = 3;

    if (dir >= 0) {
        appScrollDirection = dir;
        appScrollDirectionOverride = true;
        Serial.printf("[BLE] scroll direction set to %d\n", dir);
    }
}


static void renderTextToCanvas(const std::string& text) {
    if (xSemaphoreTake(dispMutex, pdMS_TO_TICKS(20)) != pdTRUE) return;
    canvas.fillScreen(0);
    canvas.setTextSize(1);
    canvas.setTextWrap(false);
    canvas.setCursor(0, 4);
    canvas.print(text.c_str());
    xSemaphoreGive(dispMutex);
}

static bool applyCanvasPacked(const uint8_t* payload, size_t length) {
    if (payload == nullptr || length != 20) return false;
    if (xSemaphoreTake(dispMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;

    canvas.fillScreen(0);
    for (int y = 0; y < 16; ++y) {
        const int rowBase = y * 10;
        for (int x = 0; x < 10; ++x) {
            const int bitIndex = rowBase + x;
            const uint8_t byteValue = payload[bitIndex >> 3];
            const uint8_t bitMask = static_cast<uint8_t>(1u << (bitIndex & 7));
            if ((byteValue & bitMask) != 0) {
                canvas.drawPixel(x, y, 1);
            }
        }
    }

    xSemaphoreGive(dispMutex);
    return true;
}

static void notifyPacket(const std::vector<uint8_t>& packet) {
    if (gAckChar == nullptr || packet.empty()) return;
    gAckChar->setValue(packet.data(), packet.size());
    gAckChar->notify();
}

static void sendAckPacket(uint8_t seq, uint8_t status, bool nack) {
    const uint8_t payload[1] = { status };
    const uint8_t type = nack ? matrixproto::Nack : matrixproto::Ack;
    notifyPacket(matrixproto::buildPacket(type, seq, payload, 1));
    Serial.printf("[BLE] %s seq=%u status=0x%02X\n", nack ? "NACK" : "ACK", seq, status);
}

static void sendVersionResponse(uint8_t seq) {
    const char* version = getFwVersion();
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(version);
    notifyPacket(matrixproto::buildPacket(matrixproto::VersionResponse, seq, ptr, strlen(version)));
}

static void sendModesResponse(uint8_t seq) {
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(kFirmwareModesPipe);
    notifyPacket(matrixproto::buildPacket(matrixproto::ModesResponse, seq, ptr, strlen(kFirmwareModesPipe)));
}

static bool otaBegin(uint32_t totalSize, uint16_t chunkSize) {
    if (gOta.active) {
        Serial.println("[OTA] begin rejected: session already active");
        return false;
    }
    if (totalSize == 0) {
        Serial.println("[OTA] begin rejected: total size is zero");
        return false;
    }

    if (!gOtaHandler.begin(totalSize, chunkSize)) {
        Serial.println("[OTA] begin failed");
        return false;
    }

    gOta.active = true;
    gOta.legacy = false;
    gOta.totalSize = totalSize;
    gOta.chunkSize = chunkSize;
    gOta.nextOffset = 0;
    gOta.bytesWritten = 0;
    Serial.printf("[OTA] begin total=%lu chunk=%u\n", (unsigned long)totalSize, chunkSize);
    return true;
}

static bool otaWriteChunk(uint32_t offset, const uint8_t* data, size_t length, uint8_t& status) {
    if (!gOta.active) {
        status = STATUS_INVALID_STATE;
        return false;
    }
    if (offset != gOta.nextOffset) {
        status = STATUS_BAD_OFFSET;
        Serial.printf("[OTA] bad offset expected=%lu got=%lu\n",
                      (unsigned long)gOta.nextOffset,
                      (unsigned long)offset);
        return false;
    }

    if (!gOtaHandler.writeChunk(offset, data, length)) {
        status = STATUS_FLASH_WRITE_ERROR;
        Serial.printf("[OTA] write failed off=%lu len=%u\n",
                      (unsigned long)offset,
                      (unsigned)length);
        return false;
    }

    gOta.nextOffset += static_cast<uint32_t>(length);
    gOta.bytesWritten += static_cast<uint32_t>(length);
    status = STATUS_OK;
    Serial.printf("[OTA] chunk off=%lu len=%u total=%lu/%lu\n",
                  (unsigned long)offset,
                  (unsigned)length,
                  (unsigned long)gOta.bytesWritten,
                  (unsigned long)gOta.totalSize);
    return true;
}

static bool otaEnd(uint8_t& status) {
    if (!gOta.active) {
        status = STATUS_INVALID_STATE;
        return false;
    }

    if (!gOtaHandler.end(false)) {
        status = STATUS_FINALIZE_ERROR;
        Serial.println("[OTA] finalize failed");
        gOta = OtaSession();
        return false;
    }

    status = STATUS_OK;
    Serial.printf("[OTA] finalize ok bytes=%lu\n", (unsigned long)gOta.bytesWritten);
    gOta = OtaSession();
    gNeedsReboot = true;
    return true;
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        Serial.println("[BLE] client connected");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        Serial.println("[BLE] client disconnected");
        NimBLEDevice::startAdvertising();
    }
};

class GenericCallbacks: public NimBLECharacteristicCallbacks {
public:
    explicit GenericCallbacks(Source source) : source_(source) {}

    void onWrite(NimBLECharacteristic *pCharacteristic) override {
        queueWrite(source_, pCharacteristic->getValue());
    }

private:
    Source source_;
};

static GenericCallbacks* gModeCb = nullptr;
static GenericCallbacks* gCanvasCb = nullptr;
static GenericCallbacks* gTextCb = nullptr;
static GenericCallbacks* gVersionCb = nullptr;
static GenericCallbacks* gOtaCb = nullptr;
static GenericCallbacks* gControlCb = nullptr;
static ServerCallbacks* gServerCb = nullptr;

static void handleLegacyWrite(Source source, const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    switch (source) {
        case Source::Mode:
            requestModeChange(static_cast<int>(data[0]));
            Serial.printf("[BLE] legacy mode=%u\n", data[0]);
            break;
        case Source::Text: {
            std::string text(data.begin(), data.end());
            gLastText = text;
            if (activeModeIndex == SCROLL_MODE_ID) {
                applyScrollText(text);
                Serial.printf("[BLE] scroll text len=%u\n", (unsigned)data.size());
            } else {
                activateAppControlledMode();
                renderTextToCanvas(text);
                Serial.printf("[BLE] legacy text len=%u\n", (unsigned)data.size());
            }
            break;
        }
        case Source::Canvas:
            if (data.size() == 20) {
                activateAppControlledMode();
                applyCanvasPacked(data.data(), data.size());
                Serial.println("[BLE] legacy canvas updated");
            }
            break;
        case Source::Version: {
            std::string req(data.begin(), data.end());
            if (req == "GET_VERSION" && gVersionChar != nullptr) {
                gVersionChar->setValue(getFwVersion());
                gVersionChar->notify();
            }
            break;
        }
        case Source::Control: {
            std::string cmd(data.begin(), data.end());
            if (cmd == "NEXT") requestModeChange(activeModeIndex + 1);
            else if (cmd == "PREV") requestModeChange(activeModeIndex - 1);
            else if (cmd == "RESET") requestModeChange(0);
            else if (cmd == "SPECIAL") requestModeChange(10);
            else if (cmd == "GET_MODES") {
                if (gVersionChar != nullptr) {
                    std::string payload = std::string("MODES:") + kFirmwareModesPipe;
                    gVersionChar->setValue(payload);
                    gVersionChar->notify();
                }
            }
            else if (cmd.rfind("DIR:", 0) == 0) {
                applyScrollDirectionCommand(cmd);
            }
            else if (cmd.rfind("SCROLLTXT:", 0) == 0) {
                applyScrollText(cmd.substr(10));
                if (activeModeIndex != SCROLL_MODE_ID) {
                    requestModeChange(SCROLL_MODE_ID);
                }
            }
            else if (cmd == "OTA_END") {
                uint8_t status = STATUS_OK;
                otaEnd(status);
            }
            break;
        }
        case Source::Ota:
            if (!gOtaHandler.active()) {
                Serial.println("[OTA] legacy chunk rejected: inactive session");
                return;
            }
            if (!gOtaHandler.writeLegacyChunk(data.data(), data.size())) {
                Serial.printf("[OTA] legacy chunk write failed len=%u\n", (unsigned)data.size());
                return;
            }
            gOta.bytesWritten += static_cast<uint32_t>(data.size());
            break;
    }
}

static void handleProtocolFrame(const std::vector<uint8_t>& frame) {
    if (frame.size() < 8) return;

    const uint8_t seq = frame[3];
    matrixproto::Packet packet;
    if (!matrixproto::parsePacket(frame.data(), frame.size(), packet)) {
        sendAckPacket(seq, STATUS_BAD_FRAME_OR_CRC, true);
        return;
    }

    Serial.printf("[BLE] protocol type=0x%02X seq=%u len=%u\n",
                  packet.type,
                  packet.seq,
                  (unsigned)packet.payload.size());

    switch (packet.type) {
        case matrixproto::SetMode:
            if (packet.payload.size() != 1) {
                sendAckPacket(packet.seq, STATUS_BAD_FRAME_OR_CRC, true);
                return;
            }
            requestModeChange(static_cast<int>(packet.payload[0]));
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;

        case matrixproto::SetText: {
            std::string text(packet.payload.begin(), packet.payload.end());
            gLastText = text;
            if (activeModeIndex == SCROLL_MODE_ID) {
                applyScrollText(text);
            } else {
                activateAppControlledMode();
                renderTextToCanvas(text);
            }
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;
        }

        case matrixproto::GetVersion:
            sendVersionResponse(packet.seq);
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;

        case matrixproto::GetModes:
            sendModesResponse(packet.seq);
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;

        case matrixproto::SetCanvas:
            if (packet.payload.size() != 20 || !applyCanvasPacked(packet.payload.data(), packet.payload.size())) {
                sendAckPacket(packet.seq, STATUS_BAD_FRAME_OR_CRC, true);
                return;
            }
            activateAppControlledMode();
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;

            return;

        case matrixproto::OtaBegin: {
            if (packet.payload.size() != 6) {
                sendAckPacket(packet.seq, STATUS_BAD_FRAME_OR_CRC, true);
                return;
            }
            const uint32_t totalSize = static_cast<uint32_t>(packet.payload[0])
                                     | (static_cast<uint32_t>(packet.payload[1]) << 8)
                                     | (static_cast<uint32_t>(packet.payload[2]) << 16)
                                     | (static_cast<uint32_t>(packet.payload[3]) << 24);
            const uint16_t chunkSize = static_cast<uint16_t>(packet.payload[4])
                                     | (static_cast<uint16_t>(packet.payload[5]) << 8);
            if (!otaBegin(totalSize, chunkSize)) {
                sendAckPacket(packet.seq, STATUS_INVALID_STATE, true);
                return;
            }
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;
        }

        case matrixproto::OtaChunk: {
            if (packet.payload.size() < 4) {
                sendAckPacket(packet.seq, STATUS_BAD_FRAME_OR_CRC, true);
                return;
            }
            const uint32_t offset = static_cast<uint32_t>(packet.payload[0])
                                  | (static_cast<uint32_t>(packet.payload[1]) << 8)
                                  | (static_cast<uint32_t>(packet.payload[2]) << 16)
                                  | (static_cast<uint32_t>(packet.payload[3]) << 24);
            const uint8_t* chunk = packet.payload.data() + 4;
            const size_t chunkLen = packet.payload.size() - 4;

            uint8_t status = STATUS_OK;
            if (!otaWriteChunk(offset, chunk, chunkLen, status)) {
                sendAckPacket(packet.seq, status, true);
                return;
            }
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;
        }

        case matrixproto::OtaEnd: {
            uint8_t status = STATUS_OK;
            if (!otaEnd(status)) {
                sendAckPacket(packet.seq, status, true);
                return;
            }
            sendAckPacket(packet.seq, STATUS_OK, false);
            return;
        }

        default:
            sendAckPacket(packet.seq, STATUS_BAD_FRAME_OR_CRC, true);
            return;
    }
}

static void processPendingWrite(const PendingWrite& item) {
    const bool looksFramed = item.data.size() >= 2
                          && item.data[0] == matrixproto::kPacketMagic
                          && item.data[1] == matrixproto::kPacketVersion;

    if ((item.source == Source::Control || item.source == Source::Ota) && looksFramed) {
        handleProtocolFrame(item.data);
        return;
    }

    handleLegacyWrite(item.source, item.data);
}

} // namespace

void setupComms() {
    if (gQueueMutex == nullptr) {
        gQueueMutex = xSemaphoreCreateMutex();
    }

    // WiFi AP + classic ArduinoOTA support
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    Serial.printf("[COMMS] WiFi AP started SSID=%s\n", WIFI_SSID);
    
    ArduinoOTA.onStart([]() { Serial.println("[OTA] ArduinoOTA start"); });
    ArduinoOTA.onEnd([]() { Serial.println("[OTA] ArduinoOTA end"); });
    ArduinoOTA.onError([](ota_error_t err) { Serial.printf("[OTA] ArduinoOTA error=%u\n", err); });
    ArduinoOTA.begin();

    // BLE Setup (exact app contract)
    NimBLEDevice::init("Matrix_16x10");
    NimBLEDevice::setMTU(247);
    NimBLEDevice::setSecurityAuth(false, false, false);

    gServer = NimBLEDevice::createServer();
    gServerCb = new ServerCallbacks();
    gServer->setCallbacks(gServerCb);

    NimBLEService *pService = gServer->createService(SERVICE_UUID);

    NimBLECharacteristic *pModeChar = pService->createCharacteristic(
        CHAR_MODE_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    gModeCb = new GenericCallbacks(Source::Mode);
    pModeChar->setCallbacks(gModeCb);

    NimBLECharacteristic *pCanvasChar = pService->createCharacteristic(
        CHAR_CANVAS_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    gCanvasCb = new GenericCallbacks(Source::Canvas);
    pCanvasChar->setCallbacks(gCanvasCb);

    NimBLECharacteristic *pTextChar = pService->createCharacteristic(
        CHAR_TEXT_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    gTextCb = new GenericCallbacks(Source::Text);
    pTextChar->setCallbacks(gTextCb);

    gVersionChar = pService->createCharacteristic(
        CHAR_VERSION_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
    );
    gVersionCb = new GenericCallbacks(Source::Version);
    gVersionChar->setCallbacks(gVersionCb);
    gVersionChar->setValue(getFwVersion());

    NimBLECharacteristic *pOtaChar = pService->createCharacteristic(
        CHAR_OTA_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    gOtaCb = new GenericCallbacks(Source::Ota);
    pOtaChar->setCallbacks(gOtaCb);

    NimBLECharacteristic *pControlChar = pService->createCharacteristic(
        CHAR_CONTROL_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    gControlCb = new GenericCallbacks(Source::Control);
    pControlChar->setCallbacks(gControlCb);

    gAckChar = pService->createCharacteristic(
        CHAR_ACK_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE
    );
    gAckChar->setValue("0");

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->addServiceUUID(static_cast<uint16_t>(0xABCD));
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    const bool advStarted = pAdvertising->start();

    Serial.printf("[BLE] init service=%s mode=%s canvas=%s\n", SERVICE_UUID, CHAR_MODE_UUID, CHAR_CANVAS_UUID);
    Serial.printf("[BLE] advertising %s\n", advStarted ? "started" : "failed");
}

void handleComms() {
    ArduinoOTA.handle();

    while (true) {
        PendingWrite item;
        bool hasItem = false;

        if (gQueueMutex != nullptr && xSemaphoreTake(gQueueMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            if (!gQueue.empty()) {
                item = std::move(gQueue.front());
                gQueue.pop_front();
                hasItem = true;
            }
            xSemaphoreGive(gQueueMutex);
        }

        if (!hasItem) break;
        processPendingWrite(item);
    }

    if (gVersionChar != nullptr) {
        static unsigned long lastPushMs = 0;
        if (millis() - lastPushMs > 5000) {
            gVersionChar->setValue(getFwVersion());
            gVersionChar->notify();
            lastPushMs = millis();
        }
    }

    if (gNeedsReboot) {
        Serial.println("[OTA] rebooting after successful update");
        delay(200);
        ESP.restart();
    }
}