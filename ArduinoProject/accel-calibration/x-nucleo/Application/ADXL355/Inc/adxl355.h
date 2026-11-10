/**
 * @file adxl355.h
 * @brief Public interface of the ADXL355 accelerometer driver.
 *
 * Register-level driver for the Analog Devices ADXL355 3-axis accelerometer.
 * The driver is platform independent: bus transfers and delays are provided
 * by the application through ADXL355_Platform_t.
 *
 * In the ADXL355's documentation, the terms single acceleration data, sample
 * and entry are used interchangeably to mean a single axis data.
 *
 * In this library
 *      Entry and Sample -> measurement pertaining a single axis
 *      SampleSet -> a three axis measurement (X, Y, Z)
 *
 * @author Carlos Mendes
 * @date Dec/2025
 */

#ifndef ADXL355_H
#define ADXL355_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @name ADXL355 Register Addresses
 * @{
 */
#define ADXL355_REG_DEVID_AD        0x00U
#define ADXL355_REG_DEVID_MST       0x01U
#define ADXL355_REG_PARTID          0x02U
#define ADXL355_REG_REVID           0x03U
#define ADXL355_REG_STATUS          0x04U
#define ADXL355_REG_FIFO_ENTRIES    0x05U
#define ADXL355_REG_TEMP2           0x06U
#define ADXL355_REG_TEMP1           0x07U
#define ADXL355_REG_XDATA3          0x08U
#define ADXL355_REG_XDATA2          0x09U
#define ADXL355_REG_XDATA1          0x0AU
#define ADXL355_REG_YDATA3          0x0BU
#define ADXL355_REG_YDATA2          0x0CU
#define ADXL355_REG_YDATA1          0x0DU
#define ADXL355_REG_ZDATA3          0x0EU
#define ADXL355_REG_ZDATA2          0x0FU
#define ADXL355_REG_ZDATA1          0x10U
#define ADXL355_REG_FIFO_DATA       0x11U
#define ADXL355_REG_OFFSET_X_H      0x1EU
#define ADXL355_REG_OFFSET_X_L      0x1FU
#define ADXL355_REG_OFFSET_Y_H      0x20U
#define ADXL355_REG_OFFSET_Y_L      0x21U
#define ADXL355_REG_OFFSET_Z_H      0x22U
#define ADXL355_REG_OFFSET_Z_L      0x23U
#define ADXL355_REG_ACT_EN          0x24U
#define ADXL355_REG_ACT_THRESH_H    0x25U
#define ADXL355_REG_ACT_THRESH_L    0x26U
#define ADXL355_REG_ACT_COUNT       0x27U
#define ADXL355_REG_FILTER          0x28U
#define ADXL355_REG_FIFO_SAMPLES    0x29U
#define ADXL355_REG_INT_MAP         0x2AU
#define ADXL355_REG_SYNC            0x2BU
#define ADXL355_REG_RANGE           0x2CU
#define ADXL355_REG_POWER_CTL       0x2DU
#define ADXL355_REG_SELF_TEST       0x2EU
#define ADXL355_REG_RESET           0x2FU
#define ADXL355_REG_SHADOW          0x50U
/** @} */

/**
 * @name ADXL355 Device IDs
 * Expected contents of the identification registers.
 * @{
 */
#define ADXL355_DEVID_AD        0xADU
#define ADXL355_DEVID_MST       0x1DU
#define ADXL355_PART_ID         0xEDU
#define ADXL355_REVID           0x01U
// I2C address: 0x1D (MISO/ASEL = 0) 0x53 (MISO/ASEL = 1)
#define ADXL355_I2C_ADDRESS     0x1DU
#define ADXL355_TOTAL_ID_REGISTERS      4U // DEVID_AD, DEVID_MST, PARTID, REVID
/** @} */

/**
 * @name ADXL355 Reset related constants
 * @{
 */
#define ADXL355_TOTAL_SHADOW_REGISTERS      5U
#define ADXL355_RESET_CODE                  0x52U // Written to RESET for a POR
#define ADXL355_NVM_POLL_PERIOD_MS          1U    // NVM_BUSY poll period
#define ADXL355_SHADOW_READ_DELAY_MS        10U   // Reset to shadow read delay
/** @} */

/**
 * @name ADXL355 related MASKs
 * Bit masks of the register fields.
 * @{
 */
#define ADXL355_SPI_READ_BIT            0x01U
#define ADXL355_ID_MASK                 0xFFU
#define ADXL355_NVM_BUSY_MASK           0x10U
#define ADXL355_ACTIVITY_MASK           0x08U
#define ADXL355_FIFO_OVR_MASK           0x04U
#define ADXL355_FIFO_FULL_MASK          0x02U
#define ADXL355_DATA_RDY_MASK           0x01U
#define ADXL355_FIFO_ENTRIES_MASK       0x7FU
#define ADXL355_TEMPERATURE_MASK        0x0FFFU
#define ADXL355_DATA_MASK               0xFFFFF0U
#define ADXL355_OFFSET_MASK             0xFFFFU
#define ADXL355_ACT_EN_MASK             0x07U
#define ADXL355_THRESHOLD_MASK          0xFFFFU
#define ADXL355_ACT_COUNT_MASK          0xFFU
#define ADXL355_HPF_MASK                0x70U
#define ADXL355_ODR_MASK                0x0FU
#define ADXL355_FIFO_SAMPLES_MASK       0x7FU
#define ADXL355_INT_MASK                0xFFU
#define ADXL355_FIFO_EXT_CLK_MASK       0x04U
#define ADXL355_FIFO_EXT_SYNC_MASK      0x03U
#define ADXL355_I2C_HS_MASK             0x80U
#define ADXL355_INT_POL_MASK            0x40U
#define ADXL355_RANGE_MASK              0x03U
#define ADXL355_POWER_CTL_MASK          0x07U
#define ADXL355_SELF_TEST_MASK          0x03U
#define ADXL355_RESET_MASK              0x03U
#define ADXL355_FIFO_X_MARKER_MASK      0x01U // DATA1 of a FIFO entry: X axis
#define ADXL355_FIFO_EMPTY_MASK         0x02U // DATA1 of a FIFO entry: empty
/** @} */

/**
 * @name ADXL355 Data format constants
 * Layout of the 20-bit acceleration data (two's complement, left aligned,
 * MSB first: DATA3, DATA2, DATA1) and weight of the registers aligned with
 * it.
 * @{
 */
#define ADXL355_DATA_BITS               20U // Width of the acceleration data
#define ADXL355_DATA3_BYTE              0U  // Index of DATA3 in a sample
#define ADXL355_DATA2_BYTE              1U  // Index of DATA2 in a sample
#define ADXL355_DATA1_BYTE              2U  // Index of DATA1 in a sample
#define ADXL355_DATA3_SHIFT             12U // DATA3 holds data bits 19:12
#define ADXL355_DATA2_SHIFT             4U  // DATA2 holds data bits 11:4
#define ADXL355_DATA1_SHIFT             4U  // DATA1[7:4] holds data bits 3:0
#define ADXL355_ACT_THRESH_LSB_WEIGHT   8U  // ACT_THRESH matches data 18:3
#define ADXL355_OFFSET_LSB_WEIGHT       16U // OFFSET_x matches data 19:4
/** @} */

/**
 * @name ADXL355 Sample size related constants
 * @{
 */
#define ADXL355_BYTES_PER_SAMPLE        3U  // 1 AXIS == 1 ENTRY
#define ADXL355_BYTES_PER_SAMPLE_SET    9U  // 3 AXIS
#define ADXL355_FIFO_MAX_SAMPLES        96U
#define ADXL355_FIFO_MAX_BYTES             (ADXL355_BYTES_PER_SAMPLE * \
                                         ADXL355_FIFO_MAX_SAMPLES)
