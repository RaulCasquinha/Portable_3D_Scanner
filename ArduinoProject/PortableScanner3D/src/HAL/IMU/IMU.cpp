/**
 * @file IMU.cpp
 * @brief HAL_IMU implementation: gyro integration + accelerometer dead-reckoning.
 */

#include "IMU.h"
#include <Arduino.h>
#include <Arduino_LSM6DS3.h>

// Arduino_LSM6DS3 exposes a pre-declared global object named IMU.

static const float G_MM_S2 = 9810.0f;   ///< 1 g in mm/s²

bool HAL_IMU::begin() {
    if (!IMU.begin()) return false;
    setReference();
    _lastUs = micros();
    return true;
}

void HAL_IMU::setReference() {
    _orientation = {0.0f, 0.0f, 0.0f};
    _position    = {0.0f, 0.0f, 0.0f};
    _vx = _vy = _vz = 0.0f;
}

bool HAL_IMU::read(Orientation& orient, Position& pos) {
    // Wait until both sensors have fresh data
    if (!IMU.accelerationAvailable() || !IMU.gyroscopeAvailable()) return false;

    unsigned long now = micros();
    float dt = (now - _lastUs) * 1e-6f;
    _lastUs = now;

    // --- Orientation: integrate gyroscope (deg/s → rad) ---
    float gx, gy, gz;
    IMU.readGyroscope(gx, gy, gz);
    _orientation.pitch += gx * DEG_TO_RAD * dt;
    _orientation.roll  += gy * DEG_TO_RAD * dt;
    _orientation.yaw   += gz * DEG_TO_RAD * dt;

    // --- Position: rotate accelerometer to world frame, remove gravity ---
    // Arduino_LSM6DS3 returns values in g; convert to mm/s²
    float ax, ay, az;
    IMU.readAcceleration(ax, ay, az);
    ax *= G_MM_S2;
    ay *= G_MM_S2;
    az *= G_MM_S2;

    // Pre-compute trig values for the current orientation
    float sp = sinf(_orientation.pitch), cp = cosf(_orientation.pitch);
    float sr = sinf(_orientation.roll),  cr = cosf(_orientation.roll);
    float sy = sinf(_orientation.yaw),   cy = cosf(_orientation.yaw);

    // Rotation matrix R = Rz(yaw) * Ry(pitch) * Rx(roll), applied to body-frame accel
    float ax_w = (cy*cp)*ax + (cy*sp*sr - sy*cr)*ay + (cy*sp*cr + sy*sr)*az;
    float ay_w = (sy*cp)*ax + (sy*sp*sr + cy*cr)*ay + (sy*sp*cr - cy*sr)*az;
    float az_w = (-sp)*ax   + (cp*sr)*ay             + (cp*cr)*az;

    // Remove gravity component (world Z is up, gravity is -G)
    az_w -= G_MM_S2;

    // Integrate acceleration → velocity → position (mm/s, mm)
    _vx += ax_w * dt;
    _vy += ay_w * dt;
    _vz += az_w * dt;

    _position.x += _vx * dt;
    _position.y += _vy * dt;
    _position.z += _vz * dt;

    orient = _orientation;
    pos    = _position;
    return true;
}
