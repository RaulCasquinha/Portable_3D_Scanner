/**
 * @file LiDAR.cpp
 * @brief HAL_LiDAR implementation using the Pololu VL53L0X library.
 */

#include "LiDAR.h"
#include <VL53L0X.h>

static VL53L0X _sensor;

bool HAL_LiDAR::begin() {
    _sensor.setTimeout(500);
    if (!_sensor.init()) return false;
    _sensor.startContinuous();
    return true;
}

int16_t HAL_LiDAR::readDistance() {
    uint16_t d = _sensor.readRangeContinuousMillimeters();
    if (_sensor.timeoutOccurred() || d >= 8190) return -1;
    return (int16_t)d;
}
