#include "src/HAL/LiDAR/LiDAR.h"
#include "src/HAL/IMU/IMU.h"
#include "src/Util/PointGeometry/PointGeometry.h"
#include "src/Data/TransportLayer/TransportLayer.h"
#include "src/Data/AppLayer/AppLayer.h"

static HAL_LiDAR lidar;
static HAL_IMU imu;
static Data_TransportLayer transport;
static Data_AppLayer appLayer(transport);

void setup() {
    transport.begin(115200);

    if (!lidar.begin()) {
        Serial.println("LiDAR init failed");
        while (1);
    }
    if (!imu.begin()) {
        Serial.println("IMU init failed");
        while (1);
    }
}

void loop() {
    int16_t dist = lidar.readDistance();
    if (dist < 0) return;

    Orientation orient;
    if (!imu.read(orient)) return;

    Point3D point = toPoint3D((float)dist, orient.pitch, orient.yaw);
    appLayer.sendPoint(point);
}
