# Data / AppLayer

Application-layer protocol: encodes a `Point3D` into a framed binary packet and forwards it to the transport layer.

## Responsibility

Provides a typed `sendPoint()` call. Internally it serialises the three `float` coordinates into a 15-byte packet:

```
[ 0xAA | 0xBB ]  — 2-byte sync header
[ x(4B) y(4B) z(4B) ]  — coordinates as IEEE 754 little-endian floats
[ CRC8(1B) ]  — CRC-8/SMBUS over the 12 payload bytes
```

The framing and checksum allow the host application to detect packet boundaries and verify integrity.

## Dependencies

- `Data/TransportLayer` — raw byte sender
- `Util/PointGeometry` — `Point3D` type

## Usage

```cpp
Data_TransportLayer transport;
Data_AppLayer app(transport);

app.sendPoint(point); // encodes and transmits one packet
```
