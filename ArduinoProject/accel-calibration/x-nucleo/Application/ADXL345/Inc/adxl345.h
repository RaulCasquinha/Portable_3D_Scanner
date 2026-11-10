/**
 * @file adxl345.h
 * @brief Public interface of the ADXL345 accelerometer driver.
 *
 * Register-level driver for the Analog Devices ADXL345 3-axis accelerometer.
 * The driver is platform independent: bus transfers and delays are provided
 * by the application through ADXL345_Platform_t.
 *
 * In the ADXL345's documentation, the terms sample and data are used for a
 * single axis measurement as well as for the three axes.
 *
 * In this library
 *      Sample -> measurement pertaining a single axis
 *      SampleSet -> a three axis measurement (X, Y, Z)
 *
 * References: every value taken from the datasheet carries a "Ref:" line with
 * its page and table/section in
 *      DS = ADXL345 datasheet, Rev. G (Analog Devices, 5/2022),
 *           "6. ADXL345 Driver Research/adxl345.pdf"
 *
 * Values marked "derived" are computed from the cited table (e.g. a mask
 * covering several bits).
 *
 * @author Raul Casquinha
 * @date Oct/2026
 */

#ifndef ADXL345_H
#define ADXL345_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @name ADXL345 Register Addresses
 * @note Addresses 0x01 to 0x1C are reserved and must not be accessed.
 * @{
 */

/** Device ID (R, reset 0xE5).
 *  @n Ref: DS p.23 Table 19; p.24 Table 20 */
#define ADXL345_REG_DEVID           0x00U

/** Tap threshold (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x1D" */
#define ADXL345_REG_THRESH_TAP      0x1DU

/** X-axis offset (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x1E..0x20" */
#define ADXL345_REG_OFSX            0x1EU

/** Y-axis offset (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x1E..0x20" */
#define ADXL345_REG_OFSY            0x1FU

/** Z-axis offset (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x1E..0x20" */
#define ADXL345_REG_OFSZ            0x20U

/** Tap duration (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x21" */
#define ADXL345_REG_DUR             0x21U

/** Tap latency (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x22" */
#define ADXL345_REG_LATENT          0x22U

/** Tap window (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x23" */
#define ADXL345_REG_WINDOW          0x23U

/** Activity threshold (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x24" */
#define ADXL345_REG_THRESH_ACT      0x24U

/** Inactivity threshold (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x25" */
#define ADXL345_REG_THRESH_INACT    0x25U

/** Inactivity time (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.24 "Register 0x26" */
#define ADXL345_REG_TIME_INACT      0x26U

/** Axis enable control for activity and inactivity detection (R/W, reset
 *  0x00).
 *  @n Ref: DS p.23 Table 19; p.24 Tables 21-22 */
#define ADXL345_REG_ACT_INACT_CTL   0x27U

/** Free-fall threshold (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.25 "Register 0x28" */
#define ADXL345_REG_THRESH_FF       0x28U

/** Free-fall time (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.25 "Register 0x29" */
#define ADXL345_REG_TIME_FF         0x29U

/** Axis control for single tap/double tap (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.25 Table 23 */
#define ADXL345_REG_TAP_AXES        0x2AU

/** Source of single tap/double tap (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.25 Table 24 */
#define ADXL345_REG_ACT_TAP_STATUS  0x2BU

/** Data rate and power mode control (R/W, reset 0x0A).
 *  @n Ref: DS p.23 Table 19; p.25 Table 25 */
#define ADXL345_REG_BW_RATE         0x2CU

/** Power-saving features control (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.26 Table 26 */
#define ADXL345_REG_POWER_CTL       0x2DU

/** Interrupt enable control (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.26 Tables 28-29 */
#define ADXL345_REG_INT_ENABLE      0x2EU

/** Interrupt mapping control (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.26-27 Tables 30-31 */
#define ADXL345_REG_INT_MAP         0x2FU

/** Source of interrupts (R, reset 0x02).
 *  @n Ref: DS p.23 Table 19; p.27 Tables 32-33 */
#define ADXL345_REG_INT_SOURCE      0x30U

/** Data format control (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 Table 34 */
#define ADXL345_REG_DATA_FORMAT     0x31U

/** X-axis data 0, LSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAX0          0x32U

/** X-axis data 1, MSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAX1          0x33U

/** Y-axis data 0, LSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAY0          0x34U

/** Y-axis data 1, MSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAY1          0x35U

/** Z-axis data 0, LSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAZ0          0x36U

/** Z-axis data 1, MSB (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.27 "Register 0x32 to 0x37" */
#define ADXL345_REG_DATAZ1          0x37U

/** FIFO control (R/W, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.28 Table 36 */
#define ADXL345_REG_FIFO_CTL        0x38U

/** FIFO status (R, reset 0x00).
 *  @n Ref: DS p.23 Table 19; p.28 Table 39 */
#define ADXL345_REG_FIFO_STATUS     0x39U

/** @} */

/**
 * @name ADXL345 Device IDs
 * Expected contents of the identification register.
 * @{
 */

/** Fixed DEVID value (345 octal).
 *  @n Ref: DS p.24 Table 20 */
#define ADXL345_DEVID                0xE5U

// I2C address: 0x53 (SDO/ALT ADDRESS = 0) 0x1D (SDO/ALT ADDRESS = 1)

/** 7-bit I2C address with SDO/ALT ADDRESS tied to GND.
 *  @n Ref: DS p.17 "I2C", Figure 40 */
#define ADXL345_I2C_ADDRESS_ALT_LOW  0x53U

/** 7-bit I2C address with SDO/ALT ADDRESS tied to VDD I/O.
 *  @n Ref: DS p.17 "I2C" */
#define ADXL345_I2C_ADDRESS_ALT_HIGH 0x1DU

/** @} */

/**
 * @name ADXL345 related MASKs
 * Bit masks of the register fields.
 * @{
 */

/** SPI command byte [R/W][MB][A5..A0]: 1 = read, 0 = write. The SPI mode
 *  is CPOL = 1, CPHA = 1, max 5 MHz.
 *  @n Ref: DS p.14 "SPI"; p.15 Figures 37-38; p.16 Table 10 */
#define ADXL345_SPI_READ_BIT        0x80U

/** SPI command byte: multiple-byte (auto increment) transfer.
 *  @n Ref: DS p.14 "SPI"; p.15 Figures 37-38 */
#define ADXL345_SPI_MB_BIT          0x40U

/** SPI command byte: register address bits A5..A0.
 *  @n Ref: DS p.15 Figures 37-38 */
#define ADXL345_SPI_ADDR_MASK       0x3FU

/** ACT_INACT_CTL: activity ac-coupled (1) / dc-coupled (0).
 *  @n Ref: DS p.24 Table 21 (D7), "ACT AC/DC and INACT AC/DC Bits" */
#define ADXL345_ACT_AC_MASK         0x80U

/** ACT_INACT_CTL: ACT_X, ACT_Y, ACT_Z enable.
 *  @n Ref: DS p.24 Table 21 (D6:D4) */
#define ADXL345_ACT_AXES_MASK       0x70U

/** ACT_INACT_CTL: whole activity nibble.
 *  @n Ref: DS p.24 Table 21 (D7:D4), derived */
#define ADXL345_ACT_MASK            0xF0U

/** ACT_INACT_CTL: inactivity ac-coupled (1) / dc-coupled (0).
 *  @n Ref: DS p.24 Table 22 (D3) */
#define ADXL345_INACT_AC_MASK       0x08U

/** ACT_INACT_CTL: INACT_X, INACT_Y, INACT_Z enable.
 *  @n Ref: DS p.24 Table 22 (D2:D0) */
#define ADXL345_INACT_AXES_MASK     0x07U

/** ACT_INACT_CTL: whole inactivity nibble.
 *  @n Ref: DS p.24 Table 22 (D3:D0), derived */
#define ADXL345_INACT_MASK          0x0FU

/** X, Y and Z enable bits of the axes flags, before shifting.
 *  @n Ref: DS p.24 Tables 21-22; p.25 Table 23, derived */
#define ADXL345_AXES_MASK           0x07U

/** TAP_AXES: suppress double tap if acceleration > THRESH_TAP between taps.
 *  @n Ref: DS p.25 Table 23 (D3), "Suppress Bit" */
#define ADXL345_TAP_SUPPRESS_MASK   0x08U

/** TAP_AXES: TAP_X, TAP_Y, TAP_Z enable.
 *  @n Ref: DS p.25 Table 23 (D2:D0) */
#define ADXL345_TAP_AXES_MASK       0x07U

/** TAP_AXES: all writable bits (D7:D4 read as 0).
 *  @n Ref: DS p.25 Table 23 (D3:D0), derived */
#define ADXL345_TAP_AXES_REG_MASK   0x0FU

/** ACT_TAP_STATUS: X involved in the activity event.
 *  @n Ref: DS p.25 Table 24 (D6) */
#define ADXL345_ACT_X_SOURCE        0x40U

/** ACT_TAP_STATUS: Y involved in the activity event.
 *  @n Ref: DS p.25 Table 24 (D5) */
#define ADXL345_ACT_Y_SOURCE        0x20U

/** ACT_TAP_STATUS: Z involved in the activity event.
 *  @n Ref: DS p.25 Table 24 (D4) */
#define ADXL345_ACT_Z_SOURCE        0x10U

/** ACT_TAP_STATUS: device is asleep (auto sleep only).
 *  @n Ref: DS p.25 Table 24 (D3), "Asleep Bit" */
#define ADXL345_ASLEEP_MASK         0x08U

/** ACT_TAP_STATUS: X involved in the tap event.
 *  @n Ref: DS p.25 Table 24 (D2) */
#define ADXL345_TAP_X_SOURCE        0x04U

/** ACT_TAP_STATUS: Y involved in the tap event.
 *  @n Ref: DS p.25 Table 24 (D1) */
#define ADXL345_TAP_Y_SOURCE        0x02U

/** ACT_TAP_STATUS: Z involved in the tap event.
 *  @n Ref: DS p.25 Table 24 (D0) */
#define ADXL345_TAP_Z_SOURCE        0x01U

/** BW_RATE: reduced power operation.
 *  @n Ref: DS p.25 Table 25 (D4), "LOW_POWER Bit"; p.13 "Power Modes" */
#define ADXL345_LOW_POWER_MASK      0x10U

/** BW_RATE: output data rate code.
 *  @n Ref: DS p.25 Table 25 (D3:D0), "Rate Bits" */
#define ADXL345_RATE_MASK           0x0FU

/** POWER_CTL: link activity and inactivity.
 *  @n Ref: DS p.26 Table 26 (D5), "Link Bit" */
#define ADXL345_LINK_MASK           0x20U

/** POWER_CTL: auto sleep on inactivity (needs link).
 *  @n Ref: DS p.26 Table 26 (D4), "AUTO_SLEEP Bit" */
#define ADXL345_AUTO_SLEEP_MASK     0x10U

/** POWER_CTL: measurement (1) / standby (0).
 *  @n Ref: DS p.26 Table 26 (D3), "Measure Bit" */
#define ADXL345_MEASURE_MASK        0x08U

/** POWER_CTL: sleep mode.
 *  @n Ref: DS p.26 Table 26 (D2), "Sleep Bit" */
#define ADXL345_SLEEP_MASK          0x04U

/** POWER_CTL: frequency of readings in sleep mode.
 *  @n Ref: DS p.26 Table 26 (D1:D0); Table 27 */
#define ADXL345_WAKEUP_MASK         0x03U

/** INT_ENABLE / INT_MAP / INT_SOURCE: all bits, see ADXL345_INT_x flags.
 *  @n Ref: DS p.26 Tables 28-30; p.27 Tables 31-33 */
#define ADXL345_INT_MASK            0xFFU

/** DATA_FORMAT: self-test force.
 *  @n Ref: DS p.27 Table 34 (D7), "SELF_TEST Bit" */
#define ADXL345_SELF_TEST_MASK      0x80U

/** DATA_FORMAT: 3-wire (1) / 4-wire (0) SPI.
 *  @n Ref: DS p.27 Table 34 (D6), "SPI Bit" */
#define ADXL345_SPI_3WIRE_MASK      0x40U

/** DATA_FORMAT: interrupts active low (1) / high (0).
 *  @n Ref: DS p.27 Table 34 (D5), "INT_INVERT Bit" */
#define ADXL345_INT_INVERT_MASK     0x20U

/** DATA_FORMAT: full resolution (4 mg/LSB at any range).
 *  @n Ref: DS p.27 Table 34 (D3), "FULL_RES Bit" */
#define ADXL345_FULL_RES_MASK       0x08U

/** DATA_FORMAT: left (1) / right (0) justified data.
 *  @n Ref: DS p.27 Table 34 (D2), "Justify Bit" */
#define ADXL345_JUSTIFY_MASK        0x04U

/** DATA_FORMAT: g range.
 *  @n Ref: DS p.27 Table 34 (D1:D0); Table 35 */
#define ADXL345_RANGE_MASK          0x03U

/** FIFO_CTL: FIFO mode.
 *  @n Ref: DS p.28 Table 36 (D7:D6); Table 37 */
#define ADXL345_FIFO_MODE_MASK      0xC0U

/** FIFO_CTL: trigger event on INT2 (1) / INT1 (0).
 *  @n Ref: DS p.28 Table 36 (D5), "Trigger Bit" */
#define ADXL345_FIFO_TRIGGER_MASK   0x20U

/** FIFO_CTL: watermark / trigger samples.
 *  @n Ref: DS p.28 Table 36 (D4:D0); Table 38 */
#define ADXL345_FIFO_SAMPLES_MASK   0x1FU

/** FIFO_STATUS: trigger event occurred.
 *  @n Ref: DS p.28 Table 39 (D7), "FIFO_TRIG Bit" */
#define ADXL345_FIFO_TRIG_MASK      0x80U

/** FIFO_STATUS: number of stored sample sets.
 *  @n Ref: DS p.28 Table 39 (D5:D0), "Entries Bits" */
#define ADXL345_FIFO_ENTRIES_MASK   0x3FU

/** @} */

/**
 * @name ADXL345 Data format constants
 * Position of the activity and inactivity fields and weight of the
 * threshold, offset and time registers (integers, no floating point).
 * @{
 */

/** ACT_INACT_CTL: position of the activity axes.
 *  @n Ref: DS p.24 Table 21 (ACT_Z enable is D4), derived */
#define ADXL345_ACT_SHIFT               4U

/** ACT_INACT_CTL: position of the inactivity axes.
 *  @n Ref: DS p.24 Table 22 (INACT_Z enable is D0), derived */
#define ADXL345_INACT_SHIFT             0U

/** THRESH_TAP, THRESH_ACT, THRESH_INACT, THRESH_FF scale (ug/LSB, 62.5 mg).
 *  @n Ref: DS p.24 "Register 0x1D", "0x24", "0x25"; p.25 "Register 0x28" */
#define ADXL345_THRESH_SCALE_UG         62500U

/** OFSX, OFSY, OFSZ scale (ug/LSB, 15.6 mg, twos complement).
 *  @n Ref: DS p.24 "Register 0x1E..0x20"; p.30 "Offset Calibration" */
#define ADXL345_OFFSET_SCALE_UG         15600U

/** DUR scale (us/LSB).
 *  @n Ref: DS p.24 "Register 0x21" */
#define ADXL345_DUR_SCALE_US            625U

/** LATENT and WINDOW scale (us/LSB).
 *  @n Ref: DS p.24 "Register 0x22", "Register 0x23" */
#define ADXL345_LATENT_SCALE_US         1250U

/** TIME_FF scale (ms/LSB).
 *  @n Ref: DS p.25 "Register 0x29" */
#define ADXL345_TIME_FF_SCALE_MS        5U

/** TIME_INACT scale (s/LSB).
 *  @n Ref: DS p.24 "Register 0x26" */
#define ADXL345_TIME_INACT_SCALE_S      1U

/** @} */

/**
 * @name ADXL345 Sample size related constants
 * @{
 */

/** 1 axis, LSB first (DATAx0 = LSB, DATAx1 = MSB).
 *  @n Ref: DS p.27 "Register 0x32 to 0x37" */
#define ADXL345_BYTES_PER_SAMPLE        2U

/** 3 axes, registers 0x32 to 0x37.
 *  @n Ref: DS p.27 "Register 0x32 to 0x37" */
#define ADXL345_BYTES_PER_SAMPLE_SET    6U

/** X, Y and Z.
 *  @n Ref: DS p.27 "Register 0x32 to 0x37" */
#define ADXL345_SAMPLES_PER_SAMPLE_SET  3U

/** FIFO depth.
 *  @n Ref: DS p.21 "FIFO"; p.28 "Entries Bits" */
#define ADXL345_FIFO_MAX_SAMPLE_SETS    32U

/** FIFO depth + the set in the output registers.
 *  @n Ref: DS p.28 "Entries Bits" */
#define ADXL345_FIFO_MAX_READ_SETS      33U

/** @} */

/**
 * @name ADXL345 Sensitivity constants
 * Nominal sensitivity for each measurement range (typical, VS = 2.5 V).
 * @{
 */

/** LSB/g in full resolution mode, any range (~3.9 mg/LSB).
 *  @n Ref: DS p.3 Table 1, "Sensitivity at XOUT, YOUT, ZOUT" */
#define ADXL345_SENSITIVITY_FULL_RES    256U

/** LSB/g, 10-bit, +-2 g.
 *  @n Ref: DS p.3 Table 1, "Sensitivity at XOUT, YOUT, ZOUT" */
#define ADXL345_SENSITIVITY_2G          256U

/** LSB/g, 10-bit, +-4 g.
 *  @n Ref: DS p.3 Table 1, "Sensitivity at XOUT, YOUT, ZOUT" */
#define ADXL345_SENSITIVITY_4G          128U

/** LSB/g, 10-bit, +-8 g.
 *  @n Ref: DS p.3 Table 1, "Sensitivity at XOUT, YOUT, ZOUT" */
#define ADXL345_SENSITIVITY_8G          64U

/** LSB/g, 10-bit, +-16 g.
 *  @n Ref: DS p.3 Table 1, "Sensitivity at XOUT, YOUT, ZOUT" */
#define ADXL345_SENSITIVITY_16G         32U

/** @} */

/**
 * @brief Output data rate (Rate field of the BW_RATE register).
 *
 * The bandwidth is ODR / 2. 3200 Hz and 1600 Hz need SPI >= 2 MHz, 800 Hz
 * needs >= 400 kHz and the remaining rates scale proportionally (e.g. I2C at
 * 100 kHz limits the ODR to 200 Hz).
 *
 * Ref: DS p.13 Table 7 (rate codes); p.25 Table 25; p.14 "SPI" and p.17 "I2C"
 * (bus speeds)
 */
typedef enum {
    ADXL345_ODR_0_10Hz = 0x00,  /**< 0.10 Hz */
    ADXL345_ODR_0_20Hz = 0x01,  /**< 0.20 Hz */
    ADXL345_ODR_0_39Hz = 0x02,  /**< 0.39 Hz */
    ADXL345_ODR_0_78Hz = 0x03,  /**< 0.78 Hz */
    ADXL345_ODR_1_56Hz = 0x04,  /**< 1.56 Hz */
    ADXL345_ODR_3_13Hz = 0x05,  /**< 3.13 Hz */
    ADXL345_ODR_6_25Hz = 0x06,  /**< 6.25 Hz */
    ADXL345_ODR_12_5Hz = 0x07,  /**< 12.5 Hz */
    ADXL345_ODR_25Hz = 0x08,    /**< 25 Hz */
    ADXL345_ODR_50Hz = 0x09,    /**< 50 Hz */
    ADXL345_ODR_100Hz = 0x0A,   /**< 100 Hz (reset value) */
    ADXL345_ODR_200Hz = 0x0B,   /**< 200 Hz */
    ADXL345_ODR_400Hz = 0x0C,   /**< 400 Hz */
    ADXL345_ODR_800Hz = 0x0D,   /**< 800 Hz */
    ADXL345_ODR_1600Hz = 0x0E,  /**< 1600 Hz */
    ADXL345_ODR_3200Hz = 0x0F   /**< 3200 Hz */
} ADXL345_ODR_t;

/**
 * @brief Measurement range (Range field of the DATA_FORMAT register).
 *
 * Ref: DS p.27 Table 35
 */
typedef enum {
    ADXL345_RANGE_2G = 0x00,    /**< +/-2 g (reset value) */
    ADXL345_RANGE_4G = 0x01,    /**< +/-4 g */
    ADXL345_RANGE_8G = 0x02,    /**< +/-8 g */
    ADXL345_RANGE_16G = 0x03    /**< +/-16 g */
} ADXL345_Range_t;

/**
 * @brief Polarity of the INT1 and INT2 pins (INT_INVERT bit of DATA_FORMAT).
 *
 * Ref: DS p.27 Table 34, "INT_INVERT Bit"; p.19 "Interrupts"
 */
typedef enum {
    ADXL345_INT_POL_HIGH = 0x00,    /**< Interrupts are active high (reset
                                     *   value) */
    ADXL345_INT_POL_LOW = 0x20      /**< Interrupts are active low */
} ADXL345_IntPol_t;

/**
 * @name ADXL345 Interrupt sources
 * Interrupt sources (INT_ENABLE, INT_MAP and INT_SOURCE bits).
 *
 * Values are bit masks and can be combined. They are unsigned defines rather
 * than an enum because they are combined with bitwise operators (TYP-12).
 *
 * Ref: DS p.26 Tables 28-30; p.27 Tables 31-33 (bit positions); p.20
 * "Interrupts" (meaning)
 * @{
 */
#define ADXL345_INT_NONE        0x00U   /**< No interrupt */
#define ADXL345_INT_OVERRUN     0x01U   /**< Data overwritten before being
                                         *   read */
#define ADXL345_INT_WATERMARK   0x02U   /**< FIFO entries reached FIFO_CTL
                                         *   Samples */
#define ADXL345_INT_FREE_FALL   0x04U   /**< All axes below THRESH_FF for
                                         *   TIME_FF */
#define ADXL345_INT_INACTIVITY  0x08U   /**< Participating axes below
                                         *   THRESH_INACT for TIME_INACT */
#define ADXL345_INT_ACTIVITY    0x10U   /**< A participating axis above
                                         *   THRESH_ACT */
#define ADXL345_INT_DOUBLE_TAP  0x20U   /**< Double tap detected */
#define ADXL345_INT_SINGLE_TAP  0x40U   /**< Single tap detected */
#define ADXL345_INT_DATA_READY  0x80U   /**< New data available */
/** @} */

/**
 * @brief Interrupt pin (INT_MAP bits and Trigger bit of FIFO_CTL).
 *
 * Ref: DS p.27 "Register 0x2F" (0 = INT1, 1 = INT2); p.28 "Trigger Bit"
 */
typedef enum {
    ADXL345_INT1 = 0,   /**< INT1 pin */
    ADXL345_INT2 = 1    /**< INT2 pin */
} ADXL345_IntPin_t;

/**
 * @brief Operating mode (Measure bit of POWER_CTL).
 *
 * The device powers up in standby.
 *
 * Ref: DS p.26 Table 26, "Measure Bit"; p.13 "Standby Mode"
 */
typedef enum {
    ADXL345_STANDBY = 0x00, /**< Standby, ~0.1 uA. FIFO contents are kept */
    ADXL345_MEASURE = 0x08  /**< Measuring */
} ADXL345_MeasurementMode_t;

/**
 * @brief Frequency of readings in sleep mode (Wakeup field of POWER_CTL).
 *
 * Ref: DS p.26 Table 27
 */
typedef enum {
    ADXL345_WAKEUP_8Hz = 0x00,  /**< 8 Hz */
    ADXL345_WAKEUP_4Hz = 0x01,  /**< 4 Hz */
    ADXL345_WAKEUP_2Hz = 0x02,  /**< 2 Hz */
    ADXL345_WAKEUP_1Hz = 0x03   /**< 1 Hz */
} ADXL345_Wakeup_t;

/**
 * @brief FIFO mode (FIFO_MODE field of FIFO_CTL).
 *
 * Ref: DS p.28 Table 37; p.21 "FIFO"
 */
typedef enum {
    ADXL345_FIFO_BYPASS = 0x00,     /**< FIFO not used */
    ADXL345_FIFO_FIFO = 0x40,       /**< Collects up to 32 sets then stops */
    ADXL345_FIFO_STREAM = 0x80,     /**< Holds the last 32 sets, oldest
                                     *   overwritten */
    ADXL345_FIFO_TRIGGER = 0xC0     /**< Holds the last n sets before the
                                     *   trigger event, then FIFO mode */
} ADXL345_FifoMode_t;

/**
 * @brief Coupling of activity and inactivity detection (AC/DC bits of
 *        ACT_INACT_CTL).
 *
 * Ref: DS p.24 "ACT AC/DC and INACT AC/DC Bits"
 */
typedef enum {
    ADXL345_DC_COUPLED = 0, /**< Compare acceleration magnitude directly with
                             *   the threshold */
    ADXL345_AC_COUPLED = 1  /**< Compare the change from a reference value
                             *   with the threshold */
} ADXL345_Coupling_t;

/**
 * @name ADXL345 Axes flags
 * Axes included in activity, inactivity or tap detection.
 *
 * Values are bit masks and can be combined (unsigned defines, TYP-12).
 *
 * Ref: DS p.24 Tables 21-22; p.25 Table 23 (X, Y, Z enable order, shifted per
 * register)
 * @{
 */
#define ADXL345_AXES_NONE   0x00U   /**< No axis (detection off) */
#define ADXL345_AXIS_Z_EN   0x01U   /**< Z */
#define ADXL345_AXIS_Y_EN   0x02U   /**< Y */
#define ADXL345_AXIS_X_EN   0x04U   /**< X */
#define ADXL345_AXES_ALL    0x07U   /**< X, Y and Z */
/** @} */

/**
 * @brief Accelerometer axis.
 *
 * The value is the address offset of the axis offset register from
 * ADXL345_REG_OFSX.
 *
 * Ref: DS p.23 Table 19 (OFSX, OFSY, OFSZ at consecutive addresses)
 */
typedef enum {
    ADXL345_X_AXIS = 0, /**< X axis */
    ADXL345_Y_AXIS = 1, /**< Y axis */
    ADXL345_Z_AXIS = 2  /**< Z axis */
} ADXL345_Axis_t;

/**
 * @brief Status returned by the driver functions.
 *
 * Value 8 (RESET_ERROR in the ADXL355 driver) is not used: the ADXL345 has
 * no soft reset.
 */
typedef enum {
    ADXL345_OK = 0,                /**< Success */
    ADXL345_SPI_ERROR = 1,         /**< Bus transfer failed */
    ADXL345_SPI_RECEIVE_ERROR = 2, /**< Bus receive failed */
    ADXL345_SPI_SEND_ERROR = 3,    /**< Bus send failed */
    ADXL345_DATA_ERROR = 5,        /**< Invalid data */
    ADXL345_INVALID_PARAMETER = 6, /**< Invalid function parameter */
    ADXL345_ERROR = 7,             /**< Unspecified error */
    ADXL345_INIT_ERROR = 9,        /**< Initialisation failed */
    ADXL345_TIMEOUT = 10,          /**< Wait for the device or bus expired */
    ADXL345_UNEXPECTED_READ_VALUE = 11 /**< Register holds an unexpected
                                            value */
} ADXL345_Status_t;

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
     *  (SPI: [R/W][MB][A5..A0], MB set when data_size > 1) or the I2C
     *  addressing, so an SPI transfer is data_size + 1 bytes and buffer gets
     *  the data only. */
    ADXL345_Status_t (*read)(const uint8_t address, uint8_t *buffer,
                             const uint16_t data_size);

    /** Writes data_size bytes starting at register address (blocking).
     *  data_size counts data bytes only: the platform adds the command byte
     *  (SPI: [R/W][MB][A5..A0] with R/W clear) or the I2C addressing, so
     *  writing one register is a 2-byte SPI transfer. */
    ADXL345_Status_t (*write)(const uint8_t address, const uint8_t *buffer,
                              const uint16_t data_size);

    /** Blocking delay in milliseconds (not used by the ADXL345 driver, may
     *  be NULL) */
    void (*delay_ms)(uint32_t delay_ms);
} ADXL345_Platform_t;

