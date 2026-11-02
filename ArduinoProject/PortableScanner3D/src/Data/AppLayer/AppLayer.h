/**
 * @file AppLayer.h
 * @brief Application-layer protocol: typed, framed binary packets over the transport layer.
 *
 * Every packet shares the same wire format:
 * @code
 * [ 0xAA | 0xBB ] [ TYPE : 1 B ] [ LEN : 1 B ] [ PAYLOAD : LEN B ] [ CRC8 : 1 B ]
 * @endcode
 * CRC-8/SMBUS is computed over TYPE + LEN + PAYLOAD.
 *
 * Packet types:
 * | Type       | ID   | Payload (bytes)                              |
 * |------------|------|----------------------------------------------|
 * | POINT      | 0x01 | x(4) y(4) z(4) ox(4) oy(4) oz(4) — point + sensor origin, mm (24 B) |
 * | POSE       | 0x02 | px(4) py(4) pz(4) pitch(4) roll(4) yaw(4)   |
 * | LOG        | 0x03 | UTF-8 text (variable, max 251 B)             |
 *
 * All multi-byte fields are IEEE 754 little-endian floats.
 */

#pragma once
#include "../../Util/PointGeometry/PointGeometry.h"
#include "../TransportLayer/TransportLayer.h"
#include <stdint.h>

/** @brief Identifies the content of a packet's payload. */
enum PacketType : uint8_t {
    PACKET_POINT = 0x01, ///< Scanned surface point (x, y, z).
    PACKET_POSE  = 0x02, ///< Device position and orientation.
    PACKET_LOG   = 0x03  ///< Human-readable log/status message.
};

/**
 * @class Data_AppLayer
 * @brief Encodes typed packets and forwards them to the transport layer.
 */
class Data_AppLayer {
public:
    /**
     * @brief Constructs the application layer with the given transport.
     * @param transport Reference to an initialised @c Data_TransportLayer.
     */
    explicit Data_AppLayer(Data_TransportLayer& transport);

    /**
     * @brief Transmits a scanned surface point (PACKET_POINT).
     * @param point Surface point in the reference frame (mm).
     */
    void sendPoint(const Point3D& point, const Position& sensorOrigin);

    /**
     * @brief Transmits the current device pose (PACKET_POSE).
     *
     * Should be sent every loop iteration so the host can track the
     * handheld position in real time.
     *
     * @param pos    Device position relative to the reference origin (mm).
     * @param orient Device orientation relative to the reference frame.
     */
    void sendPose(const Position& pos, const Orientation& orient);

    /**
     * @brief Transmits a log or status message (PACKET_LOG).
     * @param msg Null-terminated UTF-8 string (max 251 characters).
     */
    void sendLog(const char* msg);

private:
    Data_TransportLayer& _transport;

    /** @brief Frames and transmits a single packet. */
    void _sendPacket(uint8_t type, const uint8_t* payload, uint8_t len);
};
