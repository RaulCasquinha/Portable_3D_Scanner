/**
 * @file adxl355_stm32wl.h
 * @brief STM32 HAL bus interface for the ADXL355 driver (SPI and I2C).
 *
 * Provides the ADXL355_Platform_t the driver needs, built on the STM32 HAL:
 * blocking SPI read and write, SPI read with DMA, blocking I2C read and
 * write, and a blocking delay. The SPI part is built when
 * HAL_SPI_MODULE_ENABLED is defined, the I2C part when
 * HAL_I2C_MODULE_ENABLED is defined.
 *
 * The driver callbacks carry no context, so the module keeps one SPI and one
 * I2C configuration. With more than one sensor on a bus, call
 * ADXL355_STM32WL_ConfigSpi() or ADXL355_STM32WL_ConfigI2c() again before
 * switching to another sensor.
 *
 * This is a platform module: it includes the STM32WL HAL header. For another
 * STM32 family, change that include only.
 *
 * @author Carlos Mendes
 * @date Oct/2026
 */

#ifndef ADXL355_STM32WL_H
#define ADXL355_STM32WL_H

#include <stdbool.h>
#include <stdint.h>

#include "adxl355.h"
#include "stm32wlxx_hal.h"

/** Timeout of one blocking SPI transfer (ADXL355_FIFO_MAX_BYTES take
 *  0.3 ms at 8 MHz and 2.4 ms at 1 MHz) */
#define ADXL355_STM32WL_SPI_TIMEOUT_MS     10U

/** Longest wait for a running DMA read to end before a new transfer */
#define ADXL355_STM32WL_DMA_WAIT_MS        2U

/** Fixed part of the timeout of one blocking I2C transfer */
#define ADXL355_STM32WL_I2C_BASE_TIMEOUT_MS 10U

/** Bytes per millisecond at 100 kHz (9 clocks per byte), used to extend the
 *  I2C timeout with the transfer size */
#define ADXL355_STM32WL_I2C_BYTES_PER_MS   11U

/**
 * @brief Status returned by the configuration functions.
 *
 * The bus functions return ADXL355_Status_t, as the driver requires.
 */
typedef enum {
    ADXL355_STM32WL_OK = 0,               /**< Success */
    ADXL355_STM32WL_INVALID_PARAMETER = 1 /**< Invalid function parameter */
} ADXL355_STM32WL_Status_t;

/**
 * @brief Blocking delay (ADXL355_Platform_t.delay_ms).
 *
 * @param[in] delay_ms Delay in milliseconds (HAL_Delay() adds up to 1 ms).
 *
 * @note Not for interrupt context (needs the SysTick interrupt).
 */
void ADXL355_STM32WL_DelayMs(uint32_t delay_ms);

#ifdef HAL_SPI_MODULE_ENABLED

/**
 * @brief Sets the SPI bus and chip select of the ADXL355 and drives nCS
 *        high.
 *
 * @param[in] hspi SPI handle (already initialised: master, 8-bit, mode 0,
 *            at most 10 MHz). Must not be NULL.
 * @param[in] cs_port GPIO port of nCS (output, already initialised). Must
 *            not be NULL.
 * @param[in] cs_pin GPIO pin of nCS (one GPIO_PIN_x value).
 *
 * @retval ADXL355_STM32WL_OK Success.
 * @retval ADXL355_STM32WL_INVALID_PARAMETER A pointer is NULL or cs_pin is
 *         0.
 *
 * @note Call before any SPI transfer, and again to switch to another sensor
 *       (only while no DMA read is running).
 */
ADXL355_STM32WL_Status_t ADXL355_STM32WL_ConfigSpi(SPI_HandleTypeDef *hspi,
                                                   GPIO_TypeDef *cs_port,
                                                   uint16_t cs_pin);

/**
 * @brief Reads data_size bytes from the ADXL355 over SPI, blocking
 *        (ADXL355_Platform_t.read).
 *
 * Waits up to ADXL355_STM32WL_DMA_WAIT_MS for a running DMA read to end,
 * then sends the command byte and receives the data with nCS held low.
 *
 * @param[in] address First register address (0x00 to 0x7F).
 * @param[out] buffer Data read. Must not be NULL.
 * @param[in] data_size Bytes to read (1 to ADXL355_FIFO_MAX_BYTES).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL355_TIMEOUT A DMA read did not end in time, or the transfer
 *         did not end within ADXL355_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL355_SPI_SEND_ERROR The HAL failed to send the command byte.
 * @retval ADXL355_SPI_RECEIVE_ERROR The HAL failed to receive the data.
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL355_Status_t ADXL355_STM32WL_ReadSpi(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size);

/**
 * @brief Starts a DMA read of data_size bytes from the ADXL355 over SPI.
 *
 * Not blocking, so it must not be used as ADXL355_Platform_t.read:
 * it is meant for FIFO reads started by the application.
 *
 * Sends the command byte (blocking), then starts the DMA reception and
 * returns; nCS stays low until ADXL355_STM32WL_SpiRxCpltHandler() is
 * called. Use ADXL355_STM32WL_IsSpiDmaBusy() to know when buffer holds the
 * data.
 *
 * @param[in] address First register address (0x00 to 0x7F).
 * @param[out] buffer Data read. Must not be NULL and must stay valid until
 *             the read ends.
 * @param[in] data_size Bytes to read (1 to ADXL355_FIFO_MAX_BYTES).
 *
 * @retval ADXL355_OK The DMA read started.
 * @retval ADXL355_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL355_TIMEOUT A previous DMA read did not end in time, or the
 *         command byte was not sent within ADXL355_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL355_SPI_SEND_ERROR The HAL failed to send the command byte.
 * @retval ADXL355_SPI_RECEIVE_ERROR The HAL refused to start the DMA.
 *
 * @note The SPI handle needs a DMA channel for RX and one for TX: in
 *       full-duplex master mode the HAL sends dummy bytes with the TX DMA.
 * @note Not reentrant, not for interrupt context.
 */
