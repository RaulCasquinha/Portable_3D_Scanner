/**
 * @file adxl355_stm32wl.c
 * @brief Implementation of the STM32 HAL bus interface for the ADXL355
 *        driver.
 *
 * @author Carlos Mendes
 * @date Oct/2026
 */

#include "adxl355_stm32wl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adxl355.h"
#include "stm32wlxx_hal.h"

#define SPI_COMMAND_BYTES       1U    // Address and read/write bit
#define SPI_MAX_ADDRESS         0x7FU // 7-bit register address

#if defined(HAL_SPI_MODULE_ENABLED) || defined(HAL_I2C_MODULE_ENABLED)
static bool IsValidSize(uint16_t data_size);
#endif

void ADXL355_STM32WL_DelayMs(uint32_t delay_ms) {

    HAL_Delay(delay_ms);
}

/* ============================== SPI ============================== */

#ifdef HAL_SPI_MODULE_ENABLED

/**
 * @brief SPI bus and chip select of the ADXL355.
 */
typedef struct {
    SPI_HandleTypeDef *hspi; /**< SPI handle, NULL until configured */
    GPIO_TypeDef *cs_port;   /**< GPIO port of nCS */
    uint16_t cs_pin;         /**< GPIO pin of nCS */
} ADXL355_STM32WL_SpiConfig_t;

static const ADXL355_Platform_t ADXL355_STM32WL_SpiPlatform = {
    .read = ADXL355_STM32WL_ReadSpi,
    .write = ADXL355_STM32WL_WriteSpi,
    .delay_ms = ADXL355_STM32WL_DelayMs
};

/* Written only by ADXL355_STM32WL_ConfigSpi(), which must not be called
 * while a DMA read runs; the interrupt handler only reads it. */
static ADXL355_STM32WL_SpiConfig_t ADXL355_STM32WL_Spi = {NULL, NULL, 0U};
/* Set by the main loop before the DMA starts, cleared by the interrupt
 * handler when it ends: a bool write is a single store, so no critical
 * section is needed (EMB-02). */
static volatile bool ADXL355_STM32WL_IsDmaReadRunning = false;

static ADXL355_Status_t CheckSpiTransfer(uint8_t address,
                                         const uint8_t *buffer,
                                         uint16_t data_size);
static bool WaitForDmaIdle(void);
static void SelectDevice(void);
static void DeselectDevice(void);
static ADXL355_Status_t SendCommand(uint8_t address, bool is_read);

ADXL355_STM32WL_Status_t ADXL355_STM32WL_ConfigSpi(SPI_HandleTypeDef *hspi,
                                                   GPIO_TypeDef *cs_port,
                                                   uint16_t cs_pin) {

    if ((hspi == NULL) || (cs_port == NULL) || (cs_pin == 0U)) {
        return ADXL355_STM32WL_INVALID_PARAMETER;
    }

    ADXL355_STM32WL_Spi.hspi = hspi;
    ADXL355_STM32WL_Spi.cs_port = cs_port;
    ADXL355_STM32WL_Spi.cs_pin = cs_pin;
    DeselectDevice();

    return ADXL355_STM32WL_OK;
}

ADXL355_Status_t ADXL355_STM32WL_ReadSpi(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size) {

    ADXL355_Status_t status = CheckSpiTransfer(address, buffer, data_size);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (status != ADXL355_OK) {
        return status;
    }
    if (!WaitForDmaIdle()) {
        return ADXL355_TIMEOUT;
    }

    SelectDevice();
    status = SendCommand(address, true);
    if (status == ADXL355_OK) {
        hal_status = HAL_SPI_Receive(ADXL355_STM32WL_Spi.hspi, buffer,
                                     data_size,
                                     ADXL355_STM32WL_SPI_TIMEOUT_MS);
        if (hal_status == HAL_TIMEOUT) {
            status = ADXL355_TIMEOUT;
        } else if (hal_status != HAL_OK) {
            status = ADXL355_SPI_RECEIVE_ERROR;
        } else {
            // Data received
        }
    }
    DeselectDevice();

    return status;
}

