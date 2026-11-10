/**
 * @file adxl345.c
 * @brief Implementation of the ADXL345 accelerometer driver.
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 *
 */

#include "adxl345.h"

#include <stddef.h>

// First and last addresses of the register block after the reserved ones
#define FIRST_CONFIG_REGISTER   ADXL345_REG_THRESH_TAP
#define LAST_REGISTER           ADXL345_REG_FIFO_STATUS

// Two's complement decoding of an 8-bit register
#define INT8_RANGE              256

// The sample set functions transfer the struct as a byte buffer (TYP-20)
_Static_assert(sizeof(ADXL345_RawSampleSet_t) == ADXL345_BYTES_PER_SAMPLE_SET,
               "ADXL345_RawSampleSet_t must not contain padding");

static bool HasReadFunc(const ADXL345_Platform_t *platform) {
    return (platform != NULL) &&
           (platform->read != NULL);
}

static bool HasWriteFunc(const ADXL345_Platform_t *platform) {
    return (platform != NULL) &&
           (platform->write != NULL);
}

// 0x01 to 0x1C are reserved and must not be accessed (DS p.23 Table 19)
static bool IsRegisterValid(const uint8_t reg_address) {
    return (reg_address == ADXL345_REG_DEVID) ||
           ((reg_address >= FIRST_CONFIG_REGISTER) &&
            (reg_address <= LAST_REGISTER));
}

static bool IsAxisValid(const ADXL345_Axis_t axis) {
    return (axis == ADXL345_X_AXIS) || (axis == ADXL345_Y_AXIS) ||
           (axis == ADXL345_Z_AXIS);
}

static bool IsAxesValid(const uint8_t axes) {
    return ((uint32_t)axes & ~(uint32_t)ADXL345_AXES_MASK) == 0U;
}

static bool IsCouplingValid(const ADXL345_Coupling_t coupling) {
    return (coupling == ADXL345_DC_COUPLED) ||
           (coupling == ADXL345_AC_COUPLED);
}

static bool IsPinValid(const ADXL345_IntPin_t pin) {
    return (pin == ADXL345_INT1) || (pin == ADXL345_INT2);
}

// Decodes a two's complement byte without implementation-defined conversions
static int8_t ToInt8(const uint8_t value) {
    if (value > (uint8_t)INT8_MAX) {
        return (int8_t)((int32_t)value - INT8_RANGE);
    }

    return (int8_t)value;
}

ADXL345_Status_t ADXL345_ReadRegister(const ADXL345_Platform_t *platform,
                                      const uint8_t reg_address,
                                      uint8_t *buffer) {

    ADXL345_Status_t ret_status;

    if (!HasReadFunc(platform) || (buffer == NULL) ||
        !IsRegisterValid(reg_address)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, buffer, 1);
    return ret_status;
}

ADXL345_Status_t ADXL345_WriteRegister(const ADXL345_Platform_t *platform,
                                       const uint8_t reg_address,
                                       const uint8_t data) {

    ADXL345_Status_t ret_status;

    if (!HasWriteFunc(platform) || !IsRegisterValid(reg_address)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = platform->write(reg_address, &data, 1);
    return ret_status;
}

ADXL345_Status_t ADXL345_ReadRegisterMasked(const ADXL345_Platform_t *platform,
                                            const uint8_t reg_address,
                                            const uint8_t mask,
                                            uint8_t *buffer) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!HasReadFunc(platform) || (buffer == NULL) ||
        !IsRegisterValid(reg_address)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, &register_data, 1);

    if (ret_status == ADXL345_OK) {
        *buffer = (uint8_t)(register_data & mask);
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_WriteRegisterMasked(const ADXL345_Platform_t *platform,
                                             const uint8_t reg_address,
                                             const uint8_t mask,
                                             const uint8_t data) {

    uint8_t old_reg_data;
    uint8_t new_reg_data;
    ADXL345_Status_t ret_status;

    if (!HasReadFunc(platform) || !HasWriteFunc(platform) ||
        !IsRegisterValid(reg_address)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Bits of data outside mask would corrupt the other register fields
    if (((uint32_t)data & ~(uint32_t)mask) != 0U) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = platform->read(reg_address, &old_reg_data, 1);

    if (ret_status == ADXL345_OK) {
        new_reg_data = (uint8_t)((old_reg_data & (uint8_t)~(uint32_t)mask) |
                                 data);
        ret_status = platform->write(reg_address, &new_reg_data, 1);
    }

    return ret_status;
}

// Device ID functions

ADXL345_Status_t ADXL345_GetDevId(const ADXL345_Platform_t *platform,
                                  uint8_t *dev_id) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_DEVID, dev_id);

    return ret_status;
}