#define ADXL355_SAMPLES_PER_SAMPLE_SET     3U
#define ADXL355_FIFO_MAX_SAMPLE_SETS       (ADXL355_FIFO_MAX_SAMPLES / \
                                            ADXL355_SAMPLES_PER_SAMPLE_SET)
/** @} */

/**
 * @name ADXL355 Temperature related constants
 * Nominal transfer function of the temperature sensor (datasheet values).
 *
 * @warning ADXL355_TEMPERATURE_SLOPE and ADXL355_TEMPERATURE_SCALE are float
 *          constants. Prefer the integer constants and
 *          ADXL355_UTIL_ReadTemperatureMdegc() or ADXL355_ReadRawTemperature().
 * @{
 */
#define ADXL355_REFERENCE_TEMPERATURE       25                  // degC
#define ADXL355_REFERENCE_TEMPERATURE_MDEGC 25000               // mdegC
#define ADXL355_REFERENCE_TEMPERATURE_LSB   1885                // LSB
#define ADXL355_TEMPERATURE_SLOPE           (-9.05f)            // LSB/degC
#define ADXL355_TEMPERATURE_SLOPE_LSB_PER_100DEGC (-905)        // LSB/100degC
#define ADXL355_TEMPERATURE_SCALE           (-1.0f / 9.05f)     // degC/LSB
/** @} */

