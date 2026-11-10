/**
 * @file adxl345_stm32wl.h
 * @brief STM32 HAL bus interface for the ADXL345 driver (SPI and I2C).
 *
 * Provides the ADXL345_Platform_t the driver needs, built on the STM32 HAL:
 * blocking SPI read and write, blocking I2C read and write, and a blocking
 * delay. The SPI part is built when HAL_SPI_MODULE_ENABLED is defined, the
 * I2C part when HAL_I2C_MODULE_ENABLED is defined.
 *
 * The driver callbacks carry no context, so the module keeps one SPI and one
 * I2C configuration. With more than one sensor on a bus, call
 * ADXL345_STM32WL_ConfigSpi() or ADXL345_STM32WL_ConfigI2c() again before
 * switching to another sensor.
 *
 * Unlike the ADXL355 module there is no DMA read: the ADXL345 FIFO is read
 * one sample set (6 bytes) per transaction, so DMA would not help.
 *
 * This is a platform module: it includes the STM32WL HAL header. For another
 * STM32 family, change that include only.
 *
 * @author Raul Casquinha
 * @date Oct/2026
 */

#ifndef ADXL345_STM32WL_H
#define ADXL345_STM32WL_H

#include <stdint.h>

#include "adxl345.h"
#include "stm32wlxx_hal.h"

/** Timeout of one blocking SPI transfer (the longest transfer, 64 bytes,
 *  takes 0.1 ms at 5 MHz and 0.5 ms at 1 MHz) */
#define ADXL345_STM32WL_SPI_TIMEOUT_MS      10U

/** Fixed part of the timeout of one blocking I2C transfer */
#define ADXL345_STM32WL_I2C_BASE_TIMEOUT_MS 10U

/** Bytes per millisecond at 100 kHz (9 clocks per byte), used to extend the
 *  I2C timeout with the transfer size */
#define ADXL345_STM32WL_I2C_BYTES_PER_MS    11U

/** Longest transfer: the whole 6-bit register address space (A5..A0) */
#define ADXL345_STM32WL_MAX_TRANSFER_BYTES  64U

/**
 * @brief Status returned by the configuration functions.
 *
 * The bus functions return ADXL345_Status_t, as the driver requires.
 */
typedef enum {
    ADXL345_STM32WL_OK = 0,               /**< Success */
    ADXL345_STM32WL_INVALID_PARAMETER = 1 /**< Invalid function parameter */
} ADXL345_STM32WL_Status_t;

/**
 * @brief Blocking delay (ADXL345_Platform_t.delay_ms).
 *
 * @param[in] delay_ms Delay in milliseconds (HAL_Delay() adds up to 1 ms).
 *
 * @note Not for interrupt context (needs the SysTick interrupt).
 */
void ADXL345_STM32WL_DelayMs(uint32_t delay_ms);

#ifdef HAL_SPI_MODULE_ENABLED

/**
 * @brief Sets the SPI bus and chip select of the ADXL345 and drives CS high.
 *
 * @param[in] hspi SPI handle (already initialised: master, 8-bit, CPOL = 1,
 *            CPHA = 1, at most 5 MHz). Must not be NULL.
 * @param[in] cs_port GPIO port of CS (output, already initialised). Must
 *            not be NULL.
 * @param[in] cs_pin GPIO pin of CS (one GPIO_PIN_x value).
 *
 * @retval ADXL345_STM32WL_OK Success.
 * @retval ADXL345_STM32WL_INVALID_PARAMETER A pointer is NULL or cs_pin is
 *         0.
 *
 * @note Call before any SPI transfer, and again to switch to another sensor.
 */
ADXL345_STM32WL_Status_t ADXL345_STM32WL_ConfigSpi(SPI_HandleTypeDef *hspi,
                                                   GPIO_TypeDef *cs_port,
                                                   uint16_t cs_pin);