/**
 * @brief Raw three-axis sample set, as stored in the DATAX, DATAY and DATAZ
 *        registers (two's complement, right justified, LSB first).
 */
typedef struct {
    uint8_t x_axis[ADXL345_BYTES_PER_SAMPLE]; /**< DATAX0..1 */
    uint8_t y_axis[ADXL345_BYTES_PER_SAMPLE]; /**< DATAY0..1 */
    uint8_t z_axis[ADXL345_BYTES_PER_SAMPLE]; /**< DATAZ0..1 */
} ADXL345_RawSampleSet_t;

/**
 * @brief Reads one register.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address (0x00 or 0x1D to 0x39).
 * @param[out] buffer Register value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or reg_address is reserved.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_ReadRegister(const ADXL345_Platform_t *platform,
                                      const uint8_t reg_address,
                                      uint8_t *buffer);

/**
 * @brief Writes one register.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address (0x00 or 0x1D to 0x39).
 * @param[in] data Value to write.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or reg_address is reserved.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_WriteRegister(const ADXL345_Platform_t *platform,
                                       const uint8_t reg_address,
                                       const uint8_t data);

/**
 * @brief Reads one register and keeps only the bits selected by mask.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address (0x00 or 0x1D to 0x39).
 * @param[in] mask Bits to keep.
 * @param[out] buffer Masked register value, not shifted. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or reg_address is reserved.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_ReadRegisterMasked(const ADXL345_Platform_t *platform,
                                            const uint8_t reg_address,
                                            const uint8_t mask,
                                            uint8_t *buffer);

/**
 * @brief Read-modify-write of the bits of a register selected by mask.
 *
 * The bits in mask are cleared and data is OR-ed into the register; the
 * other bits keep their value.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] reg_address Register address (0x00 or 0x1D to 0x39).
 * @param[in] mask Bits to change.
 * @param[in] data New value of the field, already in position.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         reg_address is reserved, or data has bits outside mask.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_WriteRegisterMasked(const ADXL345_Platform_t *platform,
                                             const uint8_t reg_address,
                                             const uint8_t mask,
                                             const uint8_t data);

// Device ID functions

/**
 * @brief Reads the device ID register (expected ADXL345_DEVID).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] dev_id Register value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetDevId(const ADXL345_Platform_t *platform,
                                  uint8_t *dev_id);

/**
 * @brief Reads the device ID register and checks it against ADXL345_DEVID.
 *
 * Use it to confirm communication with the device after power up.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL345_OK Success: an ADXL345 answered.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @retval ADXL345_UNEXPECTED_READ_VALUE The register does not hold
 *         ADXL345_DEVID.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_CheckDevId(const ADXL345_Platform_t *platform);

// Acceleration data functions

/**
 * @brief Reads the current 3-axis measurement.
 *
 * Returns the DATAX, DATAY and DATAZ registers, read in one transfer so the
 * three axes belong to the same measurement, without unpacking. When the
 * FIFO is enabled, this pops one sample set from the FIFO.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] raw_sample_set Raw sample set. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_ReadRawSampleSet(
    const ADXL345_Platform_t *platform,
    ADXL345_RawSampleSet_t *raw_sample_set);

/**
 * @brief Reads the offset of one axis.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axis Axis to read.
 * @param[out] axis_offset Offset, two's complement. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or axis is not a valid ADXL345_Axis_t.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetAxisOffset(const ADXL345_Platform_t *platform,
                                       const ADXL345_Axis_t axis,
                                       int8_t *axis_offset);

/**
 * @brief Writes the offset of one axis (OFSX, OFSY or OFSZ).
 *
 * The device adds the offset to the measurement. Its LSB is 15.6 mg (see
 * ADXL345_UTIL_Offset2LSB()).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axis Axis to write.
 * @param[in] offset Offset, two's complement.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or axis is not a valid ADXL345_Axis_t.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetAxisOffset(const ADXL345_Platform_t *platform,
                                       const ADXL345_Axis_t axis,
                                       const int8_t offset);

// FIFO Functions

/**
 * @brief Reads the FIFO mode (FIFO_MODE field of FIFO_CTL).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] fifo_mode FIFO mode. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetFifoMode(const ADXL345_Platform_t *platform,
                                     ADXL345_FifoMode_t *fifo_mode);

/**
 * @brief Sets the FIFO mode (FIFO_MODE field of FIFO_CTL).
 *
 * Setting ADXL345_FIFO_BYPASS clears the FIFO.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] fifo_mode FIFO mode.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or fifo_mode is not a valid ADXL345_FifoMode_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetFifoMode(const ADXL345_Platform_t *platform,
                                     const ADXL345_FifoMode_t fifo_mode);

/**
 * @brief Reads the FIFO watermark (Samples field of FIFO_CTL).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] fifo_samples Number of sample sets that sets the watermark
 *             interrupt. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetFifoSamples(const ADXL345_Platform_t *platform,
                                        uint8_t *fifo_samples);

/**
 * @brief Sets the FIFO watermark (Samples field of FIFO_CTL).
 *
 * The field counts sample sets. In trigger mode it is the number of sample
 * sets kept before the trigger event. 0 sets the watermark interrupt
 * immediately and should not be used in trigger mode.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] fifo_samples Number of sample sets that sets the watermark
 *            interrupt (0 to 31).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER fifo_samples is out of range, or a
 *         required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetFifoSamples(const ADXL345_Platform_t *platform,
                                        const uint8_t fifo_samples);

/**
 * @brief Selects the interrupt pin whose event triggers the FIFO in trigger
 *        mode (Trigger bit of FIFO_CTL).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] pin Interrupt pin.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or pin is not a valid ADXL345_IntPin_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetFifoTrigger(const ADXL345_Platform_t *platform,
                                        const ADXL345_IntPin_t pin);

/**
 * @brief Reads the number of sample sets currently stored in the FIFO.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] num_fifo_entries Number of sample sets (0 to
 *             ADXL345_FIFO_MAX_SAMPLE_SETS). Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_ReadNumberFifoEntries(
    const ADXL345_Platform_t *platform, uint8_t *num_fifo_entries);

/**
 * @brief Reads whether a trigger event occurred (FIFO_TRIG bit of
 *        FIFO_STATUS).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] triggered true if a trigger event occurred (trigger mode).
 *             Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetFifoTriggered(const ADXL345_Platform_t *platform,
                                          bool *triggered);

/**
 * @brief Reads sample sets from the FIFO.
 *
 * Each read of the DATAX, DATAY and DATAZ registers pops one sample set, so
 * the FIFO is read with one 6-byte transfer per sample set. The device needs
 * at least 5 us between the end of one read (CS high) and the start of the
 * next: above 1.6 MHz SPI, the time between transfers must make up the
 * difference.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] sample_sets Array of at least num_sample_sets elements.
 *             Must not be NULL.
 * @param[in] num_sample_sets Sample sets to read
 *            (1 to ADXL345_FIFO_MAX_READ_SETS).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or num_sample_sets is out of range.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_ReadFifoSampleSet(const ADXL345_Platform_t *platform,
                                           ADXL345_RawSampleSet_t *sample_sets,
                                           const uint8_t num_sample_sets);

// Activity and inactivity functions

/**
 * @brief Reads the activity detection threshold (THRESH_ACT).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] act_threshold Threshold, 62.5 mg/LSB. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetActivityThreshold(
    const ADXL345_Platform_t *platform, uint8_t *act_threshold);

/**
 * @brief Writes the activity detection threshold (THRESH_ACT).
 *
 * The threshold is compared with the acceleration of each enabled axis.
 * 0 may cause undesirable behaviour with the activity interrupt enabled.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] act_threshold Threshold, 62.5 mg/LSB.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetActivityThreshold(
    const ADXL345_Platform_t *platform, const uint8_t act_threshold);

/**
 * @brief Selects the axes and the coupling used for activity detection
 *        (activity bits of ACT_INACT_CTL).
 *
 * Activity is detected when any enabled axis is above the threshold.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axes Axes to enable (OR of ADXL345_AXIS_x_EN flags).
 *            ADXL345_AXES_NONE disables activity detection.
 * @param[in] coupling dc or ac coupled comparison.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         axes has bits above ADXL345_AXES_ALL, or coupling is not a valid
 *         ADXL345_Coupling_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_EnableActivity(const ADXL345_Platform_t *platform,
                                        const uint8_t axes,
                                        const ADXL345_Coupling_t coupling);

/**
 * @brief Disables activity detection on all axes.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_DisableActivity(const ADXL345_Platform_t *platform);

/**
 * @brief Reads the inactivity detection threshold (THRESH_INACT).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] inact_threshold Threshold, 62.5 mg/LSB. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInactivityThreshold(
    const ADXL345_Platform_t *platform, uint8_t *inact_threshold);

/**
 * @brief Writes the inactivity detection threshold (THRESH_INACT).
 *
 * 0 may cause undesirable behaviour with the inactivity interrupt enabled.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] inact_threshold Threshold, 62.5 mg/LSB.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetInactivityThreshold(
    const ADXL345_Platform_t *platform, const uint8_t inact_threshold);

/**
 * @brief Reads the inactivity time (TIME_INACT).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] inact_time_s Time below the threshold needed to detect
 *             inactivity, 1 s/LSB. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInactivityTime(const ADXL345_Platform_t *platform,
                                           uint8_t *inact_time_s);

/**
 * @brief Writes the inactivity time (TIME_INACT).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] inact_time_s Time below the threshold needed to detect
 *            inactivity, 1 s/LSB. 0 detects inactivity as soon as the
 *            output is below the threshold.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetInactivityTime(const ADXL345_Platform_t *platform,
                                           const uint8_t inact_time_s);

/**
 * @brief Selects the axes and the coupling used for inactivity detection
 *        (inactivity bits of ACT_INACT_CTL).
 *
 * Inactivity is detected when all enabled axes are below the threshold.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axes Axes to enable (OR of ADXL345_AXIS_x_EN flags).
 *            ADXL345_AXES_NONE disables inactivity detection.
 * @param[in] coupling dc or ac coupled comparison.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         axes has bits above ADXL345_AXES_ALL, or coupling is not a valid
 *         ADXL345_Coupling_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_EnableInactivity(const ADXL345_Platform_t *platform,
                                          const uint8_t axes,
                                          const ADXL345_Coupling_t coupling);

/**
 * @brief Disables inactivity detection on all axes.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_DisableInactivity(const ADXL345_Platform_t *platform);

// Tap functions

/**
 * @brief Reads the tap threshold (THRESH_TAP).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] tap_threshold Threshold, 62.5 mg/LSB. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetTapThreshold(const ADXL345_Platform_t *platform,
                                         uint8_t *tap_threshold);

/**
 * @brief Writes the tap threshold (THRESH_TAP).
 *
 * 0 may cause undesirable behaviour with the tap interrupts enabled.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] tap_threshold Threshold, 62.5 mg/LSB.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetTapThreshold(const ADXL345_Platform_t *platform,
                                         const uint8_t tap_threshold);

/**
 * @brief Writes the tap duration (DUR): the maximum time an event can be
 *        above the threshold to be a tap.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] duration Duration, 625 us/LSB. 0 disables the single and double
 *            tap functions.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetTapDuration(const ADXL345_Platform_t *platform,
                                        const uint8_t duration);

/**
 * @brief Writes the tap latency (LATENT): the wait from a tap to the start
 *        of the double tap window.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] latency Latency, 1.25 ms/LSB. 0 disables the double tap
 *            function.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetTapLatency(const ADXL345_Platform_t *platform,
                                       const uint8_t latency);

/**
 * @brief Writes the tap window (WINDOW): the time, after the latency,
 *        during which a second tap can start.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] window Window, 1.25 ms/LSB. 0 disables the double tap function.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetTapWindow(const ADXL345_Platform_t *platform,
                                      const uint8_t window);

/**
 * @brief Selects the axes used for tap detection and the double tap
 *        suppress bit (TAP_AXES register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] axes Axes to enable (OR of ADXL345_AXIS_x_EN flags).
 * @param[in] suppress true to suppress a double tap when the acceleration
 *            is above the threshold between the taps.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or axes has bits above ADXL345_AXES_ALL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetTapAxes(const ADXL345_Platform_t *platform,
                                    const uint8_t axes, const bool suppress);

/**
 * @brief Reads the ACT_TAP_STATUS register: the first axis involved in a tap
 *        or activity event, and the asleep bit.
 *
 * Read it before clearing the interrupt (reading INT_SOURCE).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] status Register value (ADXL345_ACT_x_SOURCE,
 *             ADXL345_TAP_x_SOURCE and ADXL345_ASLEEP_MASK bits).
 *             Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetActTapStatus(const ADXL345_Platform_t *platform,
                                         uint8_t *status);

// Free-fall functions
/**
 * @brief Writes the free-fall threshold (THRESH_FF), compared with all axes.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] ff_threshold Threshold, 62.5 mg/LSB. 300 mg to 600 mg
 *            (0x05 to 0x09) recommended.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetFreeFallThreshold(
    const ADXL345_Platform_t *platform, const uint8_t ff_threshold);

/**
 * @brief Writes the free-fall time (TIME_FF): the minimum time all axes must
 *        be below the threshold to detect a free fall.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] ff_time Time, 5 ms/LSB. 100 ms to 350 ms (0x14 to 0x46)
 *            recommended.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetFreeFallTime(const ADXL345_Platform_t *platform,
                                         const uint8_t ff_time);

// Interrupts

/**
 * @brief Sets the polarity of the INT1 and INT2 pins.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_pol Polarity.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or int_pol is not a valid
 *         ADXL345_IntPol_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetInterruptPolarity(
    const ADXL345_Platform_t *platform, const ADXL345_IntPol_t int_pol);

/**
 * @brief Reads the polarity of the INT1 and INT2 pins.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_pol Polarity. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInterruptPolarity(
    const ADXL345_Platform_t *platform, ADXL345_IntPol_t *int_pol);

/**
 * @brief Maps, with a single write, all the interrupt sources to INT1 or
 *        INT2.
 *
 * Following ADXL345 INT_MAP register, each bit of int_map is connected to an
 * interrupt source as follows
 *
 *  DATA_READY SINGLE_TAP DOUBLE_TAP Activity Inactivity FREE_FALL Watermark
 *  Overrun
 *
 *  A bit at 1 maps the source to INT2 and a bit at 0 maps it to INT1
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_map New INT_MAP value (OR of ADXL345_INT_x sources).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetInterruptMap(const ADXL345_Platform_t *platform,
                                         const uint8_t int_map);

/**
 * @brief Reads the interrupt map (INT_MAP register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_map INT_MAP value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInterruptMap(const ADXL345_Platform_t *platform,
                                         uint8_t *int_map);

/**
 * @brief Maps the given interrupt source(s) to INT1 or INT2; the others are
 *        kept.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_src Interrupt source(s) to map.
 * @param[in] pin Interrupt pin.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or pin is not a valid ADXL345_IntPin_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_MapInterrupt(const ADXL345_Platform_t *platform,
                                      const uint8_t int_src,
                                      const ADXL345_IntPin_t pin);

/**
 * @brief Enables, with a single write, the given interrupt sources and
 *        disables the others (INT_ENABLE register).
 *
 * Configure the functions (thresholds, times, axes) before enabling their
 * interrupts. DATA_READY, watermark and overrun only gate the output: the
 * functions are always on.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_enable New INT_ENABLE value (OR of ADXL345_INT_x sources).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_SetInterruptEnable(const ADXL345_Platform_t *platform,
                                            const uint8_t int_enable);

/**
 * @brief Reads the enabled interrupt sources (INT_ENABLE register).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_enable INT_ENABLE value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInterruptEnable(const ADXL345_Platform_t *platform,
                                            uint8_t *int_enable);

/**
 * @brief Disables, with a single write, all the available interrupts.
 *
 * @param[in] platform Platform services. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the write callback.
 */