/**
 * @name ADXL355 Sensitivity constants
 * Nominal sensitivity for each measurement range.
 * @{
 */
#define ADXL355_SENSITIVITY_2G      256000  // (LSB/g) -> approx 3.9 ug/LSB)
#define ADXL355_SENSITIVITY_4G      128000  // (LSB/g) -> approx 7.81 ug/LSB)
#define ADXL355_SENSITIVITY_8G       64000  // (LSB/g) -> approx 15.63 ug/LSB)
/** @} */

/**
 * @brief Output data rate (ODR_LPF field of the FILTER register).
 *
 * The low-pass filter corner is ODR / 4.
 */
typedef enum {
    ADXL355_ODR_4000Hz = 0x00,      /**< 4000 Hz */
    ADXL355_ODR_2000Hz = 0x01,      /**< 2000 Hz */
    ADXL355_ODR_1000Hz = 0x02,      /**< 1000 Hz */
    ADXL355_ODR_500Hz = 0x03,       /**< 500 Hz */
    ADXL355_ODR_250Hz = 0x04,       /**< 250 Hz */
    ADXL355_ODR_125Hz = 0x05,       /**< 125 Hz */
    ADXL355_ODR_62_5Hz = 0x06,      /**< 62.5 Hz */
    ADXL355_ODR_31_25Hz = 0x07,     /**< 31.25 Hz */
    ADXL355_ODR_15_625Hz = 0x08,    /**< 15.625 Hz */
    ADXL355_ODR_7_813Hz = 0x09,     /**< 7.813 Hz */
    ADXL355_ODR_3_906Hz = 0x0A      /**< 3.906 Hz */
} ADXL355_ODR_t;

/**
 * @brief High-pass filter corner (HPF_CORNER field of the FILTER register).
 */
typedef enum {
    ADXL355_HPF_OFF = 0x00,     /**< HPF disabled */
    ADXL355_HPF_1 = 0x10,       /**< HPF corner: 24.700E-4 * ODR */
    ADXL355_HPF_2 = 0x20,       /**< HPF corner: 6.2084E-4 * ODR */
    ADXL355_HPF_3 = 0x30,       /**< HPF corner: 1.5545E-4 * ODR */
    ADXL355_HPF_4 = 0x40,       /**< HPF corner: 0.3862E-4 * ODR */
    ADXL355_HPF_5 = 0x50,       /**< HPF corner: 0.0954E-4 * ODR */
    ADXL355_HPF_6 = 0x60        /**< HPF corner: 0.0238E-4 * ODR */
} ADXL355_HPF_t;

/**
 * @brief Measurement range (RANGE field of the RANGE register).
 */
typedef enum {
    ADXL355_RANGE_2G = 0x01,    /**< +/-2 g */
    ADXL355_RANGE_4G = 0x02,    /**< +/-4 g */
    ADXL355_RANGE_8G = 0x03     /**< +/-8 g */
} ADXL355_Range_t;

/**
 * @brief Polarity of the INT1 and INT2 pins (INT_POL bit of RANGE).
 */
typedef enum {
    ADXL355_INT_POL_LOW = 0x00, /**< Interrupts are active low */
    ADXL355_INT_POL_HIGH = 0x40 /**< Interrupts are active high */
} ADXL355_IntPol_t;

/**
 * @brief Interrupt sources and the pin they are mapped to (INT_MAP bits).
 *
 * Values are bit masks and can be combined.
 */
