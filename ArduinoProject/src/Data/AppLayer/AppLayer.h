#pragma once
#include "../../Util/PointGeometry/PointGeometry.h"
#include "../TransportLayer/TransportLayer.h"

// Encodes a Point3D as a binary packet and hands it to the transport layer.
// Packet format: 0xAA 0xBB | x(4B) y(4B) z(4B) | CRC8(1B)
class Data_AppLayer {
public:
    explicit Data_AppLayer(Data_TransportLayer& transport);
    void sendPoint(const Point3D& point);
private:
    Data_TransportLayer& _transport;
};
