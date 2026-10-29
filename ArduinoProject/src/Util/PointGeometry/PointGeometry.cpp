#include "PointGeometry.h"
#include <math.h>

// Spherical to Cartesian: distance + pitch/yaw -> (x, y, z).
Point3D toPoint3D(float distanceMm, float pitchRad, float yawRad) {
    return {
        distanceMm * cosf(pitchRad) * cosf(yawRad),
        distanceMm * cosf(pitchRad) * sinf(yawRad),
        distanceMm * sinf(pitchRad)
    };
}
