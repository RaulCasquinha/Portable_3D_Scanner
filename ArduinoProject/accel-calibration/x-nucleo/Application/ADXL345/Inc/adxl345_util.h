/**
 * @file adxl345_util.h
 * @brief Utility functions of the ADXL345 accelerometer driver.
 *
 * Built on the register-level driver (adxl345.h): device configuration,
 * sample set unpacking and conversion to g, FIFO handling and mg to register
 * conversions. The mg to register conversions use integer arithmetic, with
 * the scale factors of adxl345.h.
 *
 * @author Raul Casquinha
 * @date Oct/2026
 */

#ifndef ADXL345_UTIL_H
#define ADXL345_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adxl345.h"

/**
 * @brief Three-axis sample set, sign-extended to 16 bits.
 */
typedef struct {
    int16_t x_axis; /**< X acceleration in LSB */
    int16_t y_axis; /**< Y acceleration in LSB */
    int16_t z_axis; /**< Z acceleration in LSB */
} ADXL345_UTIL_UnpackedSampleSet_t;

/**
 * @brief Three-axis sample set in g.
 */
typedef struct {
    float x_axis; /**< X acceleration in g */
    float y_axis; /**< Y acceleration in g */
    float z_axis; /**< Z acceleration in g */
} ADXL345_UTIL_SampleSetGs_t;

/**
 * @brief Device configuration applied by ADXL345_UTIL_Config().
 *
 * Thresholds and times are register values (see ADXL345_UTIL_Threshold2LSB()
 * and ADXL345_UTIL_Offset2LSB()). Axes fields take an OR of
 * ADXL345_AXIS_x_EN flags, interrupt fields an OR of ADXL345_INT_x sources.
 */
typedef struct {
    const ADXL345_Platform_t *platform; /**< Platform services */
    ADXL345_ODR_t odr;                   /**< Output data rate */
    bool low_power;                      /**< Reduced power operation
                                              (12.5 Hz to 400 Hz only) */
    ADXL345_Range_t range;               /**< Measurement range */
    bool full_res;                       /**< Full resolution (3.9 mg/LSB
                                              at any range) */
    ADXL345_IntPol_t int_pol;            /**< INT1 and INT2 polarity */
    uint8_t int_map;                     /**< Interrupt map (sources mapped
                                              to INT2, the others to INT1) */
    uint8_t int_enable;                  /**< Enabled interrupt sources,
                                              written last */
    uint8_t act_axes;                    /**< Axes used for activity */
    ADXL345_Coupling_t act_coupling;     /**< Activity dc/ac coupling */
    uint8_t act_thr_lsb;                 /**< THRESH_ACT register value */
    uint8_t inact_axes;                  /**< Axes used for inactivity */
    ADXL345_Coupling_t inact_coupling;   /**< Inactivity dc/ac coupling */
    uint8_t inact_thr_lsb;               /**< THRESH_INACT register value */
    uint8_t inact_time_s;                /**< TIME_INACT register value
                                              (1 s/LSB) */
    uint8_t tap_axes;                    /**< Axes used for tap detection */
    bool tap_suppress;                   /**< Suppress double tap on
                                              acceleration between taps */
    uint8_t tap_thr_lsb;                 /**< THRESH_TAP register value */
    uint8_t tap_dur_lsb;                 /**< DUR register value (0
                                              disables tap) */
    uint8_t tap_latent_lsb;              /**< LATENT register value (0
                                              disables double tap) */
    uint8_t tap_window_lsb;              /**< WINDOW register value (0
                                              disables double tap) */
    uint8_t ff_thr_lsb;                  /**< THRESH_FF register value */
    uint8_t ff_time_lsb;                 /**< TIME_FF register value */
    ADXL345_FifoMode_t fifo_mode;        /**< FIFO mode */
    uint8_t fifo_samples;                /**< FIFO watermark in sample sets
                                              (0 to 31) */
    ADXL345_IntPin_t fifo_trigger;       /**< Pin whose interrupt triggers
                                              the FIFO (trigger mode) */
    int8_t x_offset_lsb;                 /**< OFSX register value */
    int8_t y_offset_lsb;                 /**< OFSY register value */
    int8_t z_offset_lsb;                 /**< OFSZ register value */
    bool link;                           /**< Link activity and
                                              inactivity */
    bool auto_sleep;                     /**< Auto sleep on inactivity
                                              (needs link) */
    ADXL345_MeasurementMode_t meas_mode; /**< Mode set at the end */
} ADXL345_UTIL_DevCfg_t;

