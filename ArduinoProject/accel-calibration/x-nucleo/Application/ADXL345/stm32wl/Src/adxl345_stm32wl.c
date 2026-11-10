/**
 * @file adxl345_stm32wl.c
 * @brief Implementation of the STM32 HAL bus interface for the ADXL345
 *        driver.
 *
 * @author Raul Casquinha
 * @date Oct/2026
 */

#include "adxl345_stm32wl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "adxl345.h"
#include "stm32wlxx_hal.h"

#define SPI_COMMAND_BYTES       1U  // [R/W][MB][A5..A0]
#define I2C_ADDRESS_SHIFT       1U  // The HAL expects the 7-bit address << 1

#if defined(HAL_SPI_MODULE_ENABLED) || defined(HAL_I2C_MODULE_ENABLED)
static bool IsValidSize(uint16_t data_size);
#endif

void ADXL345_STM32WL_DelayMs(uint32_t delay_ms) {

    HAL_Delay(delay_ms);
}

// SPI

#ifdef HAL_SPI_MODULE_ENABLED

/**
 * @brief SPI bus and chip select of the ADXL345.
 */
typedef struct {
    SPI_HandleTypeDef *hspi; /**< SPI handle, NULL until configured */
    GPIO_TypeDef *cs_port;   /**< GPIO port of CS */
    uint16_t cs_pin;         /**< GPIO pin of CS */
} ADXL345_STM32WL_SpiConfig_t;

static const ADXL345_Platform_t ADXL345_STM32WL_SpiPlatform = {
    .read = ADXL345_STM32WL_ReadSpi,
    .write = ADXL345_STM32WL_WriteSpi,
    .delay_ms = ADXL345_STM32WL_DelayMs
};

static ADXL345_STM32WL_SpiConfig_t ADXL345_STM32WL_Spi = {NULL, NULL, 0U};

static ADXL345_Status_t CheckSpiTransfer(uint8_t address,
                                         const uint8_t *buffer,
                                         uint16_t data_size);
static void SelectDevice(void);
static void DeselectDevice(void);
static ADXL345_Status_t SendCommand(uint8_t address, bool is_read,
                                    uint16_t data_size);

ADXL345_STM32WL_Status_t ADXL345_STM32WL_ConfigSpi(SPI_HandleTypeDef *hspi,
                                                   GPIO_TypeDef *cs_port,
                                                   uint16_t cs_pin) {

    if ((hspi == NULL) || (cs_port == NULL) || (cs_pin == 0U)) {
        return ADXL345_STM32WL_INVALID_PARAMETER;
    }

    ADXL345_STM32WL_Spi.hspi = hspi;
    ADXL345_STM32WL_Spi.cs_port = cs_port;
    ADXL345_STM32WL_Spi.cs_pin = cs_pin;
    DeselectDevice();

    return ADXL345_STM32WL_OK;
}

ADXL345_Status_t ADXL345_STM32WL_ReadSpi(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size) {

    ADXL345_Status_t status = CheckSpiTransfer(address, buffer, data_size);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (status != ADXL345_OK) {
        return status;
    }

    SelectDevice();
    status = SendCommand(address, true, data_size);
    if (status == ADXL345_OK) {
        hal_status = HAL_SPI_Receive(ADXL345_STM32WL_Spi.hspi, buffer,
                                     data_size,
                                     ADXL345_STM32WL_SPI_TIMEOUT_MS);
        if (hal_status == HAL_TIMEOUT) {
            status = ADXL345_TIMEOUT;
        } else if (hal_status != HAL_OK) {
            status = ADXL345_SPI_RECEIVE_ERROR;
        } else {
            // Data received
        }
    }
    // CS released on every path, also after an error
    DeselectDevice();

    return status;
}

