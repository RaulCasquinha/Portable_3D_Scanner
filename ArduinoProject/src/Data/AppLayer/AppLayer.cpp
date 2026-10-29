#include "AppLayer.h"
#include <string.h>

static const uint8_t SYNC[2] = {0xAA, 0xBB};

static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
    return crc;
}

Data_AppLayer::Data_AppLayer(Data_TransportLayer& transport) : _transport(transport) {}

void Data_AppLayer::sendPoint(const Point3D& point) {
    uint8_t payload[12];
    memcpy(payload,      &point.x, 4);
    memcpy(payload + 4,  &point.y, 4);
    memcpy(payload + 8,  &point.z, 4);
    uint8_t crc = crc8(payload, sizeof(payload));

    _transport.send(SYNC, 2);
    _transport.send(payload, sizeof(payload));
    _transport.send(&crc, 1);
}