ADXL355_Status_t ADXL355_STM32WL_ReadSpiDma(const uint8_t address,
                                            uint8_t *buffer,
                                            const uint16_t data_size) {

    ADXL355_Status_t status = CheckSpiTransfer(address, buffer, data_size);

    if (status != ADXL355_OK) {
        return status;
    }
    if (!WaitForDmaIdle()) {
        return ADXL355_TIMEOUT;
    }

    SelectDevice();
    status = SendCommand(address, true);
    if (status != ADXL355_OK) {
        DeselectDevice();
        return status;
    }

    // Set before the start: the end of the DMA may interrupt right after it
    ADXL355_STM32WL_IsDmaReadRunning = true;
    if (HAL_SPI_Receive_DMA(ADXL355_STM32WL_Spi.hspi, buffer, data_size) !=
        HAL_OK) {
        ADXL355_STM32WL_IsDmaReadRunning = false;
        DeselectDevice();
        return ADXL355_SPI_RECEIVE_ERROR;
    }

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_STM32WL_WriteSpi(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size) {

    ADXL355_Status_t status = CheckSpiTransfer(address, buffer, data_size);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (status != ADXL355_OK) {
        return status;
    }
    if (!WaitForDmaIdle()) {
        return ADXL355_TIMEOUT;
    }

    SelectDevice();
    status = SendCommand(address, false);
    if (status == ADXL355_OK) {
        hal_status = HAL_SPI_Transmit(ADXL355_STM32WL_Spi.hspi, buffer,
                                      data_size,
                                      ADXL355_STM32WL_SPI_TIMEOUT_MS);
        if (hal_status == HAL_TIMEOUT) {
            status = ADXL355_TIMEOUT;
        } else if (hal_status != HAL_OK) {
            status = ADXL355_SPI_SEND_ERROR;
        } else {
            // Data sent
        }
    }
    DeselectDevice();

    return status;
}

bool ADXL355_STM32WL_IsSpiDmaBusy(void) {

    return ADXL355_STM32WL_IsDmaReadRunning;
}

void ADXL355_STM32WL_SpiRxCpltHandler(const SPI_HandleTypeDef *hspi) {

    if ((hspi == NULL) || (hspi != ADXL355_STM32WL_Spi.hspi) ||
        !ADXL355_STM32WL_IsDmaReadRunning) {
        return;
    }

    DeselectDevice();
    ADXL355_STM32WL_IsDmaReadRunning = false;
}

const ADXL355_Platform_t *ADXL355_STM32WL_GetSpiPlatform(void) {

    return &ADXL355_STM32WL_SpiPlatform;
}

/**
 * @brief Checks the parameters common to every SPI transfer.
 *
 * @param[in] address First register address.
 * @param[in] buffer Data buffer.
 * @param[in] data_size Bytes to transfer.
 *
 * @retval ADXL355_OK The transfer can start.
 * @retval ADXL355_INVALID_PARAMETER Not configured, address above 0x7F,
 *         buffer is NULL or data_size is out of range.
 */
static ADXL355_Status_t CheckSpiTransfer(uint8_t address,
                                         const uint8_t *buffer,
                                         uint16_t data_size) {

    if ((ADXL355_STM32WL_Spi.hspi == NULL) || (address > SPI_MAX_ADDRESS) ||
        (buffer == NULL) || !IsValidSize(data_size)) {
        return ADXL355_INVALID_PARAMETER;
    }

    return ADXL355_OK;
}

/**
 * @brief Waits for a running DMA read to end.
 *
 * @return true if no DMA read runs; false if it did not end within
 *         ADXL355_STM32WL_DMA_WAIT_MS.
 */
static bool WaitForDmaIdle(void) {

    uint32_t start_ms = HAL_GetTick();

    while (ADXL355_STM32WL_IsDmaReadRunning) {
        if ((HAL_GetTick() - start_ms) >= ADXL355_STM32WL_DMA_WAIT_MS) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Sends the command byte (register address and read/write bit).
 *
 * @param[in] address Register address (0x00 to 0x7F).
 * @param[in] is_read true for a read, false for a write.
 *
 * @retval ADXL355_OK Success.
 * @retval ADXL355_TIMEOUT Not sent within ADXL355_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL355_SPI_SEND_ERROR The HAL reported another error.
 *
 * @note nCS must already be low.
 */
static ADXL355_Status_t SendCommand(uint8_t address, bool is_read) {

    uint8_t command = (uint8_t)((uint32_t)address << 1U);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (is_read) {
        command |= ADXL355_SPI_READ_BIT;
    }

    hal_status = HAL_SPI_Transmit(ADXL355_STM32WL_Spi.hspi, &command,
                                  SPI_COMMAND_BYTES,
                                  ADXL355_STM32WL_SPI_TIMEOUT_MS);
    if (hal_status == HAL_TIMEOUT) {
        return ADXL355_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL355_SPI_SEND_ERROR;
    }

    return ADXL355_OK;
}

/**
 * @brief Drives nCS low (selects the ADXL355).
 */
static void SelectDevice(void) {

    HAL_GPIO_WritePin(ADXL355_STM32WL_Spi.cs_port,
                      ADXL355_STM32WL_Spi.cs_pin, GPIO_PIN_RESET);
}

/**
 * @brief Drives nCS high (releases the ADXL355).
 *
 * @note Also called from interrupt context (end of a DMA read).
 */
static void DeselectDevice(void) {

    HAL_GPIO_WritePin(ADXL355_STM32WL_Spi.cs_port,
                      ADXL355_STM32WL_Spi.cs_pin, GPIO_PIN_SET);
}

#endif // HAL_SPI_MODULE_ENABLED

/* ============================== I2C ============================== */

#ifdef HAL_I2C_MODULE_ENABLED

#define I2C_MAX_ADDRESS_7BIT    0x7FU

/**
 * @brief I2C bus and address of the ADXL355.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c; /**< I2C handle, NULL until configured */
    uint16_t address;        /**< 7-bit address shifted left, as the HAL
                                  expects it */
} ADXL355_STM32WL_I2cConfig_t;

static const ADXL355_Platform_t ADXL355_STM32WL_I2cPlatform = {
    .read = ADXL355_STM32WL_ReadI2c,
    .write = ADXL355_STM32WL_WriteI2c,
    .delay_ms = ADXL355_STM32WL_DelayMs
};

static ADXL355_STM32WL_I2cConfig_t ADXL355_STM32WL_I2c = {NULL, 0U};

static uint32_t GetI2cTimeoutMs(uint16_t data_size);

ADXL355_STM32WL_Status_t ADXL355_STM32WL_ConfigI2c(I2C_HandleTypeDef *hi2c,
                                                   uint8_t address_7bit) {

    if ((hi2c == NULL) || (address_7bit > I2C_MAX_ADDRESS_7BIT)) {
        return ADXL355_STM32WL_INVALID_PARAMETER;
    }

    ADXL355_STM32WL_I2c.hi2c = hi2c;
    ADXL355_STM32WL_I2c.address = (uint16_t)((uint32_t)address_7bit << 1U);

    return ADXL355_STM32WL_OK;
}

ADXL355_Status_t ADXL355_STM32WL_ReadI2c(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size) {

    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((ADXL355_STM32WL_I2c.hi2c == NULL) || (buffer == NULL) ||
        !IsValidSize(data_size)) {
        return ADXL355_INVALID_PARAMETER;
    }

    hal_status = HAL_I2C_Mem_Read(ADXL355_STM32WL_I2c.hi2c,
                                  ADXL355_STM32WL_I2c.address, address,
                                  I2C_MEMADD_SIZE_8BIT, buffer, data_size,
                                  GetI2cTimeoutMs(data_size));
    if (hal_status == HAL_TIMEOUT) {
        return ADXL355_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL355_SPI_RECEIVE_ERROR;
    }

    return ADXL355_OK;
}

ADXL355_Status_t ADXL355_STM32WL_WriteI2c(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size) {

    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((ADXL355_STM32WL_I2c.hi2c == NULL) || (buffer == NULL) ||
        !IsValidSize(data_size)) {
        return ADXL355_INVALID_PARAMETER;
    }

    // The HAL prototype takes a non-const pointer but only reads the data
    hal_status = HAL_I2C_Mem_Write(ADXL355_STM32WL_I2c.hi2c,
                                   ADXL355_STM32WL_I2c.address, address,
                                   I2C_MEMADD_SIZE_8BIT, (uint8_t *)buffer,
                                   data_size, GetI2cTimeoutMs(data_size));
    if (hal_status == HAL_TIMEOUT) {
        return ADXL355_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL355_SPI_SEND_ERROR;
    }

    return ADXL355_OK;
}

const ADXL355_Platform_t *ADXL355_STM32WL_GetI2cPlatform(void) {

    return &ADXL355_STM32WL_I2cPlatform;
}

/**
 * @brief Timeout of one I2C transfer: a fixed part plus the transfer time at
 *        100 kHz.
 *
 * @param[in] data_size Bytes to transfer.
 *
 * @return Timeout in milliseconds.
 */
static uint32_t GetI2cTimeoutMs(uint16_t data_size) {

    return ADXL355_STM32WL_I2C_BASE_TIMEOUT_MS +
           ((uint32_t)data_size / ADXL355_STM32WL_I2C_BYTES_PER_MS);
}

#endif // HAL_I2C_MODULE_ENABLED

#if defined(HAL_SPI_MODULE_ENABLED) || defined(HAL_I2C_MODULE_ENABLED)
/**
 * @brief Checks the size of one transfer.
 *
 * @param[in] data_size Bytes to transfer.
 *
 * @return true if data_size is 1 to ADXL355_FIFO_MAX_BYTES.
 */
static bool IsValidSize(uint16_t data_size) {

    return (data_size > 0U) && (data_size <= ADXL355_FIFO_MAX_BYTES);
}
#endif
