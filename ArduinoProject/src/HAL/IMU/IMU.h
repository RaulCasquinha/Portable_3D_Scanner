/**
 * @file IMU.h
 * @brief HAL for the LSM6DS3 6-axis IMU (gyroscope + accelerometer).
 *
 * Wraps the SparkFun LSM6DS3 library and integrates gyroscope readings
 * over time to produce a running orientation estimate in radians.
 *
 * @note Pure gyro integration accumulates drift. For long scan sessions
 *       consider replacing the integrator with a Madgwick or Mahony filter
 *       that fuses accelerometer data.
 */

#pragma once

/**
 * @struct Orientation
 * @brief Scanner orientation angles, all in radians.
 */
struct Orientation {
    float pitch; ///< Rotation around the X axis (nose up/down).
    float roll;  ///< Rotation around the Y axis (tilt left/right).
    float yaw;   ///< Rotation around the Z axis (heading).
};

/**
 * @class HAL_IMU
 * @brief Hardware abstraction for the LSM6DS3 IMU.
 */
class HAL_IMU {
public:
    /**
     * @brief Initialises the LSM6DS3 over I²C and zeroes the orientation.
     *
     * Must be called once in @c setup() before any call to @c read().
     *
     * @return @c true on success, @c false if the sensor is not detected.
     */
    bool begin();

    /**
     * @brief Updates the orientation estimate from the latest gyro sample.
     *
     * Computes the elapsed time since the last call, converts angular
     * velocity (deg/s) to radians, and integrates into the stored angles.
     *
     * @param[out] out Updated orientation struct.
     * @return @c true on success, @c false on read error.
     */
    bool read(Orientation& out);

private:
    unsigned long _lastUs;    ///< Timestamp of the previous @c read() call (µs).
    Orientation   _orientation; ///< Accumulated orientation angles.
};
