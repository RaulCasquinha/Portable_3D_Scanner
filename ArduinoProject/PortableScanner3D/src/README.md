# src — Firmware source modules

Layered architecture. Higher layers call into lower ones; no upward dependencies.

```
src/
├── HAL/        Hardware Abstraction Layer — talks to physical sensors
│   ├── IMU/    LSM6DS3 driver
│   └── LiDAR/  VL53L0X driver
├── Util/       Pure computation helpers (no hardware dependency)
│   └── PointGeometry/  Spherical → Cartesian conversion
└── Data/       Serial packet encoding and transmission
    ├── AppLayer/        Typed packet builder with CRC-8
    └── TransportLayer/  Byte-level Serial.write() wrapper
```

## Layer responsibilities

| Layer | Module | Responsibility |
|-------|--------|---------------|
| HAL | `HAL_IMU` | Read LSM6DS3; integrate gyro → orientation; integrate accel → position |
| HAL | `HAL_LiDAR` | Read VL53L0X; return distance in mm |
| Util | `PointGeometry` | `toWorldPoint()`: convert distance + pose → world-space (x, y, z) |
| Data | `Data_AppLayer` | Encode POINT / POSE / LOG packets with header and CRC-8 |
| Data | `Data_TransportLayer` | Write raw bytes to `Serial` (USB-CDC) |

## Data flow

```
HAL_IMU  ──► orientation + position ──┐
                                       ├──► toWorldPoint() ──► Data_AppLayer ──► Data_TransportLayer ──► USB
HAL_LiDAR ──► distance (mm) ──────────┘
```
