# HAL / IMU

Hardware Abstraction Layer for the **LSM6DS3** 6-axis IMU (gyroscope + accelerometer).

## Responsibility

Initialises the LSM6DS3 over I²C and integrates the gyroscope output over time to track the scanner's orientation (pitch, roll, yaw). The result is exposed as an `Orientation` struct in radians.

> **Note:** Pure gyroscope integration drifts over time. For long scanning sessions a complementary filter or Madgwick/Mahony filter fusing accelerometer data is recommended.

## Dependencies

| Library | Purpose |
|---------|---------|
| [SparkFun LSM6DS3](https://github.com/sparkfun/SparkFun_LSM6DS3_Arduino_Library) | LSM6DS3 Arduino driver |
| `Wire` (built-in) | I²C bus |

## Usage

```cpp
HAL_IMU imu;

void setup() {
    if (!imu.begin()) { /* handle error */ }
}

void loop() {
    Orientation o;
    if (imu.read(o)) { /* use o.pitch, o.roll, o.yaw */ }
}
```
