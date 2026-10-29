#include "IMU.h"
#include <Arduino.h>
#include <SparkFunLSM6DS3.h>

static LSM6DS3 _imu(I2C_MODE, 0x6A);

static const float DEG_TO_RAD = 0.01745329f;

bool HAL_IMU::begin() {
    if (_imu.begin() != 0) return false;
    _orientation = {0.0f, 0.0f, 0.0f};
    _lastUs = micros();
    return true;
}

bool HAL_IMU::read(Orientation& out) {
    unsigned long now = micros();
    float dt = (now - _lastUs) * 1e-6f;
    _lastUs = now;

    // Integrate gyroscope (deg/s -> rad) to track orientation.
    // Note: gyro integration drifts over time; a complementary or
    // Madgwick filter should be used for long sessions.
    _orientation.pitch += _imu.readFloatGyroX() * DEG_TO_RAD * dt;
    _orientation.roll  += _imu.readFloatGyroY() * DEG_TO_RAD * dt;
    _orientation.yaw   += _imu.readFloatGyroZ() * DEG_TO_RAD * dt;

    out = _orientation;
    return true;
}
