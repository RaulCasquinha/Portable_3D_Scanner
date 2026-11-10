/**
 * @file app_accel.c
 * @brief Source file for the app_accel.h header file.
 *
 * Layer: application service. The only module that knows which sensor is
 * fitted (APP_SENSOR) and talks to its driver.
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 */

#include "app_accel.h"

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "app_calib_conf.h"

#if (APP_SENSOR == APP_SENSOR_ADXL345)
#include "adxl345.h"
#include "adxl345_stm32wl.h"
#include "adxl345_util.h"
#elif (APP_SENSOR == APP_SENSOR_ADXL355)
#include "adxl355.h"
#include "adxl355_stm32wl.h"
#include "adxl355_util.h"
#else
#error "APP_SENSOR must be APP_SENSOR_ADXL345 or APP_SENSOR_ADXL355"
#endif

#define X                       0U
#define Y                       1U
#define Z                       2U
#define NEW_SAMPLE_TIMEOUT_MS   100U

extern SPI_HandleTypeDef hspi1;

static bool SetSpiMode(uint32_t polarity, uint32_t phase);
static bool WaitForNewSample(void);
static bool IsNewSample(bool *is_new);

#if (APP_SENSOR == APP_SENSOR_ADXL345)

#define FULL_RESOLUTION         true

static const ADXL345_Platform_t *APP_ACCEL_Platform = NULL;

bool APP_ACCEL_Init(void) {
    APP_ACCEL_Platform = ADXL345_STM32WL_GetSpiPlatform();

    ADXL345_UTIL_DevCfg_t config = {
        .platform = APP_ACCEL_Platform,
        .odr = ADXL345_ODR_100Hz,
        .range = ADXL345_RANGE_16G,
        .full_res = FULL_RESOLUTION,
        .act_coupling = ADXL345_DC_COUPLED,
        .inact_coupling = ADXL345_DC_COUPLED,
        .int_pol = ADXL345_INT_POL_HIGH,
        .fifo_mode = ADXL345_FIFO_BYPASS,
        .fifo_trigger = ADXL345_INT1,
        .meas_mode = ADXL345_MEASURE
    };

    return (ADXL345_STM32WL_ConfigSpi(&hspi1, ADXL_NCS_GPIO_Port,
                                      ADXL_NCS_Pin) == ADXL345_STM32WL_OK)
           && SetSpiMode(SPI_POLARITY_HIGH, SPI_PHASE_2EDGE)
           && (ADXL345_CheckDevId(APP_ACCEL_Platform) == ADXL345_OK)
           && (ADXL345_UTIL_Config(&config) == ADXL345_OK);
}

bool APP_ACCEL_ReadG(float sample_g[]) {
    ADXL345_RawSampleSet_t raw = {{0U}, {0U}, {0U}};
    ADXL345_UTIL_UnpackedSampleSet_t lsb = {0, 0, 0};
    ADXL345_UTIL_SampleSetGs_t g = {0.0F, 0.0F, 0.0F};

    bool is_read = WaitForNewSample()
                   && (ADXL345_ReadRawSampleSet(APP_ACCEL_Platform, &raw)
                       == ADXL345_OK)
                   && (ADXL345_UTIL_UnpackSampleSets(&raw, 1U, &lsb)
                       == ADXL345_OK)
                   && (ADXL345_UTIL_SampleSets2Gs(&lsb, ADXL345_RANGE_16G,
                                                  FULL_RESOLUTION, 1U, &g)
                       == ADXL345_OK);

    sample_g[X] = g.x_axis;
    sample_g[Y] = g.y_axis;
    sample_g[Z] = g.z_axis;
    return is_read;
}

bool APP_ACCEL_ReadTemperature(int32_t *temperature_mdegc) {
    (void)temperature_mdegc;
    return false;
}

/** Reads the data-ready flag of the ADXL345 */
static bool IsNewSample(bool *is_new) {
    uint8_t int_source = 0U;

    bool is_read = (ADXL345_GetInterruptSource(APP_ACCEL_Platform, &int_source)
                    == ADXL345_OK);
    *is_new = ((int_source & ADXL345_INT_DATA_READY) != 0U);
    return is_read;
}