ADXL345_Status_t ADXL345_CheckDevId(const ADXL345_Platform_t *platform) {

    ADXL345_Status_t ret_status;
    uint8_t dev_id;

    ret_status = ADXL345_GetDevId(platform, &dev_id);
    if (ret_status != ADXL345_OK) {
        return ret_status;
    }

    if (dev_id != ADXL345_DEVID) {
        return ADXL345_UNEXPECTED_READ_VALUE;
    }

    return ADXL345_OK;
}

// Acceleration data functions

ADXL345_Status_t ADXL345_ReadRawSampleSet(
    const ADXL345_Platform_t *platform,
    ADXL345_RawSampleSet_t *raw_sample_set) {

    ADXL345_Status_t ret_status;

    if (!HasReadFunc(platform) || (raw_sample_set == NULL)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // One transfer, so the three axes belong to the same measurement
    ret_status = platform->read(ADXL345_REG_DATAX0,
                                (uint8_t *)raw_sample_set,
                                ADXL345_BYTES_PER_SAMPLE_SET);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetAxisOffset(const ADXL345_Platform_t *platform,
                                       const ADXL345_Axis_t axis,
                                       int8_t *axis_offset) {

    ADXL345_Status_t ret_status;
    uint8_t address;
    uint8_t register_data;

    if (!HasReadFunc(platform) || (axis_offset == NULL) ||
        !IsAxisValid(axis)) {
        return ADXL345_INVALID_PARAMETER;
    }

    address = (uint8_t)(ADXL345_REG_OFSX + (uint8_t)axis);

    ret_status = platform->read(address, &register_data, 1);

    if (ret_status == ADXL345_OK) {
        *axis_offset = ToInt8(register_data);
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetAxisOffset(const ADXL345_Platform_t *platform,
                                       const ADXL345_Axis_t axis,
                                       const int8_t offset) {

    ADXL345_Status_t ret_status;

    // Signed to unsigned conversion is defined: two's complement byte
    uint8_t buffer = (uint8_t)offset;

    if (!HasWriteFunc(platform) || !IsAxisValid(axis)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = platform->write((uint8_t)(ADXL345_REG_OFSX + (uint8_t)axis),
                                 &buffer, 1);

    return ret_status;
}

// FIFO Functions

ADXL345_Status_t ADXL345_GetFifoMode(const ADXL345_Platform_t *platform,
                                     ADXL345_FifoMode_t *fifo_mode) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (fifo_mode == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_FIFO_CTL,
                                            ADXL345_FIFO_MODE_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        // Every value of the 2-bit field is a mode (DS p.28 Table 37)
        *fifo_mode = (ADXL345_FifoMode_t)register_data;
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetFifoMode(const ADXL345_Platform_t *platform,
                                     const ADXL345_FifoMode_t fifo_mode) {

    ADXL345_Status_t ret_status;

    if ((fifo_mode != ADXL345_FIFO_BYPASS) &&
        (fifo_mode != ADXL345_FIFO_FIFO) &&
        (fifo_mode != ADXL345_FIFO_STREAM) &&
        (fifo_mode != ADXL345_FIFO_TRIGGER)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_FIFO_CTL,
                                             ADXL345_FIFO_MODE_MASK,
                                             (uint8_t)fifo_mode);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetFifoSamples(const ADXL345_Platform_t *platform,
                                        uint8_t *fifo_samples) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_FIFO_CTL,
                                            ADXL345_FIFO_SAMPLES_MASK,
                                            fifo_samples);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetFifoSamples(const ADXL345_Platform_t *platform,
                                        const uint8_t fifo_samples) {

    ADXL345_Status_t ret_status;

    // According to documentation, the Samples field stores SAMPLE SETS
    // (0 to 31)

    if (fifo_samples > ADXL345_FIFO_SAMPLES_MASK) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_FIFO_CTL,
                                             ADXL345_FIFO_SAMPLES_MASK,
                                             fifo_samples);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetFifoTrigger(const ADXL345_Platform_t *platform,
                                        const ADXL345_IntPin_t pin) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!IsPinValid(pin)) {
        return ADXL345_INVALID_PARAMETER;
    }

    register_data = (pin == ADXL345_INT2) ? ADXL345_FIFO_TRIGGER_MASK : 0U;

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_FIFO_CTL,
                                             ADXL345_FIFO_TRIGGER_MASK,
                                             register_data);

    return ret_status;
}

ADXL345_Status_t ADXL345_ReadNumberFifoEntries(
    const ADXL345_Platform_t *platform, uint8_t *num_fifo_entries) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_FIFO_STATUS,
                                            ADXL345_FIFO_ENTRIES_MASK,
                                            num_fifo_entries);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetFifoTriggered(const ADXL345_Platform_t *platform,
                                          bool *triggered) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (triggered == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_FIFO_STATUS,
                                            ADXL345_FIFO_TRIG_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        *triggered = (register_data != 0U);
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_ReadFifoSampleSet(const ADXL345_Platform_t *platform,
                                           ADXL345_RawSampleSet_t *sample_sets,
                                           const uint8_t num_sample_sets) {

    ADXL345_Status_t ret_status = ADXL345_OK;

    if (!HasReadFunc(platform) || (sample_sets == NULL) ||
        (num_sample_sets == 0U) ||
        (num_sample_sets > ADXL345_FIFO_MAX_READ_SETS)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // One transfer per sample set: each read of the data registers pops one
    for (uint8_t i = 0U; (i < num_sample_sets) && (ret_status == ADXL345_OK);
         i++) {
        ret_status = platform->read(ADXL345_REG_DATAX0,
                                    (uint8_t *)&sample_sets[i],
                                    ADXL345_BYTES_PER_SAMPLE_SET);
    }

    return ret_status;
}

// Activity and inactivity functions

ADXL345_Status_t ADXL345_GetActivityThreshold(
    const ADXL345_Platform_t *platform, uint8_t *act_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_THRESH_ACT,
                                      act_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetActivityThreshold(
    const ADXL345_Platform_t *platform, const uint8_t act_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_THRESH_ACT,
                                       act_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_EnableActivity(const ADXL345_Platform_t *platform,
                                        const uint8_t axes,
                                        const ADXL345_Coupling_t coupling) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!IsAxesValid(axes) || !IsCouplingValid(coupling)) {
        return ADXL345_INVALID_PARAMETER;
    }

    register_data = (uint8_t)((uint32_t)axes << ADXL345_ACT_SHIFT);

    if (coupling == ADXL345_AC_COUPLED) {
        register_data = (uint8_t)(register_data | ADXL345_ACT_AC_MASK);
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_ACT_INACT_CTL,
                                             ADXL345_ACT_MASK,
                                             register_data);
    return ret_status;
}

ADXL345_Status_t ADXL345_DisableActivity(const ADXL345_Platform_t *platform) {

    ADXL345_Status_t ret_status;

    // With all axes excluded, activity detection is off
    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_ACT_INACT_CTL,
                                             ADXL345_ACT_AXES_MASK,
                                             ADXL345_AXES_NONE);
    return ret_status;
}

ADXL345_Status_t ADXL345_GetInactivityThreshold(
    const ADXL345_Platform_t *platform, uint8_t *inact_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_THRESH_INACT,
                                      inact_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetInactivityThreshold(
    const ADXL345_Platform_t *platform, const uint8_t inact_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_THRESH_INACT,
                                       inact_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetInactivityTime(const ADXL345_Platform_t *platform,
                                           uint8_t *inact_time_s) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_TIME_INACT,
                                      inact_time_s);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetInactivityTime(const ADXL345_Platform_t *platform,
                                           const uint8_t inact_time_s) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_TIME_INACT,
                                       inact_time_s);

    return ret_status;
}

ADXL345_Status_t ADXL345_EnableInactivity(const ADXL345_Platform_t *platform,
                                          const uint8_t axes,
                                          const ADXL345_Coupling_t coupling) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!IsAxesValid(axes) || !IsCouplingValid(coupling)) {
        return ADXL345_INVALID_PARAMETER;
    }

    register_data = (uint8_t)((uint32_t)axes << ADXL345_INACT_SHIFT);

    if (coupling == ADXL345_AC_COUPLED) {
        register_data = (uint8_t)(register_data | ADXL345_INACT_AC_MASK);
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_ACT_INACT_CTL,
                                             ADXL345_INACT_MASK,
                                             register_data);
    return ret_status;
}