typedef enum {
    ADXL355_INT_NONE = 0x00,        /**< No interrupt */
    ADXL355_DATA_READY_INT1 = 0x01, /**< Data ready on INT1 */
    ADXL355_FIFO_FULL_INT1 = 0x02,  /**< FIFO full (watermark) on INT1 */
    ADXL355_FIFO_OVR_INT1 = 0x04,   /**< FIFO overrun on INT1 */
    ADXL355_ACTIVITY_INT1 = 0x08,   /**< Activity detected on INT1 */
    ADXL355_DATA_READY_INT2 = 0x10, /**< Data ready on INT2 */
    ADXL355_FIFO_FULL_INT2 = 0x20,  /**< FIFO full (watermark) on INT2 */
    ADXL355_FIFO_OVR_INT2 = 0x40,   /**< FIFO overrun on INT2 */
    ADXL355_ACTIVITY_INT2 = 0x80    /**< Activity detected on INT2 */
} ADXL355_IntSource_t;

/**
 * @brief Operating mode (POWER_CTL register).
 *
 * Combination of the STANDBY (bit 0), TEMP_OFF (bit 1) and DRDY_OFF (bit 2)
 * bits.
 */
typedef enum {
    ADXL355_MEAS_TEMP_ON_DRDY_ON = 0,   /**< Measuring, temp. on, DRDY on */
    ADXL355_STDBY_TEMP_ON_DRDY_ON = 1,  /**< Standby, temp. on, DRDY on */
    ADXL355_MEAS_TEMP_OFF_DRDY_ON = 2,  /**< Measuring, temp. off, DRDY on */
    ADXL355_STDBY_TEMP_OFF_DRDY_ON = 3, /**< Standby, temp. off, DRDY on */
    ADXL355_MEAS_TEMP_ON_DRDY_OFF = 4,  /**< Measuring, temp. on, DRDY off */
    ADXL355_STDBY_TEMP_ON_DRDY_OFF = 5, /**< Standby, temp. on, DRDY off */
    ADXL355_MEAS_TEMP_OFF_DRDY_OFF = 6, /**< Measuring, temp. off, DRDY off */
    ADXL355_STDBY_TEMP_OFF_DRDY_OFF = 7 /**< Standby, temp. off, DRDY off */
} ADXL355_MeasurementMode_t;

/**
 * @brief Axes included in activity detection (ACT_EN register).
 */
typedef enum {
    ADXL355_ACTIVITY_OFF = 0,               /**< Activity detection off */
    ADXL355_ACTIVITY_X_ON_Y_OFF_Z_OFF = 1,  /**< X */
    ADXL355_ACTIVITY_X_OFF_Y_ON_Z_OFF = 2,  /**< Y */
    ADXL355_ACTIVITY_X_ON_Y_ON_Z_OFF = 3,   /**< X and Y */
    ADXL355_ACTIVITY_X_OFF_Y_OFF_Z_ON = 4,  /**< Z */
    ADXL355_ACTIVITY_X_ON_Y_OFF_Z_ON = 5,   /**< X and Z */
    ADXL355_ACTIVITY_X_OFF_Y_ON_Z_ON = 6,   /**< Y and Z */
    ADXL355_ACTIVITY_X_ON_Y_ON_Z_ON = 7,    /**< X, Y and Z */
} ADXL355_ActivityMode_t;

/**
 * @brief Accelerometer axis.
 *
 * The value is the address offset of the axis offset register pair from
 * ADXL355_REG_OFFSET_X_H.
 */
typedef enum {
    ADXL355_X_AXIS = 0, /**< X axis */
    ADXL355_Y_AXIS = 2, /**< Y axis */
    ADXL355_Z_AXIS = 4, /**< Z axis */
} ADXL355_Axis_t;

/**
 * @brief Status returned by the driver functions.
 */
