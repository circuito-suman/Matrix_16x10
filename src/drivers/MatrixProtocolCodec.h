#pragma once

#include <Arduino.h>
#include <vector>

namespace matrixproto {

static constexpr uint8_t kPacketMagic = 0xA5;
static constexpr uint8_t kPacketVersion = 0x01;

enum PacketType : uint8_t {
    SetMode = 0x10,
    SetText = 0x11,
    GetVersion = 0x12,
    SetCanvas = 0x13,
    GetModes = 0x14,
    OtaBegin = 0x20,
    OtaChunk = 0x21,
    OtaEnd = 0x22,
    VersionResponse = 0x30,
    ModesResponse = 0x31,
    Ack = 0xF0,
    Nack = 0xF1,
};

struct Packet {
    uint8_t type = 0;
    uint8_t seq = 0;
    std::vector<uint8_t> payload;
};

uint16_t crc16Ccitt(const uint8_t *data, size_t length);
std::vector<uint8_t> buildPacket(uint8_t type, uint8_t seq, const uint8_t *payload, size_t length);
std::vector<uint8_t> buildPacket(uint8_t type, uint8_t seq, const std::vector<uint8_t> &payload);
bool parsePacket(const uint8_t *data, size_t length, Packet &out);

}