ADXL345_Status_t ADXL345_DisableInactivity(const ADXL345_Platform_t *platform) {

    ADXL345_Status_t ret_status;

    // With all axes excluded, inactivity detection is off
    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_ACT_INACT_CTL,
                                             ADXL345_INACT_AXES_MASK,
                                             ADXL345_AXES_NONE);
    return ret_status;
}

// Tap functions

ADXL345_Status_t ADXL345_GetTapThreshold(const ADXL345_Platform_t *platform,
                                         uint8_t *tap_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_THRESH_TAP,
                                      tap_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetTapThreshold(const ADXL345_Platform_t *platform,
                                         const uint8_t tap_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_THRESH_TAP,
                                       tap_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetTapDuration(const ADXL345_Platform_t *platform,
                                        const uint8_t duration) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_DUR, duration);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetTapLatency(const ADXL345_Platform_t *platform,
                                       const uint8_t latency) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_LATENT, latency);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetTapWindow(const ADXL345_Platform_t *platform,
                                      const uint8_t window) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_WINDOW, window);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetTapAxes(const ADXL345_Platform_t *platform,
                                    const uint8_t axes, const bool suppress) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!IsAxesValid(axes)) {
        return ADXL345_INVALID_PARAMETER;
    }

    register_data = axes;

    if (suppress) {
        register_data = (uint8_t)(register_data | ADXL345_TAP_SUPPRESS_MASK);
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_TAP_AXES,
                                             ADXL345_TAP_AXES_REG_MASK,
                                             register_data);
    return ret_status;
}

