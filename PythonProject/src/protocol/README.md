# protocol — Binary packet decoder

Parses the USB-Serial byte stream from the ESP32-S3 into typed Python objects.

## Files

| File | Description |
|------|-------------|
| `app_layer.py` | `AppLayerDecoder` byte-stream state machine; `PointPacket`, `PosePacket`, `LogPacket` dataclasses |

## Packet format (mirrored from firmware)

```
[ 0xAA | 0xBB ] [ TYPE : 1 B ] [ LEN : 1 B ] [ PAYLOAD : LEN B ] [ CRC8 : 1 B ]
```

CRC-8/SMBUS (poly `0x07`, init `0x00`) is verified on receipt; corrupt packets are silently dropped.

## Data classes

| Class | Fields |
|-------|--------|
| `PointPacket` | `x, y, z, ox, oy, oz` — surface point + sensor origin (mm, float32) |
| `PosePacket` | `px, py, pz, pitch, roll, yaw` — position (mm) + orientation (rad) |
| `LogPacket` | `text` — UTF-8 string from device |
