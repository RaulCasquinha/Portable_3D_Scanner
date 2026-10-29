# HAL / LiDAR

Hardware Abstraction Layer for the **VL53L0X** Time-of-Flight distance sensor.

## Responsibility

Initialises the VL53L0X over I²C and exposes a single polling call that returns the measured distance in millimetres. All sensor-specific register access and timing details are hidden behind the `HAL_LiDAR` class so the rest of the application is independent of the physical sensor.

## Dependencies

| Library | Purpose |
|---------|---------|
| [Pololu VL53L0X](https://github.com/pololu/vl53l0x-arduino) | VL53L0X Arduino driver |
| `Wire` (built-in) | I²C bus |

## Usage

```cpp
HAL_LiDAR lidar;

void setup() {
    if (!lidar.begin()) { /* handle error */ }
}

void loop() {
    int16_t mm = lidar.readDistance(); // -1 on timeout
}
```
