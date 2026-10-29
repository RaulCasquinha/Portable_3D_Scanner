/**
 * @file AppLayer.h
 * @brief Application-layer protocol: frames a Point3D as a binary packet.
 *
 * Packet layout (15 bytes total):
 * @code
 * +--------+--------+--------+---------+---------+---------+-------+
 * | 0xAA   | 0xBB   | x (4B) | y (4B)  | z (4B)  | CRC8 (1B)      |
 * +--------+--------+--------+---------+---------+---------+-------+
 * @endcode
 *
 * The two-byte sync header lets the host reliably detect packet
 * boundaries. The CRC-8/SMBUS checksum covers the 12 payload bytes and
 * allows the host to detect single-byte corruption.
 */

#pragma once
#include "../../Util/PointGeometry/PointGeometry.h"
#include "../TransportLayer/TransportLayer.h"

/**
 * @class Data_AppLayer
 * @brief Encodes Point3D values as framed binary packets and forwards
 *        them to the transport layer.
 */
class Data_AppLayer {
public:
    /**
     * @brief Constructs the application layer with the given transport.
     *
     * @param transport Reference to an initialised @c Data_TransportLayer.
     */
    explicit Data_AppLayer(Data_TransportLayer& transport);

    /**
     * @brief Serialises a 3D point and transmits it as one packet.
     *
     * Encodes @p point as three IEEE 754 little-endian floats, prepends
     * the sync header, appends a CRC-8 byte, and hands the full packet
     * to the transport layer.
     *
     * @param point The 3D point to transmit (coordinates in mm).
     */
    void sendPoint(const Point3D& point);

private:
    Data_TransportLayer& _transport; ///< Underlying transport layer.
};