ADXL345_Status_t ADXL345_ClearInterrupts(const ADXL345_Platform_t *platform);

/**
 * @brief Enables the given interrupt source(s); the others are kept.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_src Interrupt source(s) to enable.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_EnableInterrupt(const ADXL345_Platform_t *platform,
                                         const uint8_t int_src);

/**
 * @brief Disables the given interrupt source(s); the others are kept.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] int_src Interrupt source(s) to disable.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_DisableInterrupt(const ADXL345_Platform_t *platform,
                                          const uint8_t int_src);

/**
 * @brief Reads the interrupt sources that triggered (INT_SOURCE register).
 *
 * Reading it clears the single tap, double tap, activity, inactivity and
 * free-fall bits. DATA_READY, watermark and overrun are cleared by reading
 * the data registers.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] int_source INT_SOURCE value. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetInterruptSource(const ADXL345_Platform_t *platform,
                                            uint8_t *int_source);

// ADXL345 operation (ODRs, RANGE, DATA_FORMAT, POWER_CTL)

/**
 * @brief Reads the measurement range.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] range Measurement range. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetRange(const ADXL345_Platform_t *platform,
                                  ADXL345_Range_t *range);

/**
 * @brief Sets the measurement range.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] range Measurement range.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or range is not a valid ADXL345_Range_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetRange(const ADXL345_Platform_t *platform,
                                  const ADXL345_Range_t range);

/**
 * @brief Reads the output data rate.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] odr Output data rate. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetODR(const ADXL345_Platform_t *platform,
                                ADXL345_ODR_t *odr);

/**
 * @brief Sets the output data rate.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] odr Output data rate (see ADXL345_ODR_t for the minimum bus
 *            speeds).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or odr is not a valid ADXL345_ODR_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetODR(const ADXL345_Platform_t *platform,
                                const ADXL345_ODR_t odr);

/**
 * @brief Sets the reduced power operation (LOW_POWER bit of BW_RATE).
 *
 * Only reduces the current from 12.5 Hz to 400 Hz, with slightly higher
 * noise.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] low_power true for reduced power operation.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetLowPower(const ADXL345_Platform_t *platform,
                                     const bool low_power);

/**
 * @brief Reads the resolution (FULL_RES bit of DATA_FORMAT).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] full_res true in full resolution. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetFullResolution(const ADXL345_Platform_t *platform,
                                           bool *full_res);

/**
 * @brief Sets the resolution (FULL_RES bit of DATA_FORMAT).
 *
 * Full resolution keeps ~3.9 mg/LSB at every range (up to 13 bits at
 * +/-16 g). Otherwise the output is 10 bits and the scale factor depends on
 * the range.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] full_res true for full resolution.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetFullResolution(const ADXL345_Platform_t *platform,
                                           const bool full_res);

/**
 * @brief Applies or removes the self-test force (SELF_TEST bit of
 *        DATA_FORMAT).
 *
 * For valid results use normal power and an ODR of 100 Hz to 800 Hz or
 * 3200 Hz.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] self_test true to apply the self-test force.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetSelfTest(const ADXL345_Platform_t *platform,
                                     const bool self_test);

/**
 * @brief Reads the operating mode (Measure bit of POWER_CTL).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[out] adxl_mode Operating mode. Must not be NULL.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the read callback.
 */
