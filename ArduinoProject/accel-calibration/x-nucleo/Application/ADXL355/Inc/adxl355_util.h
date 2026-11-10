/**
 * @file adxl355_util.h
 * @brief Utility functions of the ADXL355 accelerometer driver.
 *
 * Built on the register-level driver (adxl355.h): device configuration,
 * sample set unpacking and conversion to g, FIFO handling, temperature
 * conversion and a verified reset.
 *
 * @author Carlos Mendes
 * @date Jan/2026
 */

#ifndef ADXL355_UTIL_H
#define ADXL355_UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "adxl355.h"

/**
 * @brief Three-axis sample set, sign-extended to 32 bits.
 */
typedef struct {
    int32_t x_axis; /**< X acceleration in LSB (20-bit range) */
    int32_t y_axis; /**< Y acceleration in LSB (20-bit range) */
    int32_t z_axis; /**< Z acceleration in LSB (20-bit range) */
} ADXL355_UTIL_UnpackedSampleSet_t;

/**
 * @brief Three-axis sample set in g.
 */
typedef struct {
    float x_axis; /**< X acceleration in g */
    float y_axis; /**< Y acceleration in g */
    float z_axis; /**< Z acceleration in g */
} ADXL355_UTIL_SampleSetGs_t;

/**
 * @brief Device configuration applied by ADXL355_UTIL_Config().
 */
typedef struct {
    const ADXL355_Platform_t *platform; /**< Platform services */
    ADXL355_ODR_t odr;                   /**< Output data rate */
    ADXL355_HPF_t hpf;                   /**< High-pass filter corner */
    ADXL355_Range_t range;               /**< Measurement range */
    ADXL355_IntPol_t int_pol;            /**< INT1 and INT2 polarity */
    ADXL355_IntSource_t int_src;         /**< Interrupt map (OR of sources) */
    ADXL355_ActivityMode_t act_mode;     /**< Axes used for activity */
    uint8_t act_cnt;                     /**< Consecutive samples above the
                                              threshold to flag activity */
    uint16_t act_thr_lsb;                /**< ACT_THRESH register value */
    uint8_t fifo_samples;                /**< FIFO watermark in entries
                                              (single-axis samples, not
                                              sample sets) */
    int16_t x_offset_lsb;                /**< OFFSET_X register value */
    int16_t y_offset_lsb;                /**< OFFSET_Y register value */
    int16_t z_offset_lsb;                /**< OFFSET_Z register value */
    ADXL355_MeasurementMode_t meas_mode; /**< Mode set at the end */
} ADXL355_UTIL_DevCfg_t;

/**
 * @brief Configures the device and starts the selected operating mode.
 *
 * Puts the device in standby, writes every setting in dev_cfg and then sets
 * dev_cfg->meas_mode.
 *
 * @param[in] dev_cfg Configuration. Must not be NULL; every field must hold
 *            a valid value for its type.
 *
 * @retval ADXL355_OK Device configured and in dev_cfg->meas_mode.
 * @retval ADXL355_INVALID_PARAMETER dev_cfg is NULL, or a field is invalid
 *         (returned by the driver function that writes it).
 * @return Otherwise, the status returned by the bus callbacks.
 *
 * @note If a step fails, the device is left in standby with the
 *       configuration partly applied.
 * @note Blocking. Must not be called from interrupt context.
 */
ADXL355_Status_t ADXL355_UTIL_Config(const ADXL355_UTIL_DevCfg_t *dev_cfg);

/**
 * @brief Unpacks raw sample sets into sign-extended 32-bit values.
 *
 * @param[in] raw_sample_sets Raw sample sets, as read from the XDATA, YDATA
 *            and ZDATA registers or from the FIFO. Must not be NULL.
 * @param[in] num_sample_sets Number of sample sets in both buffers
 *            (at least 1).
 * @param[out] unpacked_sample_sets Unpacked sample sets. Must not be NULL
 *             and must not overlap raw_sample_sets.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A pointer is NULL or num_sample_sets is
 *         0.
 *
 * @note No hardware access. Reentrant.
 */
ADXL355_Status_t ADXL355_UTIL_UnpackSampleSets(
const ADXL355_RawSampleSet_t *raw_sample_sets, const size_t num_sample_sets,
ADXL355_UTIL_UnpackedSampleSet_t *unpacked_sample_sets);

/**
 * @brief Converts unpacked sample sets to g.
 *
 * @warning Uses floating-point arithmetic: software-emulated, slow and large
 *          on cores without an FPU, and not allowed in interrupt context
 *          (EMB-03).
 *
 * @param[in] unpacked_sample_sets Unpacked sample sets. Must not be NULL.
 * @param[in] range Range the samples were measured with.
 * @param[in] num_sample_sets Number of sample sets in both buffers
 *            (at least 1).
 * @param[out] sample_sets_gs Sample sets in g. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A pointer is NULL, num_sample_sets is 0
 *         or range is not a valid ADXL355_Range_t.
 */
ADXL355_Status_t ADXL355_UTIL_SampleSets2Gs(
const ADXL355_UTIL_UnpackedSampleSet_t *unpacked_sample_sets,
const ADXL355_Range_t range, const size_t num_sample_sets,
ADXL355_UTIL_SampleSetGs_t *sample_sets_gs);

/**
 * @brief Empties the FIFO by reading ADXL355_FIFO_MAX_SAMPLES entries.
 *
 * Useful for testing and debugging. The data read is discarded.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 *
 * @note Blocking. Uses ADXL355_FIFO_MAX_BYTES of stack. Must not be called
 *       from interrupt context.
 */
ADXL355_Status_t ADXL355_UTIL_ClearFifo(
const ADXL355_Platform_t *platform);

