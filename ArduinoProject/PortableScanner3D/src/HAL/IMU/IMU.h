/**
 * @file IMU.h
 * @brief HAL for the LSM6DS3 6-axis IMU (gyroscope + accelerometer).
 *
 * Reads both the gyroscope and the accelerometer each call:
 * - Gyroscope readings are integrated to track orientation (pitch, roll, yaw).
 * - Accelerometer readings are rotated to the world frame, gravity-compensated,
 *   and double-integrated to track position relative to the reference.
 *
 * Call @c setReference() at any time to zero both the position and the
 * orientation, defining the current pose as the world origin.
 *
 * @note Dead-reckoning from a single IMU drifts over time (especially
 *       position, which double-integrates noise). For extended sessions,
 *       fuse with an external position fix or use a Madgwick/Mahony filter.
 */

#pragma once
#include "../../Util/PointGeometry/PointGeometry.h"

/**
 * @class HAL_IMU
 * @brief Hardware abstraction for the LSM6DS3 IMU.
 *
 * Tracks 6-DOF state: orientation (from gyro integration) and position
 * (from double-integration of gravity-compensated accelerometer data).
 */
class HAL_IMU {
public:
    /**
     * @brief Initialises the LSM6DS3 via the Arduino_LSM6DS3 library and resets all accumulators.
     *
     * Must be called once in @c setup() before any call to @c read().
     *
     * @return @c true on success, @c false if the sensor is not detected.
     */
    bool begin();

    /**
     * @brief Zeroes position, velocity, and orientation, defining the
     *        current device pose as the world reference origin.
     *
     * Call this when the device is at the desired scan origin (e.g. on
     * button press) to reset the reference frame.
     */
    void setReference();

    /**
     * @brief Reads the IMU and updates orientation and position estimates.
     *
     * Integrates gyroscope → orientation; rotates accelerometer to world
     * frame, removes gravity, integrates twice → position.
     *
     * @param[out] orient  Updated orientation relative to the reference.
     * @param[out] pos     Updated position relative to the reference (mm).
     * @return @c true on success, @c false on read error.
     */
    bool read(Orientation& orient, Position& pos);

private:
    unsigned long _lastUs;

    Orientation _orientation; ///< Accumulated gyro-integrated angles (rad).
    Position    _position;    ///< Accumulated double-integrated position (mm).
    float       _vx;          ///< Velocity X component (mm/s).
    float       _vy;          ///< Velocity Y component (mm/s).
    float       _vz;          ///< Velocity Z component (mm/s).
};