typedef enum {
    ADXL355_OK = 0,                /**< Success */
    ADXL355_SPI_ERROR = 1,         /**< Bus transfer failed */
    ADXL355_SPI_RECEIVE_ERROR = 2, /**< Bus receive failed */
    ADXL355_SPI_SEND_ERROR = 3,    /**< Bus send failed */
    ADXL355_DATA_ERROR = 5,        /**< Invalid data */
    ADXL355_INVALID_PARAMETER = 6, /**< Invalid function parameter */
    ADXL355_ERROR = 7,             /**< Unspecified error */
    ADXL355_RESET_ERROR = 8,       /**< Reset failed check */
    ADXL355_INIT_ERROR = 9,        /**< Initialisation failed */
    ADXL355_TIMEOUT = 10,          /**< Wait for the device or bus expired */
    ADXL355_UNEXPECTED_READ_VALUE = 11 /**< Register holds a reserved value */
} ADXL355_Status_t;

/**
 * @brief Platform services used by the driver.
 *
 * The application fills this structure with its bus and delay functions
 * and passes it to every driver function.
 */
typedef struct {
    /** Reads data_size bytes starting at register address. Blocking: it
     *  returns only when buffer holds the data, or with an error; the driver
     *  uses buffer as soon as it returns, often a local variable. It may use
     *  DMA internally if it waits for the end of the transfer.
     *  data_size counts data bytes only: the platform adds the command byte
     *  (SPI: address << 1 plus the read bit) or the I2C addressing, so an
     *  SPI transfer is data_size + 1 bytes and buffer gets the data only. */
    ADXL355_Status_t (*read)(const uint8_t address, uint8_t *buffer,
                             const uint16_t data_size);

    /** Writes data_size bytes starting at register address (blocking).
     *  data_size counts data bytes only: the platform adds the command byte
     *  (SPI: address << 1 with the read bit clear) or the I2C addressing, so
     *  writing one register is a 2-byte SPI transfer. */
    ADXL355_Status_t (*write)(const uint8_t address, const uint8_t *buffer,
                              const uint16_t data_size);

    /** Blocking delay in milliseconds */
    void (*delay_ms)(uint32_t delay_ms);
} ADXL355_Platform_t;

/**
 * @brief Function that starts a read and returns before it ends, e.g. a DMA
 *        transfer (used by ADXL355_ReadFifoDMA()).
 *
 * Same arguments as ADXL355_Platform_t.read: first register address,
 * buffer and number of bytes. It returns ADXL355_OK once the transfer has
 * started; the application detects its end.
 */
typedef ADXL355_Status_t (*ADXL355_StartRead_t)(const uint8_t address,
                                                uint8_t *buffer,
                                                const uint16_t data_size);

/**
 * @brief Raw three-axis sample set, as stored in the XDATA, YDATA and ZDATA
 *        registers (20-bit, two's complement, left aligned, MSB first).
 */
typedef struct {
    uint8_t x_axis[ADXL355_BYTES_PER_SAMPLE]; /**< XDATA3..1 */
    uint8_t y_axis[ADXL355_BYTES_PER_SAMPLE]; /**< YDATA3..1 */
    uint8_t z_axis[ADXL355_BYTES_PER_SAMPLE]; /**< ZDATA3..1 */
} ADXL355_RawSampleSet_t;

