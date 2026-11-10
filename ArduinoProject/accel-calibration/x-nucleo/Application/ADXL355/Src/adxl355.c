/**
 * @file adxl355.c
 * @brief Implementation of the ADXL355 accelerometer driver.
 *
 * @author Carlos Mendes
 *
 * @date
 *
 */

#include "adxl355.h"

#include <stddef.h>

// The sample set functions transfer the struct as a byte buffer (TYP-20)
_Static_assert(sizeof(ADXL355_RawSampleSet_t) == ADXL355_BYTES_PER_SAMPLE_SET,
               "ADXL355_RawSampleSet_t must not contain padding");

static bool HasReadFunc(const ADXL355_Platform_t *platform) {
    return (platform != NULL) &&
           (platform->read != NULL);
}

static bool HasWriteFunc(const ADXL355_Platform_t *platform) {
    return (platform != NULL) &&
           (platform->write != NULL);
}

static bool IsAxisValid(const ADXL355_Axis_t axis) {
    return (axis == ADXL355_X_AXIS) || (axis == ADXL355_Y_AXIS) ||
           (axis == ADXL355_Z_AXIS);
}

ADXL355_Status_t ADXL355_ReadRegister(const ADXL355_Platform_t *platform,
                                      const uint8_t reg_address, uint8_t *buffer) {

    ADXL355_Status_t ret_status;

    if (!HasReadFunc(platform) || (buffer == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, buffer, 1);
    return ret_status;
}

ADXL355_Status_t ADXL355_WriteRegister(const ADXL355_Platform_t *platform,
                                       const uint8_t reg_address,
                                       const uint8_t data) {

    ADXL355_Status_t ret_status;

    if (!HasWriteFunc(platform)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->write(reg_address, &data, 1);
    return ret_status;
}

ADXL355_Status_t ADXL355_ReadRegisterMasked(
const ADXL355_Platform_t *platform, const uint8_t reg_address,
const uint8_t mask, uint8_t *buffer) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (!HasReadFunc(platform) || (buffer == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, &register_data,
                                1);

    if (ret_status == ADXL355_OK) {
        *buffer = (uint8_t)(register_data & mask);
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_WriteRegisterMasked(
const ADXL355_Platform_t *platform, const uint8_t reg_address,
const uint8_t mask, const uint8_t data) {

    uint8_t old_reg_data;
    uint8_t new_reg_data;
    ADXL355_Status_t ret_status;

    if (!HasReadFunc(platform) || !HasWriteFunc(platform)) {
        return ADXL355_INVALID_PARAMETER;
    }

    // Bits of data outside mask would corrupt the other register fields
    if (((uint32_t)data & ~(uint32_t)mask) != 0U) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, &old_reg_data,
                                1);

    if (ret_status == ADXL355_OK) {
        new_reg_data = (uint8_t)((old_reg_data & (uint8_t)~(uint32_t)mask) |
                                 data);
        ret_status = platform->write(reg_address,
                                     &new_reg_data, 1);
    }

    return ret_status;
}

// Device ID functions

ADXL355_Status_t ADXL355_GetDevIdAd(const ADXL355_Platform_t *platform,
                                    uint8_t *dev_id_ad) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_DEVID_AD,
                                            ADXL355_ID_MASK,
                                            dev_id_ad);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetDevIdMst(const ADXL355_Platform_t *platform,
                                     uint8_t *dev_id_mst) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_DEVID_MST,
                                            ADXL355_ID_MASK,
                                            dev_id_mst);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetPartId(const ADXL355_Platform_t *platform,
                                   uint8_t *dev_part_id) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_PARTID,
                                            ADXL355_ID_MASK,
                                            dev_part_id);
    return ret_status;
}

