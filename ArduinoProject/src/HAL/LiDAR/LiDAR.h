#pragma once
#include <stdint.h>

class HAL_LiDAR {
public:
    bool begin();
    int16_t readDistance(); // distance in mm; -1 on timeout/error
};
