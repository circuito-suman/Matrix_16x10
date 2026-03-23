#include "MatrixOtaUpdateHandler_ArduinoESP32.h"

#include <Update.h>

bool MatrixOtaUpdateHandlerArduinoEsp32::begin(uint32_t totalSize, uint16_t chunkSize)
{
    return beginInternal(totalSize, chunkSize);
}

bool MatrixOtaUpdateHandlerArduinoEsp32::writeChunk(uint32_t offset, const uint8_t *data, size_t length)
{
    if (!m_active || data == nullptr || length == 0) {
        return false;
    }

    if (offset != m_writtenSize) {
        return false;
    }

    if (m_expectedSize > 0 && (m_writtenSize + length) > m_expectedSize) {
        return false;
    }

    const size_t written = Update.write(const_cast<uint8_t *>(data), length);
    if (written != length) {
        return false;
    }

    m_writtenSize += static_cast<uint32_t>(written);
    return true;
}

bool MatrixOtaUpdateHandlerArduinoEsp32::writeLegacyChunk(const uint8_t *data, size_t length)
{
    if (!m_active || data == nullptr || length == 0) {
        return false;
    }

    if (m_expectedSize > 0 && (m_writtenSize + length) > m_expectedSize) {
        return false;
    }

    const size_t written = Update.write(const_cast<uint8_t *>(data), length);
    if (written != length) {
        return false;
    }

    m_writtenSize += static_cast<uint32_t>(written);
    return true;
}

bool MatrixOtaUpdateHandlerArduinoEsp32::end(bool rebootOnSuccess)
{
    if (!m_active) {
        return false;
    }

    if (m_expectedSize > 0 && m_writtenSize != m_expectedSize) {
        return false;
    }

    if (!Update.end(true)) {
        return false;
    }

    m_active = false;

    if (rebootOnSuccess) {
        delay(150);
        ESP.restart();
    }

    return true;
}

void MatrixOtaUpdateHandlerArduinoEsp32::reset()
{
    if (m_active) {
        Update.abort();
    }

    m_active = false;
    m_expectedSize = 0;
    m_writtenSize = 0;
    m_chunkSize = 0;
}

uint32_t MatrixOtaUpdateHandlerArduinoEsp32::expectedSize() const
{
    return m_expectedSize;
}

uint32_t MatrixOtaUpdateHandlerArduinoEsp32::writtenSize() const
{
    return m_writtenSize;
}

uint16_t MatrixOtaUpdateHandlerArduinoEsp32::negotiatedChunkSize() const
{
    return m_chunkSize;
}

bool MatrixOtaUpdateHandlerArduinoEsp32::active() const
{
    return m_active;
}

bool MatrixOtaUpdateHandlerArduinoEsp32::beginInternal(uint32_t totalSize, uint16_t chunkSize)
{
    if (totalSize == 0 || chunkSize == 0) {
        return false;
    }

    if (m_active) {
        Update.abort();
        m_active = false;
    }

    if (!Update.begin(totalSize)) {
        return false;
    }

    m_expectedSize = totalSize;
    m_writtenSize = 0;
    m_chunkSize = chunkSize;
    m_active = true;
    return true;
}
