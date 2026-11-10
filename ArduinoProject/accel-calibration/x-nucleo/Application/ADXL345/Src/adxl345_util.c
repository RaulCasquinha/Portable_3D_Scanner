/**
 * @file adxl345_util.c
 * @brief Implementation of the ADXL345 utility functions.
 *
 * @author Raul Casquinha
 * @date Oct/2026
 */

#include "adxl345_util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adxl345.h"

#define UG_PER_MG               1000    // ug in 1 mg
#define BYTE_BITS               8U      // Shift of the MSB of a sample
#define INT16_RANGE             65536   // Two's complement decoding

static ADXL345_Status_t GetSensitivity(const ADXL345_Range_t range,
                                       const bool full_res,
                                       int32_t *sensitivity_lsb_per_g);
static int16_t UnpackAxis(const uint8_t axis_bytes[ADXL345_BYTES_PER_SAMPLE]);
static int32_t DivideRounded(const int32_t dividend, const int32_t divisor);

ADXL345_Status_t ADXL345_UTIL_Config(const ADXL345_UTIL_DevCfg_t *dev_cfg) {

    ADXL345_Status_t ret_status = ADXL345_OK;
    uint8_t data_format = 0U;

    if (dev_cfg == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_CheckDevId(dev_cfg->platform);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // Config modifications must be done in STANDBY mode, with the interrupts
    // disabled
    ret_status = ADXL345_SetMode(dev_cfg->platform, ADXL345_STANDBY);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_ClearInterrupts(dev_cfg->platform);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // ===================== Configure ADXL345 =====================

    ret_status = ADXL345_SetODR(dev_cfg->platform, dev_cfg->odr);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetLowPower(dev_cfg->platform, dev_cfg->low_power);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // DATA_FORMAT in a single write: range, resolution and polarity, with
    // SELF_TEST off, 4-wire SPI and right-justified data
    if (((uint32_t)dev_cfg->range > (uint32_t)ADXL345_RANGE_16G) ||
        ((dev_cfg->int_pol != ADXL345_INT_POL_HIGH) &&
         (dev_cfg->int_pol != ADXL345_INT_POL_LOW))) {
        return ADXL345_INVALID_PARAMETER;
    }

    data_format = (uint8_t)((uint8_t)dev_cfg->range |
                            (uint8_t)dev_cfg->int_pol);
    if (dev_cfg->full_res) {
        data_format = (uint8_t)(data_format | ADXL345_FULL_RES_MASK);
    }

    ret_status = ADXL345_WriteRegister(dev_cfg->platform,
                                       ADXL345_REG_DATA_FORMAT, data_format);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetInterruptMap(dev_cfg->platform, dev_cfg->int_map);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetActivityThreshold(dev_cfg->platform,
                                              dev_cfg->act_thr_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_EnableActivity(dev_cfg->platform, dev_cfg->act_axes,
                                        dev_cfg->act_coupling);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetInactivityThreshold(dev_cfg->platform,
                                                dev_cfg->inact_thr_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetInactivityTime(dev_cfg->platform,
                                           dev_cfg->inact_time_s);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_EnableInactivity(dev_cfg->platform,
                                          dev_cfg->inact_axes,
                                          dev_cfg->inact_coupling);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetTapThreshold(dev_cfg->platform,
                                         dev_cfg->tap_thr_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetTapDuration(dev_cfg->platform,
                                        dev_cfg->tap_dur_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetTapLatency(dev_cfg->platform,
                                       dev_cfg->tap_latent_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetTapWindow(dev_cfg->platform,
                                      dev_cfg->tap_window_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetTapAxes(dev_cfg->platform, dev_cfg->tap_axes,
                                    dev_cfg->tap_suppress);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetFreeFallThreshold(dev_cfg->platform,
                                              dev_cfg->ff_thr_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetFreeFallTime(dev_cfg->platform,
                                         dev_cfg->ff_time_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetFifoMode(dev_cfg->platform, dev_cfg->fifo_mode);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetFifoSamples(dev_cfg->platform,
                                        dev_cfg->fifo_samples);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetFifoTrigger(dev_cfg->platform,
                                        dev_cfg->fifo_trigger);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetAxisOffset(dev_cfg->platform, ADXL345_X_AXIS,
                                       dev_cfg->x_offset_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetAxisOffset(dev_cfg->platform, ADXL345_Y_AXIS,
                                       dev_cfg->y_offset_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetAxisOffset(dev_cfg->platform, ADXL345_Z_AXIS,
                                       dev_cfg->z_offset_lsb);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    ret_status = ADXL345_SetLinkAutoSleep(dev_cfg->platform, dev_cfg->link,
                                          dev_cfg->auto_sleep);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // Interrupts only after the functions are configured (DS p.19)
    ret_status = ADXL345_SetInterruptEnable(dev_cfg->platform,
                                            dev_cfg->int_enable);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // ===================== Start Measurement mode =====================

    ret_status = ADXL345_SetMode(dev_cfg->platform, dev_cfg->meas_mode);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    return ADXL345_OK;
}

ADXL345_Status_t ADXL345_UTIL_UnpackSampleSets(
    const ADXL345_RawSampleSet_t *raw_sample_sets,
    const size_t num_sample_sets,
    ADXL345_UTIL_UnpackedSampleSet_t *unpacked_sample_sets) {

    if ((raw_sample_sets == NULL) || (unpacked_sample_sets == NULL) ||
        (num_sample_sets == 0U)) {
        return ADXL345_INVALID_PARAMETER;
    }

    for (size_t i = 0U; i < num_sample_sets; ++i) {
        unpacked_sample_sets[i].x_axis = UnpackAxis(raw_sample_sets[i].x_axis);
        unpacked_sample_sets[i].y_axis = UnpackAxis(raw_sample_sets[i].y_axis);
        unpacked_sample_sets[i].z_axis = UnpackAxis(raw_sample_sets[i].z_axis);
    }

    return ADXL345_OK;
}

ADXL345_Status_t ADXL345_UTIL_SampleSets2Gs(
    const ADXL345_UTIL_UnpackedSampleSet_t *unpacked_sample_sets,
    const ADXL345_Range_t range, const bool full_res,
    const size_t num_sample_sets, ADXL345_UTIL_SampleSetGs_t *sample_sets_gs) {

    ADXL345_Status_t ret_status = ADXL345_OK;
    int32_t sensitivity_lsb_per_g = 0;
    float sensitivity = 0.0f;

    if ((unpacked_sample_sets == NULL) || (sample_sets_gs == NULL) ||
        (num_sample_sets == 0U)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = GetSensitivity(range, full_res, &sensitivity_lsb_per_g);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    // Exact: the sensitivities are below 2^24
    sensitivity = (float)sensitivity_lsb_per_g;

    for (size_t i = 0U; i < num_sample_sets; ++i) {
        sample_sets_gs[i].x_axis =
            (float)unpacked_sample_sets[i].x_axis / sensitivity;
        sample_sets_gs[i].y_axis =
            (float)unpacked_sample_sets[i].y_axis / sensitivity;
        sample_sets_gs[i].z_axis =
            (float)unpacked_sample_sets[i].z_axis / sensitivity;
    }

    return ADXL345_OK;
}

ADXL345_Status_t ADXL345_UTIL_ClearFifo(const ADXL345_Platform_t *platform) {

    ADXL345_Status_t ret_status = ADXL345_OK;
    ADXL345_RawSampleSet_t fifo_data[ADXL345_FIFO_MAX_READ_SETS];
    uint8_t num_fifo_entries = 0U;

    ret_status = ADXL345_ReadNumberFifoEntries(platform, &num_fifo_entries);
    if ((ret_status != ADXL345_OK) || (num_fifo_entries == 0U)) {
        return ret_status;
    }

    return ADXL345_ReadFifoSampleSet(platform, fifo_data, num_fifo_entries);
}

ADXL345_Status_t ADXL345_UTIL_Threshold2LSB(const uint16_t threshold_mg,
                                            uint8_t *threshold_lsb) {

    int32_t value_lsb = 0;

    if (threshold_lsb == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Max product 65535 * 1000, fits in int32
    value_lsb = DivideRounded((int32_t)threshold_mg * UG_PER_MG,
                              (int32_t)ADXL345_THRESH_SCALE_UG);

    if (value_lsb > (int32_t)UINT8_MAX) {
        return ADXL345_INVALID_PARAMETER;
    }

    *threshold_lsb = (uint8_t)value_lsb;

    return ADXL345_OK;
}

ADXL345_Status_t ADXL345_UTIL_Offset2LSB(const int16_t offset_mg,
                                         int8_t *offset_lsb) {

    int32_t value_lsb = 0;

    if (offset_lsb == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Max product 32768 * 1000, fits in int32
    value_lsb = DivideRounded((int32_t)offset_mg * UG_PER_MG,
                              (int32_t)ADXL345_OFFSET_SCALE_UG);

    if ((value_lsb < INT8_MIN) || (value_lsb > INT8_MAX)) {
        return ADXL345_INVALID_PARAMETER;
    }

    *offset_lsb = (int8_t)value_lsb;

    return ADXL345_OK;
}

/**
 * @brief Returns the sensitivity of a range and resolution.
 *
 * @param[in] range Measurement range.
 * @param[in] full_res true in full resolution (same sensitivity at every
 *            range).
 * @param[out] sensitivity_lsb_per_g Sensitivity in LSB/g.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER range is not a valid ADXL345_Range_t.
 */
static ADXL345_Status_t GetSensitivity(const ADXL345_Range_t range,
                                       const bool full_res,
                                       int32_t *sensitivity_lsb_per_g) {

    switch (range) {
        case ADXL345_RANGE_2G:
            *sensitivity_lsb_per_g = (int32_t)ADXL345_SENSITIVITY_2G;
            break;
        case ADXL345_RANGE_4G:
            *sensitivity_lsb_per_g = (int32_t)ADXL345_SENSITIVITY_4G;
            break;
        case ADXL345_RANGE_8G:
            *sensitivity_lsb_per_g = (int32_t)ADXL345_SENSITIVITY_8G;
            break;
        case ADXL345_RANGE_16G:
            *sensitivity_lsb_per_g = (int32_t)ADXL345_SENSITIVITY_16G;
            break;
        default:
            return ADXL345_INVALID_PARAMETER;
    }

    if (full_res) {
        *sensitivity_lsb_per_g = (int32_t)ADXL345_SENSITIVITY_FULL_RES;
    }

    return ADXL345_OK;
}

/**
 * @brief Converts one axis from the register format to a signed value.
 *
 * @param[in] axis_bytes DATAx0 and DATAx1 of the axis: two's complement
 *            value, right justified, LSB first.
 *
 * @return Sign-extended value in LSB.
 */
static int16_t UnpackAxis(const uint8_t axis_bytes[ADXL345_BYTES_PER_SAMPLE]) {

    uint32_t raw_value = ((uint32_t)axis_bytes[1] << BYTE_BITS) |
                         (uint32_t)axis_bytes[0];

    /* Sign extension by subtraction: the conversion of a uint32_t above
     * INT16_MAX to int16_t is implementation-defined (GEN-03). */
    if (raw_value > (uint32_t)INT16_MAX) {
        return (int16_t)((int32_t)raw_value - INT16_RANGE);
    }

    return (int16_t)raw_value;
}

/**
 * @brief Divides rounding to the nearest integer, halfway cases away from
 *        zero.
 *
 * @param[in] dividend Dividend.
 * @param[in] divisor Divisor, > 0 (only constants from this module are
 *            passed).
 *
 * @return Rounded quotient.
 */
static int32_t DivideRounded(const int32_t dividend, const int32_t divisor) {

    int32_t half_divisor = divisor / 2;

    if (dividend >= 0) {
        return (dividend + half_divisor) / divisor;
    }

    return (dividend - half_divisor) / divisor;
}
