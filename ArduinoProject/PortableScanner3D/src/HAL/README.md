# HAL — Hardware Abstraction Layer

Isolates all sensor-specific register access and timing. The sketch only calls the HAL; it never touches sensor libraries directly.

## Modules

| Module | Sensor | Header |
|--------|--------|--------|
| [`IMU/`](IMU/README.md) | LSM6DS3 6-axis IMU | `IMU.h` |
| [`LiDAR/`](LiDAR/README.md) | VL53L0X Time-of-Flight | `LiDAR.h` |

Both sensors share the I²C bus. See [`LiDAR/README.md`](LiDAR/README.md) for the required initialisation order.
