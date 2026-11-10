/**
 * @file app_link.h
 * @brief Transport: bytes to and from the PC over COM1 (the ST-LINK virtual
 *        COM port, 115200 8N1), with timeouts.
 *
 * Layer: transport. It moves bytes and knows nothing of what they mean; the
 * lines are built and read by app_protocol.h. Unlike LOG() (app_log.h), it is
 * kept in Release builds.
 *
 * COM1 is polled, not read by interrupt: a byte that arrives while the board
 * is busy waits in the UART FIFO (enabled by APP_LINK_Init()).
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#ifndef APP_LINK_H
#define APP_LINK_H

#include <stddef.h>
#include <stdint.h>

/** Result of a link function */
typedef enum {
    APP_LINK_OK = 0,                    /**< Done */
    APP_LINK_INVALID_PARAMETER = 1,     /**< NULL pointer or bad size */
    APP_LINK_NO_DATA = 2,               /**< No byte received */
    APP_LINK_TIMEOUT = 3,               /**< Not done in time */
    APP_LINK_ERROR = 4                  /**< The UART failed */
} APP_LINK_Status_t;

/**
 * @brief Prepares COM1 for polling: enables its FIFO.
 *
 * @pre COM1 is initialised (BSP_COM_Init() in main.c).
 *
 * @retval APP_LINK_OK    Ready.
 * @retval APP_LINK_ERROR The FIFO could not be enabled; single bytes still
 *                        work, but a fast command line may lose bytes.
 */
APP_LINK_Status_t APP_LINK_Init(void);

/**
 * @brief Sends bytes. Blocking until they are sent, at most 100 ms.
 *
 * @param[in] data The bytes. Must not be NULL.
 * @param[in] size_bytes How many, 1 to UINT16_MAX.
 *
 * @retval APP_LINK_OK                All sent.
 * @retval APP_LINK_INVALID_PARAMETER NULL pointer or size out of range.
 * @retval APP_LINK_TIMEOUT           Not all sent in time.
 * @retval APP_LINK_ERROR             The UART failed.
 */
APP_LINK_Status_t APP_LINK_Send(const char data[], size_t size_bytes);

/**
 * @brief Takes one received byte, if there is one. Non-blocking.
 *
 * @param[out] byte The byte. Must not be NULL.
 *
 * @retval APP_LINK_OK                A byte was received.
 * @retval APP_LINK_NO_DATA           None was waiting.
 * @retval APP_LINK_INVALID_PARAMETER byte is NULL.
 */
APP_LINK_Status_t APP_LINK_ReceiveByte(uint8_t *byte);

/**
 * @brief Waits for one byte, at most timeout_ms.
 *
 * @param[out] byte The byte. Must not be NULL.
 * @param[in] timeout_ms Longest wait (ms).
 *
 * @retval APP_LINK_OK                A byte was received.
 * @retval APP_LINK_TIMEOUT           None arrived in time.
 * @retval APP_LINK_INVALID_PARAMETER byte is NULL.
 */
APP_LINK_Status_t APP_LINK_WaitForByte(uint8_t *byte, uint32_t timeout_ms);

#endif // APP_LINK_H