ADXL345_Status_t ADXL345_GetActTapStatus(const ADXL345_Platform_t *platform,
                                         uint8_t *status) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_ACT_TAP_STATUS,
                                      status);

    return ret_status;
}

// Free-fall functions

ADXL345_Status_t ADXL345_SetFreeFallThreshold(
    const ADXL345_Platform_t *platform, const uint8_t ff_threshold) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_THRESH_FF,
                                       ff_threshold);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetFreeFallTime(const ADXL345_Platform_t *platform,
                                         const uint8_t ff_time) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_TIME_FF, ff_time);

    return ret_status;
}

// Interrupts

ADXL345_Status_t ADXL345_SetInterruptPolarity(
    const ADXL345_Platform_t *platform, const ADXL345_IntPol_t int_pol) {

    ADXL345_Status_t ret_status;

    if ((int_pol != ADXL345_INT_POL_LOW) && (int_pol != ADXL345_INT_POL_HIGH)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_DATA_FORMAT,
                                             ADXL345_INT_INVERT_MASK,
                                             (uint8_t)int_pol);
    return ret_status;
}

ADXL345_Status_t ADXL345_GetInterruptPolarity(
    const ADXL345_Platform_t *platform, ADXL345_IntPol_t *int_pol) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (int_pol == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_DATA_FORMAT,
                                            ADXL345_INT_INVERT_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        *int_pol = (ADXL345_IntPol_t)register_data;
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetInterruptMap(const ADXL345_Platform_t *platform,
                                         const uint8_t int_map) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_INT_MAP,
                                       int_map);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetInterruptMap(const ADXL345_Platform_t *platform,
                                         uint8_t *int_map) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_INT_MAP,
                                      int_map);

    return ret_status;
}