/**
 * @brief Configures the device and starts the selected operating mode.
 *
 * Checks the device ID, puts the device in standby with the interrupts
 * disabled, writes every setting in dev_cfg, enables the interrupts and then
 * sets dev_cfg->meas_mode. DATA_FORMAT is written in a single write, which
 * also leaves self-test off, 4-wire SPI and right-justified data.
 *
 * @param[in] dev_cfg Configuration. Must not be NULL; every field must hold
 *            a valid value for its type.
 *
 * @retval ADXL345_OK Device configured and in dev_cfg->meas_mode.
 * @retval ADXL345_INVALID_PARAMETER dev_cfg is NULL, or a field is invalid
 *         (returned by the driver function that writes it).
 * @retval ADXL345_UNEXPECTED_READ_VALUE The device ID is not ADXL345_DEVID.
 * @return Otherwise, the status returned by the bus callbacks.
 *
 * @note If a step fails, the device is left in standby with the
 *       configuration partly applied.
 * @note Blocking. Must not be called from interrupt context.
 */
ADXL345_Status_t ADXL345_UTIL_Config(const ADXL345_UTIL_DevCfg_t *dev_cfg);

/**
 * @brief Unpacks raw sample sets into sign-extended 16-bit values.
 *
 * Expects right-justified data (DATA_FORMAT Justify = 0, as left at reset and
 * by ADXL345_UTIL_Config()).
 *
 * @param[in] raw_sample_sets Raw sample sets, as read from the DATAX0..DATAZ1
 *            registers or from the FIFO. Must not be NULL.
 * @param[in] num_sample_sets Number of sample sets in both buffers
 *            (at least 1).
 * @param[out] unpacked_sample_sets Unpacked sample sets. Must not be NULL
 *             and must not overlap raw_sample_sets.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A pointer is NULL or num_sample_sets is
 *         0.
 *
 * @note No hardware access. Reentrant.
 */
ADXL345_Status_t ADXL345_UTIL_UnpackSampleSets(
    const ADXL345_RawSampleSet_t *raw_sample_sets,
    const size_t num_sample_sets,
    ADXL345_UTIL_UnpackedSampleSet_t *unpacked_sample_sets);

/**
 * @brief Converts unpacked sample sets to g.
 *
 * @warning Uses floating-point arithmetic: software-emulated, slow and large
 *          on cores without an FPU, and not allowed in interrupt context
 *          (EMB-03).
 *
 * @param[in] unpacked_sample_sets Unpacked sample sets. Must not be NULL.
 * @param[in] range Range the samples were measured with (ignored in full
 *            resolution).
 * @param[in] full_res Whether the samples were measured in full resolution.
 * @param[in] num_sample_sets Number of sample sets in both buffers
 *            (at least 1).
 * @param[out] sample_sets_gs Sample sets in g. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A pointer is NULL, num_sample_sets is 0
 *         or range is not a valid ADXL345_Range_t.
 */
ADXL345_Status_t ADXL345_UTIL_SampleSets2Gs(
    const ADXL345_UTIL_UnpackedSampleSet_t *unpacked_sample_sets,
    const ADXL345_Range_t range, const bool full_res,
    const size_t num_sample_sets, ADXL345_UTIL_SampleSetGs_t *sample_sets_gs);

/**
 * @brief Empties the FIFO by reading all the stored sample sets.
 *
 * Useful for testing and debugging. The data read is discarded.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 *
 * @note Blocking. Uses ADXL345_FIFO_MAX_READ_SETS sample sets of stack.
 *       Must not be called from interrupt context.
 */
ADXL345_Status_t ADXL345_UTIL_ClearFifo(const ADXL345_Platform_t *platform);

/**
 * @brief Converts a threshold in mg to the nearest THRESH_TAP, THRESH_ACT,
 *        THRESH_INACT or THRESH_FF value (62.5 mg/LSB, any range).
 *
 * @param[in] threshold_mg Threshold in mg. The register limits it to
 *            15937 mg (255 LSB).
 * @param[out] threshold_lsb Register value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER threshold_lsb is NULL or threshold_mg is
 *         above the limit.
 *
 * @note No hardware access. Reentrant.
 */
ADXL345_Status_t ADXL345_UTIL_Threshold2LSB(const uint16_t threshold_mg,
                                            uint8_t *threshold_lsb);

/**
 * @brief Converts an axis offset in mg to the nearest OFSX, OFSY or OFSZ
 *        value (15.6 mg/LSB, two's complement, any range).
 *
 * The device adds the offset to the measurement: to cancel an error of
 * +E mg, pass -E (the ADXL355 subtracts it instead).
 *
 * @param[in] offset_mg Offset in mg. The register limits it to about -2000
 *            to 1988 mg (-128 to 127 LSB).
 * @param[out] offset_lsb Register value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER offset_lsb is NULL or offset_mg is
 *         outside the limits.
 *
 * @note No hardware access. Reentrant.
 */
ADXL345_Status_t ADXL345_UTIL_Offset2LSB(const int16_t offset_mg,
                                         int8_t *offset_lsb);

#endif // ADXL345_UTIL_H