/**
 * @brief Reads data_size bytes from the ADXL345 over SPI, blocking
 *        (ADXL345_Platform_t.read).
 *
 * Sends the command byte ([R/W][MB][A5..A0], MB set when data_size > 1) and
 * receives the data with CS held low.
 *
 * @param[in] address First register address (0x00 to 0x3F).
 * @param[out] buffer Data read. Must not be NULL.
 * @param[in] data_size Bytes to read (1 to
 *            ADXL345_STM32WL_MAX_TRANSFER_BYTES).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER Not configured, address above 0x3F,
 *         buffer is NULL or data_size is out of range.
 * @retval ADXL345_TIMEOUT The transfer did not end within
 *         ADXL345_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL345_SPI_SEND_ERROR The HAL failed to send the command byte.
 * @retval ADXL345_SPI_RECEIVE_ERROR The HAL failed to receive the data.
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL345_Status_t ADXL345_STM32WL_ReadSpi(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size);

/**
 * @brief Writes data_size bytes to the ADXL345 over SPI, blocking
 *        (ADXL345_Platform_t.write).
 *
 * Sends the command byte ([R/W][MB][A5..A0], MB set when data_size > 1) and
 * the data with CS held low.
 *
 * @param[in] address First register address (0x00 to 0x3F).
 * @param[in] buffer Data to write. Must not be NULL.
 * @param[in] data_size Bytes to write (1 to
 *            ADXL345_STM32WL_MAX_TRANSFER_BYTES).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER Not configured, address above 0x3F,
 *         buffer is NULL or data_size is out of range.
 * @retval ADXL345_TIMEOUT The transfer did not end within
 *         ADXL345_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL345_SPI_SEND_ERROR The HAL reported another error.
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL345_Status_t ADXL345_STM32WL_WriteSpi(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size);

/**
 * @brief Gets the platform services for the ADXL345 driver on SPI.
 *
 * @return Pointer to static platform services with ReadSpi, WriteSpi
 *         and DelayMs (never NULL).
 *
 * @note Call ADXL345_STM32WL_ConfigSpi() before using them.
 */
const ADXL345_Platform_t *ADXL345_STM32WL_GetSpiPlatform(void);

#endif // HAL_SPI_MODULE_ENABLED

#ifdef HAL_I2C_MODULE_ENABLED

/**
 * @brief Sets the I2C bus and address of the ADXL345.
 *
 * @param[in] hi2c I2C handle (already initialised). Must not be NULL.
 * @param[in] address_7bit 7-bit device address (ADXL345_I2C_ADDRESS_ALT_LOW
 *            with ALT ADDRESS low, ADXL345_I2C_ADDRESS_ALT_HIGH with it
 *            high).
 *
 * @retval ADXL345_STM32WL_OK Success.
 * @retval ADXL345_STM32WL_INVALID_PARAMETER hi2c is NULL or address_7bit
 *         is not one of the two ADXL345 addresses.
 *
 * @note Call before any I2C transfer, and again to switch to another sensor.
 */
ADXL345_STM32WL_Status_t ADXL345_STM32WL_ConfigI2c(I2C_HandleTypeDef *hi2c,
                                                   uint8_t address_7bit);

/**
 * @brief Reads data_size bytes from the ADXL345 over I2C, blocking
 *        (ADXL345_Platform_t.read).
 *
 * @param[in] address First register address.
 * @param[out] buffer Data read. Must not be NULL.
 * @param[in] data_size Bytes to read (1 to
 *            ADXL345_STM32WL_MAX_TRANSFER_BYTES).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL345_TIMEOUT The transfer did not end in time (10 ms plus the
 *         transfer time at 100 kHz).
 * @retval ADXL345_SPI_RECEIVE_ERROR The HAL reported another error (bus
 *         receive failed; the driver has no I2C-specific status).
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL345_Status_t ADXL345_STM32WL_ReadI2c(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size);

/**
 * @brief Writes data_size bytes to the ADXL345 over I2C, blocking
 *        (ADXL345_Platform_t.write).
 *
 * @param[in] address First register address.
 * @param[in] buffer Data to write. Must not be NULL.
 * @param[in] data_size Bytes to write (1 to
 *            ADXL345_STM32WL_MAX_TRANSFER_BYTES).
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL345_TIMEOUT The transfer did not end in time (10 ms plus the
 *         transfer time at 100 kHz).
 * @retval ADXL345_SPI_SEND_ERROR The HAL reported another error (bus send
 *         failed; the driver has no I2C-specific status).
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL345_Status_t ADXL345_STM32WL_WriteI2c(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size);

/**
 * @brief Gets the platform services for the ADXL345 driver on I2C.
 *
 * @return Pointer to static platform services with ReadI2c, WriteI2c and
 *         DelayMs (never NULL).
 *
 * @note Call ADXL345_STM32WL_ConfigI2c() before using them.
 */
const ADXL345_Platform_t *ADXL345_STM32WL_GetI2cPlatform(void);

#endif // HAL_I2C_MODULE_ENABLED

#endif // ADXL345_STM32WL_H