ADXL345_Status_t ADXL345_STM32WL_WriteSpi(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size) {

    ADXL345_Status_t status = CheckSpiTransfer(address, buffer, data_size);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (status != ADXL345_OK) {
        return status;
    }

    SelectDevice();
    status = SendCommand(address, false, data_size);
    if (status == ADXL345_OK) {
        hal_status = HAL_SPI_Transmit(ADXL345_STM32WL_Spi.hspi, buffer,
                                      data_size,
                                      ADXL345_STM32WL_SPI_TIMEOUT_MS);
        if (hal_status == HAL_TIMEOUT) {
            status = ADXL345_TIMEOUT;
        } else if (hal_status != HAL_OK) {
            status = ADXL345_SPI_SEND_ERROR;
        } else {
            // Data sent
        }
    }
    // CS released on every path, also after an error
    DeselectDevice();

    return status;
}

const ADXL345_Platform_t *ADXL345_STM32WL_GetSpiPlatform(void) {

    return &ADXL345_STM32WL_SpiPlatform;
}

/**
 * @brief Checks the parameters common to every SPI transfer.
 *
 * @param[in] address First register address.
 * @param[in] buffer Data buffer.
 * @param[in] data_size Bytes to transfer.
 *
 * @retval ADXL345_OK The transfer can start.
 * @retval ADXL345_INVALID_PARAMETER Not configured, address above 0x3F,
 *         buffer is NULL or data_size is out of range.
 */
static ADXL345_Status_t CheckSpiTransfer(uint8_t address,
                                         const uint8_t *buffer,
                                         uint16_t data_size) {

    if ((ADXL345_STM32WL_Spi.hspi == NULL)
        || (address > ADXL345_SPI_ADDR_MASK) || (buffer == NULL)
        || !IsValidSize(data_size)) {
        return ADXL345_INVALID_PARAMETER;
    }

    return ADXL345_OK;
}

/**
 * @brief Sends the command byte: [R/W][MB][A5..A0].
 *
 * @param[in] address Register address (0x00 to 0x3F).
 * @param[in] is_read true for a read, false for a write.
 * @param[in] data_size Bytes of the transfer; MB is set when above 1.
 *
 * @retval ADXL345_OK Success.
 * @retval ADXL345_TIMEOUT Not sent within ADXL345_STM32WL_SPI_TIMEOUT_MS.
 * @retval ADXL345_SPI_SEND_ERROR The HAL reported another error.
 *
 * @note CS must already be low.
 */
static ADXL345_Status_t SendCommand(uint8_t address, bool is_read,
                                    uint16_t data_size) {

    uint8_t command = (uint8_t)(address & ADXL345_SPI_ADDR_MASK);
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (is_read) {
        command = (uint8_t)(command | ADXL345_SPI_READ_BIT);
    }
    if (data_size > 1U) {
        command = (uint8_t)(command | ADXL345_SPI_MB_BIT);
    }

    hal_status = HAL_SPI_Transmit(ADXL345_STM32WL_Spi.hspi, &command,
                                  SPI_COMMAND_BYTES,
                                  ADXL345_STM32WL_SPI_TIMEOUT_MS);
    if (hal_status == HAL_TIMEOUT) {
        return ADXL345_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL345_SPI_SEND_ERROR;
    }

    return ADXL345_OK;
}

/**
 * @brief Drives CS low (selects the ADXL345).
 */
static void SelectDevice(void) {

    HAL_GPIO_WritePin(ADXL345_STM32WL_Spi.cs_port,
                      ADXL345_STM32WL_Spi.cs_pin, GPIO_PIN_RESET);
}

/**
 * @brief Drives CS high (releases the ADXL345).
 */
static void DeselectDevice(void) {

    HAL_GPIO_WritePin(ADXL345_STM32WL_Spi.cs_port,
                      ADXL345_STM32WL_Spi.cs_pin, GPIO_PIN_SET);
}

#endif // HAL_SPI_MODULE_ENABLED

// I2C

#ifdef HAL_I2C_MODULE_ENABLED

/**
 * @brief I2C bus and address of the ADXL345.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c; /**< I2C handle, NULL until configured */
    uint16_t address;        /**< 7-bit address shifted left, as the HAL
                                  expects it */
} ADXL345_STM32WL_I2cConfig_t;

