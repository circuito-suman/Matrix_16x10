#pragma once

#include <Arduino.h>

class MatrixOtaUpdateHandlerArduinoEsp32 {
public:
    bool begin(uint32_t totalSize, uint16_t chunkSize);
    bool writeChunk(uint32_t offset, const uint8_t *data, size_t length);
    bool writeLegacyChunk(const uint8_t *data, size_t length);
    bool end(bool rebootOnSuccess = true);

    void reset();

    uint32_t expectedSize() const;
    uint32_t writtenSize() const;
    uint16_t negotiatedChunkSize() const;
    bool active() const;

private:
    bool beginInternal(uint32_t totalSize, uint16_t chunkSize);

    bool m_active = false;
    uint32_t m_expectedSize = 0;
    uint32_t m_writtenSize = 0;
    uint16_t m_chunkSize = 0;
};