#else

#define ADXL355_RANGE           ADXL355_RANGE_2G

static const ADXL355_Platform_t *APP_ACCEL_Platform = NULL;

bool APP_ACCEL_Init(void) {
    uint8_t part_id = 0U;

    APP_ACCEL_Platform = ADXL355_STM32WL_GetSpiPlatform();

    ADXL355_UTIL_DevCfg_t config = {
        .platform = APP_ACCEL_Platform,
        .odr = ADXL355_ODR_125Hz,
        .hpf = ADXL355_HPF_OFF,
        .range = ADXL355_RANGE,
        .int_pol = ADXL355_INT_POL_HIGH,
        .int_src = ADXL355_INT_NONE,
        .act_mode = ADXL355_ACTIVITY_OFF,
        .act_cnt = 1U,
        .act_thr_lsb = 0U,
        .fifo_samples = ADXL355_FIFO_MAX_SAMPLES,
        .x_offset_lsb = 0,
        .y_offset_lsb = 0,
        .z_offset_lsb = 0,
        .meas_mode = ADXL355_MEAS_TEMP_ON_DRDY_ON
    };

    return (ADXL355_STM32WL_ConfigSpi(&hspi1, ADXL_NCS_GPIO_Port,
                                      ADXL_NCS_Pin) == ADXL355_STM32WL_OK)
           && SetSpiMode(SPI_POLARITY_LOW, SPI_PHASE_1EDGE)
           && (ADXL355_GetPartId(APP_ACCEL_Platform, &part_id) == ADXL355_OK)
           && (part_id == ADXL355_PART_ID)
           && (ADXL355_UTIL_Config(&config) == ADXL355_OK);
}

bool APP_ACCEL_ReadG(float sample_g[]) {
    ADXL355_RawSampleSet_t raw;
    ADXL355_UTIL_UnpackedSampleSet_t lsb = {0, 0, 0};
    ADXL355_UTIL_SampleSetGs_t g = {0.0F, 0.0F, 0.0F};

    bool is_read = WaitForNewSample()
                   && (ADXL355_ReadRawSampleSet(APP_ACCEL_Platform, &raw)
                       == ADXL355_OK)
                   && (ADXL355_UTIL_UnpackSampleSets(&raw, 1U, &lsb)
                       == ADXL355_OK)
                   && (ADXL355_UTIL_SampleSets2Gs(&lsb, ADXL355_RANGE, 1U, &g)
                       == ADXL355_OK);

    sample_g[X] = g.x_axis;
    sample_g[Y] = g.y_axis;
    sample_g[Z] = g.z_axis;
    return is_read;
}

bool APP_ACCEL_ReadTemperature(int32_t *temperature_mdegc) {
    return (ADXL355_UTIL_ReadTemperatureMdegc(APP_ACCEL_Platform,
                                              temperature_mdegc)
            == ADXL355_OK);
}

/** Reads the data-ready flag of the ADXL355 */
static bool IsNewSample(bool *is_new) {
    uint8_t status = 0U;

    bool is_read = (ADXL355_ReadRegister(APP_ACCEL_Platform, ADXL355_REG_STATUS,
                                         &status) == ADXL355_OK);
    *is_new = ((status & ADXL355_DATA_RDY_MASK) != 0U);
    return is_read;
}

#endif // APP_SENSOR

/** Sets the SPI mode of SPI1: the two sensors use different modes */
static bool SetSpiMode(uint32_t polarity, uint32_t phase) {
    hspi1.Init.CLKPolarity = polarity;
    hspi1.Init.CLKPhase = phase;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    return (HAL_SPI_Init(&hspi1) == HAL_OK);
}

/** Waits for the data-ready flag, at most NEW_SAMPLE_TIMEOUT_MS */
static bool WaitForNewSample(void) {
    uint32_t start_ms = HAL_GetTick();
    bool is_new = false;

    while (!is_new && ((HAL_GetTick() - start_ms) <= NEW_SAMPLE_TIMEOUT_MS)) {
        if (!IsNewSample(&is_new)) {
            return false;
        }
    }
    return is_new;
}
