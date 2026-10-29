/**
 * @file TransportLayer.cpp
 * @brief Data_TransportLayer implementation backed by Arduino Serial.
 */

#include "TransportLayer.h"
#include <Arduino.h>

void Data_TransportLayer::begin(uint32_t baudRate) {
    Serial.begin(baudRate);
}

void Data_TransportLayer::send(const uint8_t* data, size_t len) {
    Serial.write(data, len);
}
