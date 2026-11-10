/**
 * @file app_accel.h
 * @brief Accelerometer selected by APP_SENSOR (app_calib_conf.h) on SPI1,
 *        through its driver.
 *
 *  - ADXL345: 100 Hz, full resolution, +-16 g, SPI mode 3
 *  - ADXL355: 125 Hz, +-2 g, high-pass filter off, temperature on, SPI mode 0
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 */

#ifndef APP_ACCEL_H
#define APP_ACCEL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Sets the SPI mode, checks the device ID and configures the sensor.
 * @return false if the sensor is not found or the configuration failed
 */
bool APP_ACCEL_Init(void);

/**
 * @brief Waits for a new sample and reads it.
 * @param[out] sample_g X, Y, Z in g (nominal sensitivity)
 * @return false on a bus error or a timeout
 */
bool APP_ACCEL_ReadG(float sample_g[]);

/**
 * @brief Reads the temperature of the sensor.
 * @param[out] temperature_mdegc Temperature in millidegrees Celsius (nominal
 *             datasheet transfer function)
 * @return false on a bus error, or always if the sensor has no temperature
 *         sensor (ADXL345)
 */
bool APP_ACCEL_ReadTemperature(int32_t *temperature_mdegc);

#endif // APP_ACCEL_H