ADXL355_Status_t ADXL355_GetRevId(const ADXL355_Platform_t *platform,
                                  uint8_t *dev_rev_id) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_REVID,
                                            ADXL355_ID_MASK,
                                            dev_rev_id);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetAllIds(const ADXL355_Platform_t *platform,
                                   uint8_t *adxl_id) {

    ADXL355_Status_t ret_status;

    if (!HasReadFunc(platform) || (adxl_id == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_DEVID_AD, adxl_id,
                                ADXL355_TOTAL_ID_REGISTERS);

    return ret_status;
}

// Temperature functions

ADXL355_Status_t ADXL355_ReadRawTemperature(
const ADXL355_Platform_t *platform, uint16_t *temperature) {

    ADXL355_Status_t ret_status;

    uint8_t temperature_reg[2] = {0xFFU, 0xFFU};

    if (!HasReadFunc(platform) || (temperature == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_TEMP2,
                                temperature_reg, 2);

    if (ret_status == ADXL355_OK) {
        // TEMP2 holds bits 11:8 in its low nibble, TEMP1 holds bits 7:0
        *temperature = (uint16_t)((((uint16_t)temperature_reg[0] & 0x0FU)
                                   << 8U) | temperature_reg[1]);
    }

    return ret_status;
}

// Acceleration data functions

ADXL355_Status_t ADXL355_ReadRawSampleSet(
const ADXL355_Platform_t *platform,
ADXL355_RawSampleSet_t *raw_sample_set) {

    ADXL355_Status_t ret_status;

    if (!HasReadFunc(platform) || (raw_sample_set == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_XDATA3,
                                (uint8_t *)raw_sample_set,
                                ADXL355_BYTES_PER_SAMPLE_SET);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetAxisOffset(const ADXL355_Platform_t *platform,
                                       const ADXL355_Axis_t axis,
                                       uint8_t axis_offset[2]) {

    ADXL355_Status_t ret_status;
    uint8_t address;

    if (!HasReadFunc(platform) || (axis_offset == NULL) ||
        !IsAxisValid(axis)) {
        return ADXL355_INVALID_PARAMETER;
    }

    address = (uint8_t)(ADXL355_REG_OFFSET_X_H + (uint8_t)axis);

    ret_status = platform->read(address, axis_offset, 2);

    return ret_status;
}

ADXL355_Status_t ADXL355_SetAxisOffset(const ADXL355_Platform_t *platform,
                                       const ADXL355_Axis_t axis,
                                       const int16_t offset) {

    ADXL355_Status_t ret_status;

    uint8_t buffer[2] = {(uint8_t)((uint16_t)offset >> 8U),
                         (uint8_t)((uint16_t)offset & 0xFFU)};

    if (!HasWriteFunc(platform) || !IsAxisValid(axis)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->write(
                                 (uint8_t)(ADXL355_REG_OFFSET_X_H + (uint8_t)axis), buffer, 2);

    return ret_status;
}

// FIFO Functions

ADXL355_Status_t ADXL355_GetFifoSamples(
const ADXL355_Platform_t *platform, uint8_t *fifo_samples) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_FIFO_SAMPLES,
                                            ADXL355_FIFO_SAMPLES_MASK,
                                            fifo_samples);

    return ret_status;
}

ADXL355_Status_t ADXL355_SetFifoSamples(
const ADXL355_Platform_t *platform, const uint8_t fifo_max_entries) {

    ADXL355_Status_t ret_status;

    // According to documentation, FIFO_SAMPLES stores ENTRIES (1 to 96)

    if ((fifo_max_entries == 0U) ||
        (fifo_max_entries > ADXL355_FIFO_MAX_SAMPLES)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_FIFO_SAMPLES,
                                             ADXL355_FIFO_SAMPLES_MASK,
                                             fifo_max_entries);

    return ret_status;
}

ADXL355_Status_t ADXL355_ReadNumberFifoEntries(
const ADXL355_Platform_t *platform, uint8_t *num_fifo_entries) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_FIFO_ENTRIES,
                                            ADXL355_FIFO_ENTRIES_MASK,
                                            num_fifo_entries);

    return ret_status;
}

ADXL355_Status_t ADXL355_ReadFifo(const ADXL355_Platform_t *platform,
                                  uint8_t *fifo_data, const uint8_t num_entries) {

    ADXL355_Status_t ret_status;
    uint16_t bytes_to_read = (uint16_t)(num_entries * ADXL355_BYTES_PER_SAMPLE);

    if (!HasReadFunc(platform) || (fifo_data == NULL) ||
        (num_entries == 0U) || (num_entries > ADXL355_FIFO_MAX_SAMPLES)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_FIFO_DATA,
                                fifo_data, bytes_to_read);

    return ret_status;
}

ADXL355_Status_t ADXL355_ReadFifoSampleSet(
const ADXL355_Platform_t *platform,
ADXL355_RawSampleSet_t *sample_sets, const uint8_t num_sample_sets) {

    ADXL355_Status_t ret_status;
    uint16_t bytes_to_read = (uint16_t)(num_sample_sets *
                                        ADXL355_BYTES_PER_SAMPLE_SET);

    if (!HasReadFunc(platform) || (sample_sets == NULL) ||
        (num_sample_sets == 0U) ||
        (num_sample_sets > ADXL355_FIFO_MAX_SAMPLE_SETS)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_FIFO_DATA,
                                (uint8_t *)sample_sets,
                                bytes_to_read);

    return ret_status;
}

ADXL355_Status_t ADXL355_ReadFifoDMA(ADXL355_StartRead_t start_read,
                                     uint8_t *fifo_data,
                                     const uint8_t num_entries) {

    uint16_t bytes_to_read = (uint16_t)(num_entries * ADXL355_BYTES_PER_SAMPLE);

    if ((start_read == NULL) || (fifo_data == NULL) ||
        (num_entries == 0U) || (num_entries > ADXL355_FIFO_MAX_SAMPLES)) {
        return ADXL355_INVALID_PARAMETER;
    }

    // Only starts the transfer: fifo_data is filled after this returns
    return start_read(ADXL355_REG_FIFO_DATA, fifo_data, bytes_to_read);
}

// Activity functions

ADXL355_Status_t ADXL355_GetActivityThreshold(
const ADXL355_Platform_t *platform, uint16_t *act_threshold) {

    ADXL355_Status_t ret_status;
    uint8_t buffer[2];

    if (!HasReadFunc(platform) || (act_threshold == NULL)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->read(ADXL355_REG_ACT_THRESH_H,
                                buffer, 2);
    if (ret_status == ADXL355_OK) {
        *act_threshold = (uint16_t)(((uint16_t)buffer[0] << 8U) | buffer[1]);
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_SetActivityThreshold(
const ADXL355_Platform_t *platform, const uint16_t act_threshold) {

    ADXL355_Status_t ret_status;

    uint8_t buffer[2] = {(uint8_t)(act_threshold >> 8U),
                         (uint8_t)(act_threshold & 0xFFU)};

    if (!HasWriteFunc(platform)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->write(ADXL355_REG_ACT_THRESH_H,
                                 buffer, 2);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetActivityCount(
const ADXL355_Platform_t *platform, uint8_t *act_count) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_ACT_COUNT,
                                            ADXL355_ACT_COUNT_MASK,
                                            act_count);

    return ret_status;
}

ADXL355_Status_t ADXL355_SetActivityCount(
const ADXL355_Platform_t *platform, const uint8_t act_count) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_ACT_COUNT,
                                             ADXL355_ACT_COUNT_MASK,
                                             act_count);

    return ret_status;
}

ADXL355_Status_t ADXL355_EnableActivity(
const ADXL355_Platform_t *platform,
const ADXL355_ActivityMode_t act_mode) {

    ADXL355_Status_t ret_status;

    if ((uint32_t)act_mode > (uint32_t)ADXL355_ACTIVITY_X_ON_Y_ON_Z_ON) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_ACT_EN,
                                             ADXL355_ACT_EN_MASK,
                                             (uint8_t)act_mode);
    return ret_status;
}

ADXL355_Status_t ADXL355_DisableActivity(
const ADXL355_Platform_t *platform) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_ACT_EN,
                                             ADXL355_ACT_EN_MASK,
                                             (uint8_t)ADXL355_ACTIVITY_OFF);
    return ret_status;
}