ADXL355_Status_t ADXL355_STM32WL_ReadSpiDma(const uint8_t address,
                                            uint8_t *buffer,
                                            const uint16_t data_size);

/**
 * @brief Writes data_size bytes to the ADXL355 over SPI, blocking
 *        (ADXL355_Platform_t.write).
 *
 * Waits up to ADXL355_STM32WL_DMA_WAIT_MS for a running DMA read to end,
 * then sends the command byte and the data with nCS held low.
 *
 * @param[in] address First register address (0x00 to 0x7F).
 * @param[in] buffer Data to write. Must not be NULL.
 * @param[in] data_size Bytes to write (1 to ADXL355_FIFO_MAX_BYTES).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL355_TIMEOUT A DMA read did not end in time, or the transfer
 *         did not end within ADXL355_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL355_SPI_SEND_ERROR The HAL reported another error.
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL355_Status_t ADXL355_STM32WL_WriteSpi(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size);

/**
 * @brief Tells whether a DMA read started by ADXL355_STM32WL_ReadSpiDma()
 *        is still running.
 *
 * @return true while the read runs; false once
 *         ADXL355_STM32WL_SpiRxCpltHandler() has ended it.
 */
bool ADXL355_STM32WL_IsSpiDmaBusy(void);

/**
 * @brief Ends a DMA read: drives nCS high and clears the busy flag.
 *
 * Call it from the application's HAL_SPI_RxCpltCallback() and
 * HAL_SPI_ErrorCallback(). The module does not define these HAL callbacks,
 * so other SPI devices of the application can use them too. Calls for
 * another SPI handle, or when no DMA read runs, are ignored.
 *
 * @param[in] hspi SPI handle passed to the HAL callback.
 *
 * @note Interrupt context: short, no blocking calls (EMB-03).
 */
void ADXL355_STM32WL_SpiRxCpltHandler(const SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the platform services for the ADXL355 driver on SPI.
 *
 * @return Pointer to static platform services with ReadSpi, WriteSpi
 *         and DelayMs (never NULL).
 *
 * @note Call ADXL355_STM32WL_ConfigSpi() before using them.
 */
const ADXL355_Platform_t *ADXL355_STM32WL_GetSpiPlatform(void);

#endif // HAL_SPI_MODULE_ENABLED

#ifdef HAL_I2C_MODULE_ENABLED

/**
 * @brief Sets the I2C bus and address of the ADXL355.
 *
 * @param[in] hi2c I2C handle (already initialised). Must not be NULL.
 * @param[in] address_7bit 7-bit device address (0x1D with ASEL low, 0x53
 *            with ASEL high).
 *
 * @retval ADXL355_STM32WL_OK Success.
 * @retval ADXL355_STM32WL_INVALID_PARAMETER hi2c is NULL or address_7bit
 *         is above 0x7F.
 *
 * @note Call before any I2C transfer, and again to switch to another sensor.
 */
ADXL355_STM32WL_Status_t ADXL355_STM32WL_ConfigI2c(I2C_HandleTypeDef *hi2c,
                                                   uint8_t address_7bit);

/**
 * @brief Reads data_size bytes from the ADXL355 over I2C, blocking
 *        (ADXL355_Platform_t.read).
 *
 * @param[in] address First register address.
 * @param[out] buffer Data read. Must not be NULL.
 * @param[in] data_size Bytes to read (1 to ADXL355_FIFO_MAX_BYTES).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL355_TIMEOUT The transfer did not end in time (10 ms plus the
 *         transfer time at 100 kHz).
 * @retval ADXL355_SPI_RECEIVE_ERROR The HAL reported another error (bus
 *         receive failed; the driver has no I2C-specific status).
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL355_Status_t ADXL355_STM32WL_ReadI2c(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size);

/**
 * @brief Writes data_size bytes to the ADXL355 over I2C, blocking
 *        (ADXL355_Platform_t.write).
 *
 * @param[in] address First register address.
 * @param[in] buffer Data to write. Must not be NULL.
 * @param[in] data_size Bytes to write (1 to ADXL355_FIFO_MAX_BYTES).
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_INVALID_PARAMETER Not configured, buffer is NULL or
 *         data_size is out of range.
 * @retval ADXL355_TIMEOUT The transfer did not end in time (10 ms plus the
 *         transfer time at 100 kHz).
 * @retval ADXL355_SPI_SEND_ERROR The HAL reported another error (bus send
 *         failed; the driver has no I2C-specific status).
 *
 * @note Not reentrant, not for interrupt context.
 */
ADXL355_Status_t ADXL355_STM32WL_WriteI2c(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size);

/**
 * @brief Gets the platform services for the ADXL355 driver on I2C.
 *
 * @return Pointer to static platform services with ReadI2c, WriteI2c and
 *         DelayMs (never NULL).
 *
 * @note Call ADXL355_STM32WL_ConfigI2c() before using them.
 */
const ADXL355_Platform_t *ADXL355_STM32WL_GetI2cPlatform(void);

#endif // HAL_I2C_MODULE_ENABLED

#endif // ADXL355_STM32WL_H
