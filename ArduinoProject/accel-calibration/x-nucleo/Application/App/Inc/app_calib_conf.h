/**
 * @file app_calib_conf.h
 * @brief Settings of the calibration application. Change and rebuild.
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 */

#ifndef APP_CALIB_CONF_H
#define APP_CALIB_CONF_H

#include "accel_calib.h"

#define APP_SENSOR_ADXL345              0
#define APP_SENSOR_ADXL355              1

/** APP_SENSOR_ADXL345 or APP_SENSOR_ADXL355, on SPI1 (CS on PA5) */
#define APP_SENSOR                      APP_SENSOR_ADXL345

/** Orientations to capture with the c key, up to 32. The usual calibration
 *  is the 6-point one: 6 independent orientations, for bias and scale; with
 *  the non-orthogonality the PC fit needs 9 or more. The PC app sends its
 *  own settings with $START */
#define APP_CALIB_POSITIONS             6U

/** Samples averaged per orientation (200 = about 2 s) */
#define APP_CALIB_SAMPLES               200U

/** Countdown from d to measuring, so the surface settles after the key press
 *  (s, 0 = none) */
#define APP_CALIB_SETTLE_S              1U

/** Max standard deviation of any axis for the board to be still (g) */
#define APP_CALIB_STILL_G               0.020F

/** Min angle between two captured orientations (deg) */
#define APP_CALIB_MIN_SEPARATION_DEG    30.0F

/** ACCEL_CALIB_REFERENCES_NONE, ACCEL_CALIB_REFERENCES_Z (+Z up) or
 *  ACCEL_CALIB_REFERENCES_Z_X (+Z up and +X up), for the alignment, an
 *  optional extra */
#define APP_CALIB_REFERENCES            ACCEL_CALIB_REFERENCES_NONE

/** Longest wait for d before a capture is abandoned (s): a capture left
 *  unattended ends instead of blocking the board (rule FUN-12) */
#define APP_CALIB_INPUT_TIMEOUT_S       600U

/* The calibration applied to the cal values of $READ: paste here the defines
 * of the PC app's Parameters window, then rebuild and flash. */
// calibrated at +33.34 C, mean of the start and end
// corrected = K * (raw - bias)
#define ACCEL_CAL_BIAS_G {0.0107114F, 0.0038867F, 0.0113503F}
#define ACCEL_CAL_K { \
    {1.0060449F, -0.0014838F, -0.0599296F}, \
    {0.0000000F, 0.9993683F, -0.0247662F}, \
    {0.0604110F, 0.0247103F, 0.9980292F} \
}

#if ((APP_CALIB_POSITIONS == 0U) \
     || (APP_CALIB_POSITIONS > ACCEL_CALIB_MAX_POSITIONS))
#error "APP_CALIB_POSITIONS must be 1 to 32"
#endif

#endif // APP_CALIB_CONF_H