// Interrupts

ADXL355_Status_t ADXL355_SetInterruptPolarity(
const ADXL355_Platform_t *platform, const ADXL355_IntPol_t int_pol) {

    ADXL355_Status_t ret_status;

    if ((int_pol != ADXL355_INT_POL_LOW) && (int_pol != ADXL355_INT_POL_HIGH)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_RANGE,
                                             ADXL355_INT_POL_MASK,
                                             (uint8_t)int_pol);
    return ret_status;
}

ADXL355_Status_t ADXL355_GetInterruptPolarity(
const ADXL355_Platform_t *platform, ADXL355_IntPol_t *int_pol) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (int_pol == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_RANGE,
                                            ADXL355_INT_POL_MASK,
                                            &register_data);
    if (ret_status == ADXL355_OK) {
        *int_pol = (ADXL355_IntPol_t)register_data;
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_SetInterruptMap(
const ADXL355_Platform_t *platform, const uint8_t int_map) {

    ADXL355_Status_t ret_status;

    if (!HasWriteFunc(platform)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->write(ADXL355_REG_INT_MAP, &int_map,
                                 1);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetInterruptMap(
const ADXL355_Platform_t *platform, uint8_t *int_map) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_ReadRegister(platform, ADXL355_REG_INT_MAP,
                                      int_map);

    return ret_status;
}

ADXL355_Status_t ADXL355_ClearInterrupts(
const ADXL355_Platform_t *platform) {

    ADXL355_Status_t ret_status;
    uint8_t spi_data = 0x00U;

    if (!HasWriteFunc(platform)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = platform->write(ADXL355_REG_INT_MAP,
                                 &spi_data, 1);

    return ret_status;
}

ADXL355_Status_t ADXL355_EnableInterrupt(
const ADXL355_Platform_t *platform,
const ADXL355_IntSource_t int_src) {

    ADXL355_Status_t ret_status;

    // Mask = int_src: only the given sources are set, the others are kept
    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_INT_MAP,
                                             (uint8_t)int_src,
                                             (uint8_t)int_src);
    return ret_status;
}

ADXL355_Status_t ADXL355_DisableInterrupt(
const ADXL355_Platform_t *platform,
const ADXL355_IntSource_t int_src) {

    ADXL355_Status_t ret_status;

    // Any int_src is valid. No need to check parameter validity
    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_INT_MAP,
                                             (uint8_t)int_src, 0U);

    return ret_status;
}

// ADXL355 operation (ODRs, FILTER, RANGE)

ADXL355_Status_t ADXL355_GetRange(const ADXL355_Platform_t *platform,
                                  ADXL355_Range_t *range) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (range == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_RANGE,
                                            ADXL355_RANGE_MASK,
                                            &register_data);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // 0x00 is a reserved value of the RANGE field
    if (register_data < (uint8_t)ADXL355_RANGE_2G) {
        return ADXL355_UNEXPECTED_READ_VALUE;
    }

    *range = (ADXL355_Range_t)register_data;

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_SetRange(const ADXL355_Platform_t *platform,
                                  const ADXL355_Range_t range) {

    ADXL355_Status_t ret_status;

    if ((range != ADXL355_RANGE_2G) && (range != ADXL355_RANGE_4G) &&
        (range != ADXL355_RANGE_8G)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_RANGE,
                                             ADXL355_RANGE_MASK,
                                             (uint8_t)range);
    return ret_status;
}

ADXL355_Status_t ADXL355_GetODR(const ADXL355_Platform_t *platform,
                                ADXL355_ODR_t *odr) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (odr == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_FILTER,
                                            ADXL355_ODR_MASK,
                                            &register_data);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // 0x0B to 0x0F are reserved values of the ODR_LPF field
    if (register_data > (uint8_t)ADXL355_ODR_3_906Hz) {
        return ADXL355_UNEXPECTED_READ_VALUE;
    }

    *odr = (ADXL355_ODR_t)register_data;

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_SetODR(const ADXL355_Platform_t *platform,
                                const ADXL355_ODR_t odr) {

    ADXL355_Status_t ret_status;

    if ((uint32_t)odr > (uint32_t)ADXL355_ODR_3_906Hz) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_FILTER,
                                             ADXL355_ODR_MASK,
                                             (uint8_t)odr);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetHPF(const ADXL355_Platform_t *platform,
                                ADXL355_HPF_t *hpf) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (hpf == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_FILTER,
                                            ADXL355_HPF_MASK,
                                            &register_data);
    if (ret_status != ADXL355_OK) {
        return ret_status;
    }

    // 0x70 is a reserved value of the HPF_CORNER field
    if (register_data > (uint8_t)ADXL355_HPF_6) {
        return ADXL355_UNEXPECTED_READ_VALUE;
    }

    *hpf = (ADXL355_HPF_t)register_data;

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_SetHPF(const ADXL355_Platform_t *platform,
                                const ADXL355_HPF_t hpf) {

    ADXL355_Status_t ret_status;

    // Valid values are 0x00 to 0x60 with the low nibble clear
    if (((uint32_t)hpf > (uint32_t)ADXL355_HPF_6) ||
        (((uint32_t)hpf & ~(uint32_t)ADXL355_HPF_MASK) != 0U)) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_FILTER,
                                             ADXL355_HPF_MASK,
                                             (uint8_t)hpf);

    return ret_status;
}

