/**
 * @file LiDAR.h
 * @brief HAL for the VL53L0X Time-of-Flight distance sensor.
 *
 * Wraps the Pololu VL53L0X library and exposes a minimal interface
 * for initialisation and distance polling. All I²C and timing details
 * are encapsulated here so upper layers are sensor-agnostic.
 */

#pragma once
#include <stdint.h>

/**
 * @class HAL_LiDAR
 * @brief Hardware abstraction for the VL53L0X ToF sensor.
 */
class HAL_LiDAR {
public:
    /**
     * @brief Initialises the I²C bus and the VL53L0X sensor.
     *
     * Starts continuous ranging mode with a 500 ms read timeout.
     * Must be called once in @c setup() before any call to @c readDistance().
     *
     * @return @c true on success, @c false if the sensor is not detected.
     */
    bool begin();

    /**
     * @brief Returns the most recent distance measurement.
     *
     * Reads the next sample from the continuous ranging buffer.
     *
     * @return Distance in millimetres (0–8190), or @c -1 on timeout/error.
     */
    int16_t readDistance();
};