/**
 * @brief Reads one register.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address.
 * @param[out] buffer Register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadRegister(const ADXL355_Platform_t *platform,
                                      const uint8_t reg_address, uint8_t *buffer);

/**
 * @brief Writes one register.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address.
 * @param[in] data Value to write.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_WriteRegister(const ADXL355_Platform_t *platform,
                                       const uint8_t reg_address,
                                       const uint8_t data);

/**
 * @brief Reads one register and keeps only the bits selected by mask.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address.
 * @param[in] mask Bits to keep.
 * @param[out] buffer Masked register value, not shifted. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadRegisterMasked(const ADXL355_Platform_t *platform,
                                            const uint8_t reg_address,
                                            const uint8_t mask, uint8_t *buffer);

/**
 * @brief Read-modify-write of the bits of a register selected by mask.
 *
 * The bits in mask are cleared and data is OR-ed into the register; the
 * other bits keep their value.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address.
 * @param[in] mask Bits to change.
 * @param[in] data New value of the field, already in position.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or data has bits outside mask.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_WriteRegisterMasked(const ADXL355_Platform_t *platform,
                                             const uint8_t reg_address,
                                             const uint8_t mask,
                                             const uint8_t data);

// Device ID functions

/**
 * @brief Reads the Analog Devices ID register (expected ADXL355_DEVID_AD).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] dev_id_ad Register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetDevIdAd(const ADXL355_Platform_t *platform,
                                    uint8_t *dev_id_ad);

/**
 * @brief Reads the Analog Devices MEMS ID register (expected
 *        ADXL355_DEVID_MST).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] dev_id_mst Register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetDevIdMst(const ADXL355_Platform_t *platform,
                                     uint8_t *dev_id_mst);

/**
 * @brief Reads the part ID register (expected ADXL355_PART_ID).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] dev_part_id Register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetPartId(const ADXL355_Platform_t *platform,
                                   uint8_t *dev_part_id);

/**
 * @brief Reads the silicon revision ID register.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] dev_rev_id Register value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetRevId(const ADXL355_Platform_t *platform,
                                  uint8_t *dev_rev_id);

/**
 * @brief Reads the four ID registers (DEVID_AD, DEVID_MST, PARTID, REVID)
 *        in one transfer.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] adxl_id Buffer of at least ADXL355_TOTAL_ID_REGISTERS bytes,
 *             filled in register order. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetAllIds(const ADXL355_Platform_t *platform,
                                   uint8_t *adxl_id);

// Temperature functions

/**
 * @brief Reads the raw 12-bit temperature (TEMP2 and TEMP1 registers).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] temperature Temperature in LSB (0 to 4095). Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadRawTemperature(const ADXL355_Platform_t *platform,
                                            uint16_t *temperature);

// Acceleration data functions

/**
 * @brief Reads the current 3-axis measurement.
 *
 * Returns the XDATA, YDATA and ZDATA registers without unpacking or
 * aligning.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] raw_sample_set Raw sample set. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadRawSampleSet(const ADXL355_Platform_t *platform,
                                          ADXL355_RawSampleSet_t *raw_sample_set);

/**
 * @brief Reads the offset of one axis.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axis Axis to read.
 * @param[out] axis_offset Offset, high byte first. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or axis is not a valid ADXL355_Axis_t.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetAxisOffset(const ADXL355_Platform_t *platform,
                                       const ADXL355_Axis_t axis,
                                       uint8_t axis_offset[2]);

/**
 * @brief Writes the offset of one axis (OFFSET_x_H and OFFSET_x_L).
 *
 * The device subtracts the offset from the measurement (measured on the
 * device). Its LSB has the weight of bit 4 of the 20-bit acceleration data.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axis Axis to write.
 * @param[in] offset Offset, two's complement.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or axis is not a valid ADXL355_Axis_t.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_SetAxisOffset(const ADXL355_Platform_t *platform,
                                       const ADXL355_Axis_t axis,
                                       const int16_t offset);

// FIFO Functions

/**
 * @brief Reads the FIFO watermark (FIFO_SAMPLES register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] fifo_samples Number of entries that sets FIFO_FULL.
 *             Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetFifoSamples(const ADXL355_Platform_t *platform,
                                        uint8_t *fifo_samples);

/**
 * @brief Sets the FIFO watermark (FIFO_SAMPLES register).
 *
 * The register counts entries (single-axis samples), not sample sets.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] fifo_max_entries Number of entries that sets FIFO_FULL
 *            (1 to ADXL355_FIFO_MAX_SAMPLES).
 *
 * @retval ADXL355_OK Watermark written.
 * @retval ADXL355_INVALID_PARAMETER fifo_max_entries is out of range, or a
 *         required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetFifoSamples(const ADXL355_Platform_t *platform,
                                        const uint8_t fifo_max_entries);

/**
 * @brief Reads the number of entries currently stored in the FIFO.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] num_fifo_entries Number of entries (0 to
 *             ADXL355_FIFO_MAX_SAMPLES). Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadNumberFifoEntries(const ADXL355_Platform_t *platform,
                                               uint8_t *num_fifo_entries);

/**
 * @brief Raw read of entries from the FIFO.
 *
 * Reads (3 x num_entries) bytes from the FIFO base address without checking
 * the current valid entries/samples nor the FIFO entries alignment.
 * To avoid losing alignment (i.e, to make sure the first entry is the X
 * axis), it is advisable to read the FIFO in multiples of 3 entries/samples
 * (9 bytes) to form a valid sample set.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] fifo_data Buffer of at least (3 x num_entries) bytes.
 *             Must not be NULL.
 * @param[in] num_entries Entries to read (1 to ADXL355_FIFO_MAX_SAMPLES).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or num_entries is out of range.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadFifo(const ADXL355_Platform_t *platform,
                                  uint8_t *fifo_data, const uint8_t num_entries);

/**
 * @brief Reads sample sets from the FIFO.
 *
 * Reads the FIFO in multiples of sample sets, where a sample set is a
 * sequence of X, Y and Z measurements.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] sample_sets Array of at least num_sample_sets elements.
 *             Must not be NULL.
 * @param[in] num_sample_sets Sample sets to read
 *            (1 to ADXL355_FIFO_MAX_SAMPLE_SETS).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or num_sample_sets is out of range.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_ReadFifoSampleSet(const ADXL355_Platform_t *platform,
                                           ADXL355_RawSampleSet_t *sample_sets,
                                           const uint8_t num_sample_sets);

/**
 * @brief Starts a read of entries from the FIFO (e.g. with DMA) and returns
 *        without waiting for the data.
 *
 * Calls start_read for (3 x num_entries) bytes from the FIFO base address,
 * like ADXL355_ReadFifo(), and returns its status. The driver keeps no
 * state: the application detects the end of the transfer and then uses
 * fifo_data. With num_entries a multiple of 3 (and an aligned FIFO),
 * fifo_data holds whole sample sets for ADXL355_UTIL_UnpackSampleSets().
 *
 * @warning fifo_data must stay valid until the transfer ends: use a static
 *          buffer, never a local variable of a function that returns before
 *          the end.
 *
 * @param[in] start_read Function that starts the read. Must not be NULL.
 * @param[out] fifo_data Buffer of at least (3 x num_entries) bytes.
 *             Must not be NULL.
 * @param[in] num_entries Entries to read (1 to ADXL355_FIFO_MAX_SAMPLES).
 *
 * @retval ADXL355_OK The read started.
 * @retval ADXL355_INVALID_PARAMETER start_read or fifo_data is NULL, or
 *         num_entries is out of range (start_read is not called).
 * @return Otherwise, the status returned by start_read.
 *
 * @note The interrupt that tells the FIFO is full (FIFO_FULL on INT1 or
 *       INT2) is configured with ADXL355_EnableInterrupt() as usual.
 */
