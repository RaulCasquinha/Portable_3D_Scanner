/**
 * @file AppLayer.cpp
 * @brief Data_AppLayer implementation: packet framing, CRC-8, typed sends.
 */

#include "AppLayer.h"
#include <string.h>

static const uint8_t SYNC[2] = {0xAA, 0xBB};

/** @brief CRC-8/SMBUS over a buffer, continuing from an initial value. */
static uint8_t crc8_feed(uint8_t crc, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
    return crc;
}

Data_AppLayer::Data_AppLayer(Data_TransportLayer& transport) : _transport(transport) {}

void Data_AppLayer::_sendPacket(uint8_t type, const uint8_t* payload, uint8_t len) {
    uint8_t header[2] = {type, len};
    uint8_t crc = crc8_feed(0,   header,  2);
    crc         = crc8_feed(crc, payload, len);

    _transport.send(SYNC,    2);
    _transport.send(header,  2);
    _transport.send(payload, len);
    _transport.send(&crc,    1);
}

void Data_AppLayer::sendPoint(const Point3D& point, const Position& sensorOrigin) {
    uint8_t payload[24];
    memcpy(payload,      &point.x,        4);
    memcpy(payload + 4,  &point.y,        4);
    memcpy(payload + 8,  &point.z,        4);
    memcpy(payload + 12, &sensorOrigin.x, 4);
    memcpy(payload + 16, &sensorOrigin.y, 4);
    memcpy(payload + 20, &sensorOrigin.z, 4);
    _sendPacket(PACKET_POINT, payload, sizeof(payload));
}

void Data_AppLayer::sendPose(const Position& pos, const Orientation& orient) {
    uint8_t payload[24];
    memcpy(payload,      &pos.x,        4);
    memcpy(payload + 4,  &pos.y,        4);
    memcpy(payload + 8,  &pos.z,        4);
    memcpy(payload + 12, &orient.pitch, 4);
    memcpy(payload + 16, &orient.roll,  4);
    memcpy(payload + 20, &orient.yaw,   4);
    _sendPacket(PACKET_POSE, payload, sizeof(payload));
}

void Data_AppLayer::sendLog(const char* msg) {
    uint8_t len = (uint8_t)strlen(msg);
    _sendPacket(PACKET_LOG, reinterpret_cast<const uint8_t*>(msg), len);
}