ADXL345_Status_t ADXL345_GetMode(const ADXL345_Platform_t *platform,
                                 ADXL345_MeasurementMode_t *adxl_mode);

/**
 * @brief Sets the operating mode (Measure bit of POWER_CTL).
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] adxl_mode Operating mode.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or adxl_mode is not a valid
 *         ADXL345_MeasurementMode_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetMode(const ADXL345_Platform_t *platform,
                                 const ADXL345_MeasurementMode_t adxl_mode);

/**
 * @brief Sets the Link and AUTO_SLEEP bits of POWER_CTL.
 *
 * AUTO_SLEEP only has effect with link set. When clearing either bit, go to
 * standby and back to measure afterwards so the device is properly biased.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] link true so that activity detection waits for inactivity and
 *            vice versa.
 * @param[in] auto_sleep true to sleep on inactivity and wake up on activity.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetLinkAutoSleep(const ADXL345_Platform_t *platform,
                                          const bool link,
                                          const bool auto_sleep);

/**
 * @brief Sets the Sleep and Wakeup bits of POWER_CTL.
 *
 * Sleep suppresses DATA_READY, stops the FIFO and samples at the wakeup rate;
 * only activity detection works. When clearing sleep, go to standby and back
 * to measure afterwards.
 *
 * @param[in] platform Platform services. Must not be NULL.
 * @param[in] sleep true to enter sleep mode.
 * @param[in] wakeup Frequency of readings in sleep mode.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER A required pointer or bus callback is NULL,
 *         or wakeup is not a valid ADXL345_Wakeup_t.
 * @return Otherwise, the status returned by the bus callbacks.
 */
ADXL345_Status_t ADXL345_SetSleep(const ADXL345_Platform_t *platform,
                                  const bool sleep,
                                  const ADXL345_Wakeup_t wakeup);

#endif // ADXL345_H
