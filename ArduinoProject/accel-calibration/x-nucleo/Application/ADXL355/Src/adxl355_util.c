/**
 * @file adxl355_util.c
 * @brief Implementation of the ADXL355 utility functions.
 *
 * @author Carlos Mendes
 * @date Jan/2026
 */

#include "adxl355_util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adxl355.h"

#define MG_PER_G                1000U   // mg in 1 g
#define MDEGC_PER_100DEGC       100000  // mdegC in 100 degC

/* The mg to register conversions divide the sensitivity by
 * MG_PER_G * weight before multiplying, which is exact only if this divides
 * every sensitivity. The ACT_THRESH weight divides the OFFSET weight, so
 * checking the OFFSET weight covers both. */
#define SENSITIVITY_DIVISOR     (MG_PER_G * ADXL355_OFFSET_LSB_WEIGHT)
_Static_assert(((ADXL355_OFFSET_LSB_WEIGHT %
                 ADXL355_ACT_THRESH_LSB_WEIGHT) == 0U) &&
               ((ADXL355_SENSITIVITY_2G % SENSITIVITY_DIVISOR) == 0U) &&
               ((ADXL355_SENSITIVITY_4G % SENSITIVITY_DIVISOR) == 0U) &&
               ((ADXL355_SENSITIVITY_8G % SENSITIVITY_DIVISOR) == 0U),
               "mg to register conversions must be exact");

static ADXL355_Status_t GetSensitivity(const ADXL355_Range_t range,
                                       uint32_t *sensitivity_lsb_per_g);
static int32_t UnpackAxis(const uint8_t axis_bytes[ADXL355_BYTES_PER_SAMPLE]);

