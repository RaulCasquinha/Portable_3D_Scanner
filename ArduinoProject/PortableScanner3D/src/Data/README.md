# Data — Serial packet stack

Two-layer stack that encodes typed payloads and writes them to USB-Serial.

## Modules

| Module | Responsibility |
|--------|---------------|
| [`AppLayer/`](AppLayer/README.md) | Builds framed binary packets with CRC-8: header `0xAA 0xBB`, type byte, length byte, payload, checksum |
| [`TransportLayer/`](TransportLayer/README.md) | Thin wrapper around `Serial.write()` — separates physical I/O from packet logic |

## Packet format

```
[ 0xAA | 0xBB ] [ TYPE : 1 B ] [ LEN : 1 B ] [ PAYLOAD : LEN B ] [ CRC8 : 1 B ]
```

CRC-8/SMBUS (poly `0x07`, init `0x00`) computed over `TYPE + LEN + PAYLOAD`.

| Type | ID | Payload size | Content |
|------|----|-------------|---------|
| POINT | `0x01` | 24 B | `x y z ox oy oz` — 6 × float32 LE |
| POSE | `0x02` | 24 B | `px py pz pitch roll yaw` — 6 × float32 LE |
| LOG | `0x03` | variable | UTF-8 string (max 251 B) |
