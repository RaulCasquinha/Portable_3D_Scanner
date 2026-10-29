#pragma once
#include <stdint.h>
#include <stddef.h>

class Data_TransportLayer {
public:
    void begin(uint32_t baudRate);
    void send(const uint8_t* data, size_t len);
};