ADXL355_Status_t ADXL355_UTIL_Config(const ADXL355_UTIL_DevCfg_t *dev_cfg) {

    ADXL355_Status_t ret_status = ADXL355_OK;

    if (dev_cfg == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    // Config modifications must be done in STANDBY mode
    ret_status = ADXL355_SetMode(dev_cfg->platform,
                                 ADXL355_STDBY_TEMP_OFF_DRDY_OFF);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // ===================== Configure ADXL355 =====================

    ret_status = ADXL355_SetODR(dev_cfg->platform, dev_cfg->odr);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetHPF(dev_cfg->platform, dev_cfg->hpf);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetRange(dev_cfg->platform, dev_cfg->range);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetInterruptPolarity(dev_cfg->platform,
                                              dev_cfg->int_pol);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetInterruptMap(dev_cfg->platform, dev_cfg->int_src);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetActivityThreshold(dev_cfg->platform,
                                              dev_cfg->act_thr_lsb);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetActivityCount(dev_cfg->platform,
                                          dev_cfg->act_cnt);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_EnableActivity(dev_cfg->platform, dev_cfg->act_mode);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetFifoSamples(dev_cfg->platform,
                                        dev_cfg->fifo_samples);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetAxisOffset(dev_cfg->platform, ADXL355_X_AXIS,
                                       dev_cfg->x_offset_lsb);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetAxisOffset(dev_cfg->platform, ADXL355_Y_AXIS,
                                       dev_cfg->y_offset_lsb);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    ret_status = ADXL355_SetAxisOffset(dev_cfg->platform, ADXL355_Z_AXIS,
                                       dev_cfg->z_offset_lsb);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // ===================== Start Measurement mode =====================

    ret_status = ADXL355_SetMode(dev_cfg->platform, dev_cfg->meas_mode);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_UTIL_UnpackSampleSets(
const ADXL355_RawSampleSet_t *raw_sample_sets, const size_t num_sample_sets,
ADXL355_UTIL_UnpackedSampleSet_t *unpacked_sample_sets) {

    if ((raw_sample_sets == NULL) || (unpacked_sample_sets == NULL) ||
        (num_sample_sets == 0U)) {
        return ADXL355_INVALID_PARAMETER;
    }

    for (size_t i = 0U; i < num_sample_sets; ++i) {
        unpacked_sample_sets[i].x_axis = UnpackAxis(raw_sample_sets[i].x_axis);
        unpacked_sample_sets[i].y_axis = UnpackAxis(raw_sample_sets[i].y_axis);
        unpacked_sample_sets[i].z_axis = UnpackAxis(raw_sample_sets[i].z_axis);
    }

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_UTIL_SampleSets2Gs(
const ADXL355_UTIL_UnpackedSampleSet_t *unpacked_sample_sets,
const ADXL355_Range_t range, const size_t num_sample_sets,
ADXL355_UTIL_SampleSetGs_t *sample_sets_gs) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint32_t sensitivity_lsb_per_g = 0U;
    float sensitivity = 0.0f;

    if ((unpacked_sample_sets == NULL) || (sample_sets_gs == NULL) ||
        (num_sample_sets == 0U)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = GetSensitivity(range, &sensitivity_lsb_per_g);
    if (ret_status != ADXL355_OK) {
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

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_UTIL_ClearFifo(
const ADXL355_Platform_t *platform) {

    uint8_t fifo_data[ADXL355_FIFO_MAX_BYTES] = {0};

    return ADXL355_ReadFifo(platform, fifo_data,
                            ADXL355_FIFO_MAX_SAMPLES);
}

ADXL355_Status_t ADXL355_UTIL_ActivityThreshold2LSB(
const uint16_t threshold_mg, const ADXL355_Range_t range,
uint16_t *threshold_lsb) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint32_t sensitivity_lsb_per_g = 0U;
    uint32_t register_lsb_per_mg = 0U;
    uint32_t value_lsb = 0U;

    if (threshold_lsb == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = GetSensitivity(range, &sensitivity_lsb_per_g);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // Exact division (see the _Static_assert); max product 65535 * 32
    register_lsb_per_mg = sensitivity_lsb_per_g /
                          (MG_PER_G * ADXL355_ACT_THRESH_LSB_WEIGHT);
    value_lsb = (uint32_t)threshold_mg * register_lsb_per_mg;

    if (value_lsb > UINT16_MAX) {
        return ADXL355_INVALID_PARAMETER;
    }

    *threshold_lsb = (uint16_t)value_lsb;

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_UTIL_Offset2LSB(const int16_t offset_mg,
                                         const ADXL355_Range_t range,
                                         int16_t *offset_lsb) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint32_t sensitivity_lsb_per_g = 0U;
    int32_t register_lsb_per_mg = 0;
    int32_t value_lsb = 0;

    if (offset_lsb == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = GetSensitivity(range, &sensitivity_lsb_per_g);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // Exact division (see the _Static_assert); max product 32768 * 16
    register_lsb_per_mg = (int32_t)(sensitivity_lsb_per_g /
                                    SENSITIVITY_DIVISOR);
    value_lsb = (int32_t)offset_mg * register_lsb_per_mg;

    if ((value_lsb < INT16_MIN) || (value_lsb > INT16_MAX)) {
        return ADXL355_INVALID_PARAMETER;
    }

    *offset_lsb = (int16_t)value_lsb;

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_UTIL_AlignFifo(
const ADXL355_Platform_t *platform, const uint16_t max_reads) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint8_t fifo_entry[ADXL355_BYTES_PER_SAMPLE] = {0};
    bool is_x_axis = false;
    bool is_empty = true;
    bool has_x_axis = false;
    uint8_t num_yz_entries = 0U;

    if ((platform == NULL) ||
        (platform->read == NULL) ||
        (max_reads < ADXL355_SAMPLES_PER_SAMPLE_SET)) {
        return ADXL355_INVALID_PARAMETER;
    }

    for (uint16_t reads = 0U; reads < max_reads; ++reads) {
        ret_status = platform->read(ADXL355_REG_FIFO_DATA,
                                    fifo_entry,
                                    ADXL355_BYTES_PER_SAMPLE);
        if (ret_status != ADXL355_OK) {
            return ret_status;
        }

        is_x_axis = ((fifo_entry[ADXL355_DATA1_BYTE] &
                      ADXL355_FIFO_X_MARKER_MASK) != 0U);
        is_empty = ((fifo_entry[ADXL355_DATA1_BYTE] &
                     ADXL355_FIFO_EMPTY_MASK) != 0U);

        if (is_empty) {
            // No new data yet: read again
        } else if (!has_x_axis) {
            has_x_axis = is_x_axis;
        } else if (!is_x_axis) {
            ++num_yz_entries;
            // The X entry is followed by the Y and Z entries
            if (num_yz_entries == (ADXL355_SAMPLES_PER_SAMPLE_SET - 1U)) {
                return ADXL355_OK;
            }
        } else {
            // New X entry (an entry was lost): restart the set from it
            num_yz_entries = 0U;
        }
    }

    return ADXL355_TIMEOUT;
}

ADXL355_Status_t ADXL355_UTIL_ReadTemperature(
const ADXL355_Platform_t *platform, float *temperature_degc) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint16_t raw_temperature_lsb = 0U;

    if (temperature_degc == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRawTemperature(platform,
                                            &raw_temperature_lsb);

    if (ret_status == ADXL355_OK) {
        // y = m(x-x0) + y0
        *temperature_degc = (float)ADXL355_REFERENCE_TEMPERATURE +
                            (((float)raw_temperature_lsb -
                              (float)ADXL355_REFERENCE_TEMPERATURE_LSB) /
                             (float)ADXL355_TEMPERATURE_SLOPE);
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_UTIL_ReadTemperatureMdegc(
const ADXL355_Platform_t *platform, int32_t *temperature_mdegc) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint16_t raw_temperature_lsb = 0U;
    int32_t delta_lsb = 0;

    if (temperature_mdegc == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRawTemperature(platform,
                                            &raw_temperature_lsb);

    if (ret_status == ADXL355_OK) {
        /* y = m(x-x0) + y0, with m in LSB per 100 degC and y in mdegC.
         * |delta_lsb * 100000| < 2^31 for the 12-bit raw range. */
        delta_lsb = (int32_t)raw_temperature_lsb -
                    (int32_t)ADXL355_REFERENCE_TEMPERATURE_LSB;
        *temperature_mdegc = ADXL355_REFERENCE_TEMPERATURE_MDEGC +
                             ((delta_lsb * MDEGC_PER_100DEGC) /
                              ADXL355_TEMPERATURE_SLOPE_LSB_PER_100DEGC);
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_UTIL_ResetAndVerify(
const ADXL355_Platform_t *platform,
const uint8_t stored_shadow_registers[ADXL355_TOTAL_SHADOW_REGISTERS],
const uint8_t max_reset_retries) {

    ADXL355_Status_t ret_status = ADXL355_OK;
    uint8_t nvm_status = ADXL355_NVM_BUSY_MASK;
    uint8_t read_shadow_registers[ADXL355_TOTAL_SHADOW_REGISTERS] = {0};

    if ((platform == NULL) ||
        (platform->read == NULL) ||
        (platform->write == NULL) ||
        (platform->delay_ms == NULL) ||
        (stored_shadow_registers == NULL) || (max_reset_retries == 0U)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_SoftReset(platform);

    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // Wait for NVM_BUSY = 0
    uint8_t retries = 0U;
    for (; retries < max_reset_retries; ++retries) {
        platform->delay_ms(ADXL355_NVM_POLL_PERIOD_MS);
        ret_status = ADXL355_ReadRegisterMasked(platform,
                                                ADXL355_REG_STATUS,
                                                ADXL355_NVM_BUSY_MASK,
                                                &nvm_status);
        if (ret_status != ADXL355_OK) {
            return ret_status;
        }

        if (nvm_status != ADXL355_NVM_BUSY_MASK) {
            break;
        }
    }

    if ((retries == max_reset_retries) &&
        (nvm_status == ADXL355_NVM_BUSY_MASK)) {
        return ADXL355_TIMEOUT;
    }

    // Delay is needed between soft reset command and shadow registers reading
    // TODO(cmendes): What is the correct delay?
    platform->delay_ms(ADXL355_SHADOW_READ_DELAY_MS);

    ret_status = platform->read(ADXL355_REG_SHADOW, read_shadow_registers,
                                ADXL355_TOTAL_SHADOW_REGISTERS);
    if (ret_status != ADXL355_OK) {
        return ADXL355_RESET_ERROR;
    }

    for (uint8_t idx = 0U; idx < ADXL355_TOTAL_SHADOW_REGISTERS; ++idx) {
        if (read_shadow_registers[idx] != stored_shadow_registers[idx]) {
            return ADXL355_RESET_ERROR;
        }
    }

    return ADXL355_OK;
}

/**
 * @brief Gets the sensitivity of a measurement range.
 *
 * @param[in] range Measurement range.
 * @param[out] sensitivity_lsb_per_g Sensitivity in LSB/g of the 20-bit data.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER range is not a valid ADXL355_Range_t.
 */
static ADXL355_Status_t GetSensitivity(const ADXL355_Range_t range,
                                       uint32_t *sensitivity_lsb_per_g) {

    switch (range) {
        case ADXL355_RANGE_2G:
            *sensitivity_lsb_per_g = ADXL355_SENSITIVITY_2G;
            break;
        case ADXL355_RANGE_4G:
            *sensitivity_lsb_per_g = ADXL355_SENSITIVITY_4G;
            break;
        case ADXL355_RANGE_8G:
            *sensitivity_lsb_per_g = ADXL355_SENSITIVITY_8G;
            break;
        default:
            return ADXL355_INVALID_PARAMETER;
    }

    return ADXL355_OK;
}

/**
 * @brief Converts one axis from the register format to a signed value.
 *
 * @param[in] axis_bytes DATA3, DATA2 and DATA1 of the axis: 20-bit two's
 *            complement value, left aligned, MSB first.
 *
 * @return Sign-extended value in LSB (-2^19 to 2^19 - 1).
 */
static int32_t UnpackAxis(const uint8_t axis_bytes[ADXL355_BYTES_PER_SAMPLE]) {

    uint32_t raw_value =
        ((uint32_t)axis_bytes[ADXL355_DATA3_BYTE] << ADXL355_DATA3_SHIFT) |
        ((uint32_t)axis_bytes[ADXL355_DATA2_BYTE] << ADXL355_DATA2_SHIFT) |
        ((uint32_t)axis_bytes[ADXL355_DATA1_BYTE] >> ADXL355_DATA1_SHIFT);
    // Exact: raw_value is below 2^ADXL355_DATA_BITS
    int32_t value = (int32_t)raw_value;

    /* Sign extension by subtraction instead of OR-ing the upper bits: the
     * conversion of a uint32_t above INT32_MAX to int32_t is
     * implementation-defined (GEN-03). */
    if ((raw_value & (1UL << (ADXL355_DATA_BITS - 1U))) != 0U) {
        value -= (int32_t)(1UL << ADXL355_DATA_BITS);
    }

    return value;
}
