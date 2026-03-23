#include "MatrixProtocolCodec.h"

namespace matrixproto {

uint16_t crc16Ccitt(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t index = 0; index < length; ++index) {
        crc ^= static_cast<uint16_t>(data[index]) << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

std::vector<uint8_t> buildPacket(uint8_t type, uint8_t seq, const uint8_t *payload, size_t length)
{
    std::vector<uint8_t> packet;
    packet.reserve(8 + length);

    packet.push_back(kPacketMagic);
    packet.push_back(kPacketVersion);
    packet.push_back(type);
    packet.push_back(seq);

    const uint16_t payloadLength = static_cast<uint16_t>(length);
    packet.push_back(static_cast<uint8_t>(payloadLength & 0xFF));
    packet.push_back(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));

    for (size_t index = 0; index < length; ++index) {
        packet.push_back(payload[index]);
    }

    const uint16_t crc = crc16Ccitt(packet.data(), packet.size());
    packet.push_back(static_cast<uint8_t>(crc & 0xFF));
    packet.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

    return packet;
}

std::vector<uint8_t> buildPacket(uint8_t type, uint8_t seq, const std::vector<uint8_t> &payload)
{
    if (payload.empty()) {
        return buildPacket(type, seq, nullptr, 0);
    }
    return buildPacket(type, seq, payload.data(), payload.size());
}

bool parsePacket(const uint8_t *data, size_t length, Packet &out)
{
    if (data == nullptr || length < 8) {
        return false;
    }

    if (data[0] != kPacketMagic || data[1] != kPacketVersion) {
        return false;
    }

    const uint16_t payloadLength = static_cast<uint16_t>(data[4])
                                 | (static_cast<uint16_t>(data[5]) << 8);

    if (length != static_cast<size_t>(8 + payloadLength)) {
        return false;
    }

    const uint16_t receivedCrc = static_cast<uint16_t>(data[6 + payloadLength])
                               | (static_cast<uint16_t>(data[7 + payloadLength]) << 8);
    const uint16_t computedCrc = crc16Ccitt(data, 6 + payloadLength);

    if (receivedCrc != computedCrc) {
        return false;
    }

    out.type = data[2];
    out.seq = data[3];
    out.payload.assign(data + 6, data + 6 + payloadLength);
    return true;
}

}