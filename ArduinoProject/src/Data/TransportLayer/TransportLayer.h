/**
 * @file TransportLayer.h
 * @brief Lowest-level data layer: raw byte transmission over Arduino Serial.
 *
 * Wraps @c Serial so the rest of the data stack is independent of the
 * concrete serial backend. Swapping this class for a BLE or Wi-Fi
 * implementation requires no changes above this layer.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @class Data_TransportLayer
 * @brief Sends raw bytes over the hardware serial port.
 */
class Data_TransportLayer {
public:
    /**
     * @brief Initialises the serial port at the requested baud rate.
     *
     * Must be called once in @c setup() before any call to @c send().
     *
     * @param baudRate Desired baud rate (e.g. 115200).
     */
    void begin(uint32_t baudRate);

    /**
     * @brief Transmits a block of bytes over the serial port.
     *
     * Blocks until all bytes have been written to the hardware FIFO.
     *
     * @param data Pointer to the byte buffer to transmit.
     * @param len  Number of bytes to transmit.
     */
    void send(const uint8_t* data, size_t len);
};