ADXL355_Status_t ADXL355_GetMode(const ADXL355_Platform_t *platform,
                                 ADXL355_MeasurementMode_t *adxl_mode) {

    ADXL355_Status_t ret_status;
    uint8_t register_data;

    if (adxl_mode == NULL) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_ReadRegisterMasked(platform,
                                            ADXL355_REG_POWER_CTL,
                                            ADXL355_POWER_CTL_MASK,
                                            &register_data);
    if (ret_status == ADXL355_OK) {
        *adxl_mode = (ADXL355_MeasurementMode_t)register_data;
    }

    return ret_status;
}

ADXL355_Status_t ADXL355_SetMode(const ADXL355_Platform_t *platform,
                                 const ADXL355_MeasurementMode_t adxl_mode) {

    ADXL355_Status_t ret_status;

    if ((uint32_t)adxl_mode > (uint32_t)ADXL355_STDBY_TEMP_OFF_DRDY_OFF) {
        return ADXL355_INVALID_PARAMETER;
    }

    ret_status = ADXL355_WriteRegisterMasked(platform,
                                             ADXL355_REG_POWER_CTL,
                                             ADXL355_POWER_CTL_MASK,
                                             (uint8_t)adxl_mode);

    return ret_status;
}

ADXL355_Status_t ADXL355_SoftReset(const ADXL355_Platform_t *platform) {

    ADXL355_Status_t ret_status;

    ret_status = ADXL355_WriteRegister(platform, ADXL355_REG_RESET,
                                       ADXL355_RESET_CODE);

    return ret_status;
}
