# HAL / IMU

Hardware Abstraction Layer for the **LSM6DS3** 6-axis IMU (gyroscope + accelerometer).

## Responsibility

Initialises the LSM6DS3 over I²C and tracks the scanner's full 6-DOF state:

- **Orientation** (pitch, roll, yaw in radians) — gyroscope readings integrated over time
- **Position** (x, y, z in mm) — accelerometer rotated to world frame, gravity-compensated, double-integrated

Both are accumulated relative to a reference frame set by `setReference()`. Call it once the device is at the desired scan origin to zero out position and orientation.

> **Drift note:** Pure gyroscope integration drifts over time. For long scanning sessions a Madgwick or Mahony filter fusing accelerometer data is recommended.

## Dependencies

| Library | Purpose |
|---------|---------|
| [Arduino_LSM6DS3](https://github.com/arduino-libraries/Arduino_LSM6DS3) | LSM6DS3 Arduino driver (exposes pre-declared global `IMU`) |
| `Wire` (built-in) | I²C bus — must be initialised before `begin()` |

## Usage

```cpp
HAL_IMU imu;

void setup() {
    Wire.begin();
    if (!imu.begin()) { /* sensor not found — degrade gracefully */ }
    imu.setReference();   // zero the reference frame here
}

void loop() {
    Orientation orient;
    Position    pos;
    if (imu.read(orient, pos)) {
        // orient.pitch / .roll / .yaw in radians
        // pos.x / .y / .z in mm
    }
}
```

`read()` returns `false` if new data is not yet available from the sensor; call it every loop iteration and skip the frame if it returns false.
