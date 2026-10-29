#include "LiDAR.h"
#include <Wire.h>
#include <VL53L0X.h>

static VL53L0X _sensor;

bool HAL_LiDAR::begin() {
    Wire.begin();
    if (!_sensor.init()) return false;
    _sensor.setTimeout(500);
    _sensor.startContinuous();
    return true;
}

int16_t HAL_LiDAR::readDistance() {
    uint16_t d = _sensor.readRangeContinuousMillimeters();
    if (_sensor.timeoutOccurred()) return -1;
    return (int16_t)d;
}
