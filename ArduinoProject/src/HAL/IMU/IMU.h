#pragma once

struct Orientation {
    float pitch; // radians
    float roll;  // radians
    float yaw;   // radians
};

class HAL_IMU {
public:
    bool begin();
    bool read(Orientation& out);
private:
    unsigned long _lastUs;
    Orientation _orientation;
};
