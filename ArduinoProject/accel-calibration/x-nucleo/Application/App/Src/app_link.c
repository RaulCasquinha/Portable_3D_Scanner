/**
 * @file app_link.c
 * @brief Source file for the app_link.h header file.
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#include "app_link.h"

#include <stddef.h>
#include <stdint.h>

#include "main.h"

// 128 bytes take 11 ms at 115200 bit/s: this leaves a wide margin
#define SEND_TIMEOUT_MS     100U
#define NO_WAIT_MS          0U

APP_LINK_Status_t APP_LINK_Init(void) {
    if (HAL_UARTEx_EnableFifoMode(&hcom_uart[COM1]) != HAL_OK) {
        return APP_LINK_ERROR;
    }
    return APP_LINK_OK;
}

APP_LINK_Status_t APP_LINK_Send(const char data[], size_t size_bytes) {
    if ((data == NULL) || (size_bytes == 0U) || (size_bytes > UINT16_MAX)) {
        return APP_LINK_INVALID_PARAMETER;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(&hcom_uart[COM1],
                                                 (const uint8_t *)data,
                                                 (uint16_t)size_bytes,
                                                 SEND_TIMEOUT_MS);
    if (status == HAL_TIMEOUT) {
        return APP_LINK_TIMEOUT;
    }
    return (status == HAL_OK) ? APP_LINK_OK : APP_LINK_ERROR;
}

APP_LINK_Status_t APP_LINK_ReceiveByte(uint8_t *byte) {
    if (byte == NULL) {
        return APP_LINK_INVALID_PARAMETER;
    }
    // An overrun (bytes lost while the board was busy) blocks the receiver
    // until it is cleared; the lost bytes can't be recovered
    __HAL_UART_CLEAR_OREFLAG(&hcom_uart[COM1]);
    if (HAL_UART_Receive(&hcom_uart[COM1], byte, 1U, NO_WAIT_MS) != HAL_OK) {
        return APP_LINK_NO_DATA;
    }
    return APP_LINK_OK;
}

/*
 * Polls the receive flag instead of calling HAL_UART_Receive() per byte: at
 * 4 MHz that call is slower than the bytes of a command line arrive, and it
 * fails on the resulting overrun.
 */
APP_LINK_Status_t APP_LINK_WaitForByte(uint8_t *byte, uint32_t timeout_ms) {
    uint32_t start_ms = HAL_GetTick();

    if (byte == NULL) {
        return APP_LINK_INVALID_PARAMETER;
    }
    while (__HAL_UART_GET_FLAG(&hcom_uart[COM1], UART_FLAG_RXNE) == RESET) {
        if ((HAL_GetTick() - start_ms) >= timeout_ms) {
            return APP_LINK_TIMEOUT;
        }
    }
    *byte = (uint8_t)READ_REG(hcom_uart[COM1].Instance->RDR);
    return APP_LINK_OK;
}