static const ADXL345_Platform_t ADXL345_STM32WL_I2cPlatform = {
    .read = ADXL345_STM32WL_ReadI2c,
    .write = ADXL345_STM32WL_WriteI2c,
    .delay_ms = ADXL345_STM32WL_DelayMs
};

static ADXL345_STM32WL_I2cConfig_t ADXL345_STM32WL_I2c = {NULL, 0U};

static uint32_t GetI2cTimeoutMs(uint16_t data_size);

ADXL345_STM32WL_Status_t ADXL345_STM32WL_ConfigI2c(I2C_HandleTypeDef *hi2c,
                                                   uint8_t address_7bit) {

    if ((hi2c == NULL)
        || ((address_7bit != ADXL345_I2C_ADDRESS_ALT_LOW)
            && (address_7bit != ADXL345_I2C_ADDRESS_ALT_HIGH))) {
        return ADXL345_STM32WL_INVALID_PARAMETER;
    }

    ADXL345_STM32WL_I2c.hi2c = hi2c;
    ADXL345_STM32WL_I2c.address = (uint16_t)((uint32_t)address_7bit
                                             << I2C_ADDRESS_SHIFT);

    return ADXL345_STM32WL_OK;
}

ADXL345_Status_t ADXL345_STM32WL_ReadI2c(const uint8_t address,
                                         uint8_t *buffer,
                                         const uint16_t data_size) {

    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((ADXL345_STM32WL_I2c.hi2c == NULL) || (buffer == NULL)
        || !IsValidSize(data_size)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Register address, repeated start, then read; the pointer auto increments
    hal_status = HAL_I2C_Mem_Read(ADXL345_STM32WL_I2c.hi2c,
                                  ADXL345_STM32WL_I2c.address, address,
                                  I2C_MEMADD_SIZE_8BIT, buffer, data_size,
                                  GetI2cTimeoutMs(data_size));
    if (hal_status == HAL_TIMEOUT) {
        return ADXL345_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL345_SPI_RECEIVE_ERROR;
    }

    return ADXL345_OK;
}

ADXL345_Status_t ADXL345_STM32WL_WriteI2c(const uint8_t address,
                                          const uint8_t *buffer,
                                          const uint16_t data_size) {

    uint8_t tx_buffer[ADXL345_STM32WL_MAX_TRANSFER_BYTES] = {0U};
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((ADXL345_STM32WL_I2c.hi2c == NULL) || (buffer == NULL)
        || !IsValidSize(data_size)) {
        return ADXL345_INVALID_PARAMETER;
    }

    // Copy instead of casting away const: the HAL takes a non-const pointer
    (void)memcpy(tx_buffer, buffer, data_size);

    hal_status = HAL_I2C_Mem_Write(ADXL345_STM32WL_I2c.hi2c,
                                   ADXL345_STM32WL_I2c.address, address,
                                   I2C_MEMADD_SIZE_8BIT, tx_buffer, data_size,
                                   GetI2cTimeoutMs(data_size));
    if (hal_status == HAL_TIMEOUT) {
        return ADXL345_TIMEOUT;
    }
    if (hal_status != HAL_OK) {
        return ADXL345_SPI_SEND_ERROR;
    }

    return ADXL345_OK;
}

const ADXL345_Platform_t *ADXL345_STM32WL_GetI2cPlatform(void) {

    return &ADXL345_STM32WL_I2cPlatform;
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

    return ADXL345_STM32WL_I2C_BASE_TIMEOUT_MS +
           ((uint32_t)data_size / ADXL345_STM32WL_I2C_BYTES_PER_MS);
}

#endif // HAL_I2C_MODULE_ENABLED

#if defined(HAL_SPI_MODULE_ENABLED) || defined(HAL_I2C_MODULE_ENABLED)
/**
 * @brief Checks the size of one transfer.
 *
 * @param[in] data_size Bytes to transfer.
 *
 * @return true if data_size is 1 to ADXL345_STM32WL_MAX_TRANSFER_BYTES.
 */
static bool IsValidSize(uint16_t data_size) {

    return (data_size > 0U)
           && (data_size <= ADXL345_STM32WL_MAX_TRANSFER_BYTES);
}
#endif
