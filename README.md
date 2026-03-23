# Matrix 16x10 Firmware

ESP32 firmware for a 16x10 LED matrix with BLE control, OTA update support, and sensor-driven modes.

## Firmware Version

- Semantic firmware version: `v1.0.7` (`FW_SEMVER` in `src/Config.h`)
- CI release tags: `v*` (see `.github/workflows/main.yml`)

## BLE GATT Contract (App-Compatible)

Service UUID:
- `ABCD`

Characteristics:
- Mode (write): `1234`
- Canvas (write): `5678`
- Text (write): `00009ABC-0000-1000-8000-00805F9B34FB`
- Version (read/notify): `00009ABD-0000-1000-8000-00805F9B34FB`
- OTA data (write): `00009ABE-0000-1000-8000-00805F9B34FB`
- Control (write): `00009ABF-0000-1000-8000-00805F9B34FB`
- Ack (notify/indicate): `00009AC0-0000-1000-8000-00805F9B34FB`

## Protocol Frame

All multi-byte fields are little-endian.

- Byte 0: `magic` = `0xA5`
- Byte 1: `version` = `0x01`
- Byte 2: `type`
- Byte 3: `seq`
- Byte 4..5: `payload_len` (LE)
- Byte 6..: `payload`
- Final 2 bytes: `crc16_ccitt` (LE), computed over `magic..payload`

### Type IDs

Commands from app:
- `0x10` SetMode
- `0x11` SetText
- `0x12` GetVersion
- `0x13` SetCanvas
- `0x20` OtaBegin
- `0x21` OtaChunk
- `0x22` OtaEnd

Responses from firmware:
- `0x30` VersionResponse
- `0xF0` Ack
- `0xF1` Nack

### ACK/NACK Status Codes

- `0x00` success
- `0x01` bad frame/CRC
- `0x02` invalid state
- `0x03` bad offset
- `0x04` flash write error
- `0x05` finalize error

## OTA Flow

Protocol OTA flow:
1. Send `OtaBegin(totalSize, chunkSize)`
2. Send ordered `OtaChunk(offset, bytes...)`
3. Send `OtaEnd()`
4. Firmware ACKs each step and reboots on successful finalize

Notes:
- OTA offset must be monotonic (`offset == nextExpectedOffset`)
- Any error returns NACK with status code
- Firmware logs OTA state transitions over serial

## Legacy Compatibility

Legacy paths are still accepted:
- Mode characteristic: single-byte mode index
- Text characteristic: UTF-8 payload rendered to canvas
- Canvas characteristic: packed 20-byte bitmap
- Version characteristic write: `GET_VERSION`, then notify/read version string
- OTA characteristic: raw chunk stream (best-effort legacy path)

## Integration Checklist (Mobile App)

1. Scan and connect to BLE device name `Matrix_16x10`
2. Verify service `ABCD` is present
3. Verify writable characteristics:
   - `1234`, `5678`, `00009ABC-...`, `00009ABE-...`, `00009ABF-...`
4. Verify notify/indicate capability on `00009AC0-...`
5. Verify read/notify on `00009ABD-...`
6. Send `GetVersion (0x12)` frame and confirm:
   - `VersionResponse (0x30)` payload includes `v1.0.7`
   - `Ack (0xF0)` with status `0x00`
7. Send `SetMode`, `SetText`, and `SetCanvas` framed packets and verify display changes
8. Execute OTA begin/chunk/end sequence and verify ACK `0x00` for each packet
9. Confirm reboot after successful `OtaEnd`
10. Reconnect and verify updated firmware behavior/version