ADXL355_Status_t ADXL355_ReadFifoDMA(ADXL355_StartRead_t start_read,
                                     uint8_t *fifo_data,
                                     const uint8_t num_entries);

// Activity functions

/**
 * @brief Reads the activity detection threshold (ACT_THRESH_H/L).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] act_threshold Threshold. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetActivityThreshold(const ADXL355_Platform_t *platform,
                                              uint16_t *act_threshold);

/**
 * @brief Writes the activity detection threshold (ACT_THRESH_H/L).
 *
 * The threshold is compared with the magnitude of the acceleration
 * (bits 18:3 of the 20-bit data) of each enabled axis.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] act_threshold Threshold.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_SetActivityThreshold(const ADXL355_Platform_t *platform,
                                              const uint16_t act_threshold);

/**
 * @brief Reads the activity count (ACT_COUNT register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] act_count Consecutive samples above the threshold needed to
 *             detect activity. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetActivityCount(const ADXL355_Platform_t *platform,
                                          uint8_t *act_count);

/**
 * @brief Writes the activity count (ACT_COUNT register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] act_count Consecutive samples above the threshold needed to
 *            detect activity.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetActivityCount(const ADXL355_Platform_t *platform,
                                          const uint8_t act_count);

/**
 * @brief Selects the axes used for activity detection (ACT_EN register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] act_mode Axes to enable.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or act_mode is not a valid
 *         ADXL355_ActivityMode_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_EnableActivity(const ADXL355_Platform_t *platform,
                                        const ADXL355_ActivityMode_t act_mode);

/**
 * @brief Disables activity detection on all axes.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_DisableActivity(const ADXL355_Platform_t *platform);

// Interrupts

/**
 * @brief Sets the polarity of the INT1 and INT2 pins.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_pol Polarity.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or int_pol is not a valid
 *         ADXL355_IntPol_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetInterruptPolarity(const ADXL355_Platform_t *platform,
                                              const ADXL355_IntPol_t int_pol);

/**
 * @brief Reads the polarity of the INT1 and INT2 pins.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_pol Polarity. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetInterruptPolarity(const ADXL355_Platform_t *platform,
                                              ADXL355_IntPol_t *int_pol);

/**
 * @brief Configures, with a single write, all the available interrupts.
 *
 * Following ADXL355 INT_MAP register, each bit of int_map is connected to a
 * possible interrupt signal as follows
 *
 *  ACT_EN2 OVR_EN2 FULL_EN2 RDY_EN2 ACT_EN1 OVR_EN1 FULL_EN1 RDY_EN1
 *
 *  EN2 acts on INT2 and EN1 acts on INT1
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_map New INT_MAP value (OR of ADXL355_IntSource_t values).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_SetInterruptMap(const ADXL355_Platform_t *platform,
                                         const uint8_t int_map);

/**
 * @brief Reads the interrupt map (INT_MAP register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_map INT_MAP value. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetInterruptMap(const ADXL355_Platform_t *platform,
                                         uint8_t *int_map);

/**
 * @brief Disables, with a single write, all the available interrupts.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_ClearInterrupts(const ADXL355_Platform_t *platform);

/**
 * @brief Enables the given interrupt source(s); the others are kept.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_src Interrupt source(s) to enable.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_EnableInterrupt(const ADXL355_Platform_t *platform,
                                         const ADXL355_IntSource_t int_src);

/**
 * @brief Disables the given interrupt source(s); the others are kept.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_src Interrupt source(s) to disable.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_DisableInterrupt(const ADXL355_Platform_t *platform,
                                          const ADXL355_IntSource_t int_src);

// ADXL355 operation (ODRs, FILTER, RANGE)

/**
 * @brief Reads the measurement range.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] range Measurement range. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @retval ADXL355_UNEXPECTED_READ_VALUE The register holds a reserved
 *         value; range is not written.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetRange(const ADXL355_Platform_t *platform,
                                  ADXL355_Range_t *range);

/**
 * @brief Sets the measurement range.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] range Measurement range.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or range is not a valid ADXL355_Range_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetRange(const ADXL355_Platform_t *platform,
                                  const ADXL355_Range_t range);

/**
 * @brief Reads the output data rate.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] odr Output data rate. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @retval ADXL355_UNEXPECTED_READ_VALUE The register holds a reserved
 *         value; odr is not written.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetODR(const ADXL355_Platform_t *platform,
                                ADXL355_ODR_t *odr);

/**
 * @brief Sets the output data rate.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] odr Output data rate.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or odr is not a valid ADXL355_ODR_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetODR(const ADXL355_Platform_t *platform,
                                const ADXL355_ODR_t odr);

/**
 * @brief Reads the high-pass filter corner.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] hpf High-pass filter corner. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @retval ADXL355_UNEXPECTED_READ_VALUE The register holds a reserved
 *         value; hpf is not written.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetHPF(const ADXL355_Platform_t *platform,
                                ADXL355_HPF_t *hpf);

/**
 * @brief Sets the high-pass filter corner.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] hpf High-pass filter corner.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or hpf is not a valid ADXL355_HPF_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetHPF(const ADXL355_Platform_t *platform,
                                const ADXL355_HPF_t hpf);

/**
 * @brief Reads the operating mode (POWER_CTL register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] adxl_mode Operating mode. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL355_Status_t ADXL355_GetMode(const ADXL355_Platform_t *platform,
                                 ADXL355_MeasurementMode_t *adxl_mode);

/**
 * @brief Sets the operating mode (POWER_CTL register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] adxl_mode Operating mode.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or adxl_mode is not a valid
 *         ADXL355_MeasurementMode_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL355_Status_t ADXL355_SetMode(const ADXL355_Platform_t *platform,
                                 const ADXL355_MeasurementMode_t adxl_mode);

/**
 * @brief Writes the reset code to the RESET register (soft reset).
 *
 * Only issues the command: it does not wait for NVM_BUSY to clear nor check
 * the shadow registers.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL355_Status_t ADXL355_SoftReset(const ADXL355_Platform_t *platform);

#endif // ADXL355_H
