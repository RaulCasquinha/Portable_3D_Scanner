# PortableScanner3D

Root of the Arduino sketch. Arduino IDE requires the `.ino` file to live in a folder of the same name.

## Entry point

`PortableScanner3D.ino` contains `setup()` and `loop()` plus the scan-and-average accumulator:

- **`setup()`** — initialises Serial (USB-CDC), Wire, IMU, LiDAR in the required order; sends a boot status LOG packet
- **`loop()`** — reads IMU + LiDAR each iteration; accumulates `ACC_N = 5` valid readings; flushes early if orientation or position changes beyond threshold; sends a POINT packet per flush; detects new USB-CDC host connections and resends boot status

## Accumulator constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `ACC_N` | 5 | Readings averaged per transmitted point |
| `ORI_THRESH` | 0.017 rad | ~1° orientation change triggers early flush |
| `POS_THRESH` | 2 mm | 2 mm translation triggers early flush |

## Source modules

See [`src/README.md`](src/README.md) for the full layered architecture.
