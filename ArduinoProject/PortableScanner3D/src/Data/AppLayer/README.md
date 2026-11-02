# Data / AppLayer

Application-layer protocol: encodes typed data into framed binary packets and forwards them to the transport layer.

## Packet wire format

```
[ 0xAA | 0xBB ]  2-byte sync header
[ TYPE  : 1 B ]  packet type identifier
[ LEN   : 1 B ]  payload length in bytes
[ PAYLOAD       ]  LEN bytes of data
[ CRC8  : 1 B ]  CRC-8/SMBUS over TYPE + LEN + PAYLOAD
```

## Packet types

| Type | ID | Payload | Description |
|------|----|---------|-------------|
| POINT | 0x01 | 24 B | `x y z` (surface point, mm) + `ox oy oz` (sensor origin, mm) |
| POSE | 0x02 | 24 B | `px py pz` (position, mm) + `pitch roll yaw` (radians) |
| LOG | 0x03 | variable | UTF-8 status string (max 251 B) |

All multi-byte fields are IEEE 754 little-endian floats.

The sensor origin `(ox, oy, oz)` in a POINT packet is the device world position captured at the **start of the accumulation window**, not at transmission time. The Python host uses it to compute the measurement ray `normalize(point − origin)` for ray-based cluster averaging.

## CRC

CRC-8/SMBUS: polynomial 0x07, initial value 0x00, computed over the concatenation of `[TYPE, LEN, PAYLOAD...]`.

## Dependencies

- `Data/TransportLayer` — raw byte sender
- `Util/PointGeometry` — `Point3D`, `Position`, `Orientation` types

## Usage

```cpp
Data_TransportLayer transport;
Data_AppLayer app(transport);

app.sendPoint(point, sensorOrigin);      // POINT packet (24 B payload)
app.sendPose(pos, orient);               // POSE  packet (24 B payload)
app.sendLog("[INFO] Ready");             // LOG   packet (variable)
```