/**
 * @brief Converts an activity threshold in mg to the ACT_THRESH register
 *        value for the given range.
 *
 * ACT_THRESH is compared with bits 18:3 of the acceleration data, so one
 * register LSB is 8 data LSB. The conversion is exact, with integer
 * arithmetic: 1 mg is 32, 16 or 8 register LSB at +/-2, +/-4 or +/-8 g.
 *
 * @param[in] threshold_mg Threshold in mg. The register limits it to
 *            2047 mg at +/-2 g, 4095 mg at +/-4 g and 8191 mg at +/-8 g.
 * @param[in] range Range the device is configured with.
 * @param[out] threshold_lsb ACT_THRESH register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER threshold_lsb is NULL, range is not a
 *         valid ADXL355_Range_t or threshold_mg is above the limit of the
 *         range.
 *
 * @note No hardware access. Reentrant.
 */
ADXL355_Status_t ADXL355_UTIL_ActivityThreshold2LSB(
const uint16_t threshold_mg, const ADXL355_Range_t range,
uint16_t *threshold_lsb);

/**
 * @brief Converts an axis offset in mg to the OFFSET_x register value for
 *        the given range.
 *
 * The OFFSET_x LSB has the weight of bit 4 of the acceleration data, so one
 * register LSB is 16 data LSB. The conversion is exact, with integer
 * arithmetic: 1 mg is 16, 8 or 4 register LSB at +/-2, +/-4 or +/-8 g.
 *
 * The device subtracts the offset from the measurement: to cancel an error
 * of +E mg, pass +E.
 *
 * @param[in] offset_mg Offset in mg. The register limits it to
 *            -2048 to 2047 mg at +/-2 g, -4096 to 4095 mg at +/-4 g and
 *            -8192 to 8191 mg at +/-8 g.
 * @param[in] range Range the device is configured with.
 * @param[out] offset_lsb OFFSET_x register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER offset_lsb is NULL, range is not a
 *         valid ADXL355_Range_t or offset_mg is outside the limits of the
 *         range.
 *
 * @note No hardware access. Reentrant.
 */
ADXL355_Status_t ADXL355_UTIL_Offset2LSB(const int16_t offset_mg,
                                         const ADXL355_Range_t range,
                                         int16_t *offset_lsb);

/**
 * @brief Discards FIFO entries until the next entry read is an X axis entry.
 *
 * Reads entries until it has read one X entry followed by its Y and Z
 * entries. Empty entries are skipped. One sample set is lost.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] max_reads Maximum number of FIFO entry reads (at least 3).
 *            There is no delay between reads, so the time limit is
 *            max_reads times the time of one 3-byte bus read.
 *
 * @retval ADXL355_OK The FIFO is aligned on an X entry.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is
 *         NULL, or max_reads is below 3.
 * @retval ADXL355_TIMEOUT Not aligned after max_reads reads (for example,
 *         the device is in standby and the FIFO stays empty).
 * @return Otherwise, the status returned by the read callback.
 *
 * @note Blocking. Must not be called from interrupt context.
 */
ADXL355_Status_t ADXL355_UTIL_AlignFifo(
const ADXL355_Platform_t *platform, const uint16_t max_reads);

/**
 * @brief Reads the temperature and converts it to degrees Celsius using the
 *        nominal datasheet transfer function.
 *
 * @warning Uses floating-point arithmetic: software-emulated, slow and large
 *          on cores without an FPU, and not allowed in interrupt context
 *          (EMB-03). Prefer ADXL355_UTIL_ReadTemperatureMdegc() (integer,
 *          mdegC) or ADXL355_ReadRawTemperature() (raw LSB).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] temperature_degc Temperature in degC. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_UTIL_ReadTemperature(
const ADXL355_Platform_t *platform, float *temperature_degc);

/**
 * @brief Reads the temperature and converts it to millidegrees Celsius using
 *        the nominal datasheet transfer function, with integer arithmetic.
 *
 * The result is truncated towards zero (error below 1 mdegC from the
 * conversion; the sensor accuracy is much lower).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] temperature_mdegc Temperature in mdegC. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_UTIL_ReadTemperatureMdegc(
const ADXL355_Platform_t *platform, int32_t *temperature_mdegc);

/**
 * @brief Soft resets the device and checks that the factory trim was
 *        reloaded.
 *
 * Writes the reset code (ADXL355_SoftReset()), waits for NVM_BUSY to clear
 * and compares the shadow registers with the values the application stored
 * for this device.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] stored_shadow_registers Expected shadow register values.
 *            Must not be NULL.
 * @param[in] max_reset_retries Maximum number of NVM_BUSY polls after the
 *            reset command (1 to 255). The status is polled every
 *            ADXL355_NVM_POLL_PERIOD_MS (1 ms), so 1 count is 1 ms and this
 *            is the reset timeout in ms.
 *
 * @retval ADXL355_OK Reset done and shadow registers match.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or callback (read,
 *         write, delay) is NULL, or max_reset_retries is 0.
 * @retval ADXL355_TIMEOUT NVM_BUSY did not clear within max_reset_retries ms.
 * @retval ADXL355_RESET_ERROR The shadow registers could not be read or do
 *         not match.
 * @return Otherwise, the status returned by the bus callbacks.
 *
 * @note Blocking: uses the delay callback. Must not be called from interrupt
 *       context.
 */
ADXL355_Status_t ADXL355_UTIL_ResetAndVerify(
const ADXL355_Platform_t *platform,
const uint8_t stored_shadow_registers[ADXL355_TOTAL_SHADOW_REGISTERS],
const uint8_t max_reset_retries);

#endif // ADXL355_UTIL_H
