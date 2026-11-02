#include <Wire.h>
#include "src/HAL/LiDAR/LiDAR.h"
#include "src/HAL/IMU/IMU.h"
#include "src/Util/PointGeometry/PointGeometry.h"
#include "src/Data/TransportLayer/TransportLayer.h"
#include "src/Data/AppLayer/AppLayer.h"

static HAL_LiDAR lidar;
static HAL_IMU   imu;
static Data_TransportLayer transport;
static Data_AppLayer       appLayer(transport);

static bool _lidarOk = false;
static bool _imuOk   = false;

// --- Accumulator for position-aware averaging ----------------------------
// Collect up to ACC_N valid distance readings.  If the IMU detects the
// device rotated more than ORI_THRESH radians on any axis before the
// buffer fills, the partial average is flushed immediately (device moved).
static const int   ACC_N       = 5;
static const float ORI_THRESH  = 0.017f;  // ~1 degree
static const float POS_THRESH  = 2.0f;    // mm

static struct {
    float       sumDist;
    int         count;
    Orientation orient;
    Position    pos;
} _acc = {0.0f, 0, {0,0,0}, {0,0,0}};

static void _accFlush() {
    if (_acc.count == 0) return;
    float avgDist = _acc.sumDist / _acc.count;
    Point3D point = toWorldPoint(avgDist, _acc.pos, _acc.orient);
    appLayer.sendPoint(point, _acc.pos);
    _acc.sumDist = 0.0f;
    _acc.count   = 0;
}

static bool _orientMoved(const Orientation& a, const Orientation& b) {
    return fabsf(a.pitch - b.pitch) > ORI_THRESH
        || fabsf(a.roll  - b.roll)  > ORI_THRESH
        || fabsf(a.yaw   - b.yaw)   > ORI_THRESH;
}

static bool _posMoved(const Position& a, const Position& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return (dx*dx + dy*dy + dz*dz) > (POS_THRESH * POS_THRESH);
}
// -------------------------------------------------------------------------

static void _sendBootStatus() {
    appLayer.sendLog("[INFO] PortableScanner3D ready");
    appLayer.sendLog(_imuOk   ? "[INFO] IMU OK"   : "[WARN] IMU not found - fixed zero pose active");
    appLayer.sendLog(_lidarOk ? "[INFO] LiDAR OK" : "[WARN] LiDAR not found - point capture disabled");
    if      (_lidarOk && _imuOk) appLayer.sendLog("[INFO] All sensors ready - full scan mode");
    else if (_lidarOk)           appLayer.sendLog("[WARN] LiDAR only - points at fixed origin");
    else if (_imuOk)             appLayer.sendLog("[WARN] IMU only - pose tracking, no point capture");
    else                         appLayer.sendLog("[ERROR] No sensors found - check wiring");
}

void setup() {
    transport.begin(115200);

    Wire.begin();
    appLayer.sendLog("[INFO] Initializing IMU...");
    _imuOk = imu.begin();

    Wire.begin();
    appLayer.sendLog("[INFO] Initializing LiDAR...");
    _lidarOk = lidar.begin();

    if (_imuOk) imu.setReference();

    unsigned long t0 = millis();
    while (!Serial && (millis() - t0) < 3000) delay(10);
    _sendBootStatus();
}

void loop() {
    static bool _prevSerial = false;
    bool nowSerial = (bool)Serial;
    if (nowSerial && !_prevSerial) _sendBootStatus();
    _prevSerial = nowSerial;

    Orientation orient = {0.0f, 0.0f, 0.0f};
    Position    pos    = {0.0f, 0.0f, 0.0f};

    if (_imuOk) {
        if (!imu.read(orient, pos)) return;
        appLayer.sendPose(pos, orient);
    }

    if (_lidarOk) {
        int16_t dist = lidar.readDistance();
        if (dist <= 0) return;

        // If IMU is available and the device has rotated since we started
        // accumulating, flush the partial average before adding this sample.
        if (_imuOk && _acc.count > 0 && (_orientMoved(_acc.orient, orient) || _posMoved(_acc.pos, pos))) {
            _accFlush();
        }

        // Start a new accumulation window at the current pose.
        if (_acc.count == 0) {
            _acc.orient = orient;
            _acc.pos    = pos;
        }

        _acc.sumDist += (float)dist;
        _acc.count++;

        if (_acc.count >= ACC_N) {
            _accFlush();
        }
    }
}