ADXL345_Status_t ADXL345_MapInterrupt(const ADXL345_Platform_t *platform,
                                      const uint8_t int_src,
                                      const ADXL345_IntPin_t pin) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (!IsPinValid(pin)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Mask = int_src: a 1 maps the source to INT2, a 0 maps it to INT1
    register_data = (pin == ADXL345_INT2) ? int_src : 0U;

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_INT_MAP,
                                             int_src, register_data);
    return ret_status;
}

ADXL345_Status_t ADXL345_SetInterruptEnable(const ADXL345_Platform_t *platform,
                                            const uint8_t int_enable) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_INT_ENABLE,
                                       int_enable);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetInterruptEnable(const ADXL345_Platform_t *platform,
                                            uint8_t *int_enable) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_INT_ENABLE,
                                      int_enable);

    return ret_status;
}

ADXL345_Status_t ADXL345_ClearInterrupts(const ADXL345_Platform_t *platform) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_WriteRegister(platform, ADXL345_REG_INT_ENABLE,
                                       ADXL345_INT_NONE);

    return ret_status;
}

ADXL345_Status_t ADXL345_EnableInterrupt(const ADXL345_Platform_t *platform,
                                         const uint8_t int_src) {

    ADXL345_Status_t ret_status;

    // Mask = int_src: only the given sources are set, the others are kept
    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_INT_ENABLE,
                                             int_src, int_src);
    return ret_status;
}

ADXL345_Status_t ADXL345_DisableInterrupt(const ADXL345_Platform_t *platform,
                                          const uint8_t int_src) {

    ADXL345_Status_t ret_status;

    // Any int_src is valid. No need to check parameter validity
    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_INT_ENABLE,
                                             int_src, 0U);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetInterruptSource(const ADXL345_Platform_t *platform,
                                            uint8_t *int_source) {

    ADXL345_Status_t ret_status;

    ret_status = ADXL345_ReadRegister(platform, ADXL345_REG_INT_SOURCE,
                                      int_source);

    return ret_status;
}

// ADXL345 operation (ODRs, RANGE, DATA_FORMAT, POWER_CTL)

ADXL345_Status_t ADXL345_GetRange(const ADXL345_Platform_t *platform,
                                  ADXL345_Range_t *range) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (range == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_DATA_FORMAT,
                                            ADXL345_RANGE_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        // Every value of the 2-bit field is a range (DS p.27 Table 35)
        *range = (ADXL345_Range_t)register_data;
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetRange(const ADXL345_Platform_t *platform,
                                  const ADXL345_Range_t range) {

    ADXL345_Status_t ret_status;

    if ((uint32_t)range > (uint32_t)ADXL345_RANGE_16G) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_DATA_FORMAT,
                                             ADXL345_RANGE_MASK,
                                             (uint8_t)range);
    return ret_status;
}

