# HAL / LiDAR

Hardware Abstraction Layer for the **VL53L0X** Time-of-Flight distance sensor.

## Responsibility

Initialises the VL53L0X over I²C in continuous back-to-back ranging mode and exposes a single polling call that returns the measured distance in millimetres. All sensor-specific register access and timing details are hidden behind `HAL_LiDAR`.

## I²C initialisation order

`Wire.begin()` must be called **before** `HAL_LiDAR::begin()` — and it must be called **after** the IMU is initialised, because `Arduino_LSM6DS3::begin()` calls `Wire.begin()` internally and would reset the I²C peripheral if called after the LiDAR. The correct order in `setup()`:

```cpp
Wire.begin();
imu.begin();     // IMU first — its Wire.begin() resets the bus
Wire.begin();    // reset bus again to clean up after IMU init
lidar.begin();   // LiDAR last — gets a clean bus
```

## Dependencies

| Library | Purpose |
|---------|---------|
| [VL53L0X by Pololu](https://github.com/pololu/vl53l0x-arduino) | VL53L0X Arduino driver |
| `Wire` (built-in) | I²C bus |

## Usage

```cpp
HAL_LiDAR lidar;

void setup() {
    Wire.begin();
    if (!lidar.begin()) { /* sensor not found */ }
}

void loop() {
    int16_t mm = lidar.readDistance(); // -1 on timeout or out-of-range
}
```

`readDistance()` returns -1 if the measurement times out (500 ms) or the reading exceeds 8190 mm (the sensor's out-of-range sentinel value).