ADXL345_Status_t ADXL345_GetODR(const ADXL345_Platform_t *platform,
                                ADXL345_ODR_t *odr) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (odr == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_BW_RATE,
                                            ADXL345_RATE_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        // Every value of the 4-bit field is a rate (DS p.13 Table 7)
        *odr = (ADXL345_ODR_t)register_data;
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetODR(const ADXL345_Platform_t *platform,
                                const ADXL345_ODR_t odr) {

    ADXL345_Status_t ret_status;

    if ((uint32_t)odr > (uint32_t)ADXL345_ODR_3200Hz) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_BW_RATE,
                                             ADXL345_RATE_MASK,
                                             (uint8_t)odr);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetLowPower(const ADXL345_Platform_t *platform,
                                     const bool low_power) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    register_data = low_power ? ADXL345_LOW_POWER_MASK : 0U;

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_BW_RATE,
                                             ADXL345_LOW_POWER_MASK,
                                             register_data);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetFullResolution(const ADXL345_Platform_t *platform,
                                           bool *full_res) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (full_res == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_DATA_FORMAT,
                                            ADXL345_FULL_RES_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        *full_res = (register_data != 0U);
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetFullResolution(const ADXL345_Platform_t *platform,
                                           const bool full_res) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    register_data = full_res ? ADXL345_FULL_RES_MASK : 0U;

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_DATA_FORMAT,
                                             ADXL345_FULL_RES_MASK,
                                             register_data);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetSelfTest(const ADXL345_Platform_t *platform,
                                     const bool self_test) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    register_data = self_test ? ADXL345_SELF_TEST_MASK : 0U;

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_DATA_FORMAT,
                                             ADXL345_SELF_TEST_MASK,
                                             register_data);

    return ret_status;
}

ADXL345_Status_t ADXL345_GetMode(const ADXL345_Platform_t *platform,
                                 ADXL345_MeasurementMode_t *adxl_mode) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if (adxl_mode == NULL) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_ReadRegisterMasked(platform,
                                            ADXL345_REG_POWER_CTL,
                                            ADXL345_MEASURE_MASK,
                                            &register_data);
    if (ret_status == ADXL345_OK) {
        *adxl_mode = (ADXL345_MeasurementMode_t)register_data;
    }

    return ret_status;
}

ADXL345_Status_t ADXL345_SetMode(const ADXL345_Platform_t *platform,
                                 const ADXL345_MeasurementMode_t adxl_mode) {

    ADXL345_Status_t ret_status;

    if ((adxl_mode != ADXL345_STANDBY) && (adxl_mode != ADXL345_MEASURE)) {
        return ADXL345_INVALID_PARAMETER;
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_POWER_CTL,
                                             ADXL345_MEASURE_MASK,
                                             (uint8_t)adxl_mode);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetLinkAutoSleep(const ADXL345_Platform_t *platform,
                                          const bool link,
                                          const bool auto_sleep) {

    ADXL345_Status_t ret_status;
    uint8_t register_data = 0U;

    if (link) {
        register_data = (uint8_t)(register_data | ADXL345_LINK_MASK);
    }

    if (auto_sleep) {
        register_data = (uint8_t)(register_data | ADXL345_AUTO_SLEEP_MASK);
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_POWER_CTL,
                                             (uint8_t)(ADXL345_LINK_MASK |
                                                       ADXL345_AUTO_SLEEP_MASK),
                                             register_data);

    return ret_status;
}

ADXL345_Status_t ADXL345_SetSleep(const ADXL345_Platform_t *platform,
                                  const bool sleep,
                                  const ADXL345_Wakeup_t wakeup) {

    ADXL345_Status_t ret_status;
    uint8_t register_data;

    if ((uint32_t)wakeup > (uint32_t)ADXL345_WAKEUP_1Hz) {
        return ADXL345_INVALID_PARAMETER;
    }

    register_data = (uint8_t)wakeup;

    if (sleep) {
        register_data = (uint8_t)(register_data | ADXL345_SLEEP_MASK);
    }

    ret_status = ADXL345_WriteRegisterMasked(platform,
                                             ADXL345_REG_POWER_CTL,
                                             (uint8_t)(ADXL345_SLEEP_MASK |
                                                       ADXL345_WAKEUP_MASK),
                                             register_data);

    return ret_status;
}
