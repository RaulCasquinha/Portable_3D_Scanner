/**
 * @file PointGeometry.cpp
 * @brief Implementation of the spherical-to-Cartesian conversion.
 */

#include "PointGeometry.h"
#include <math.h>

Point3D toPoint3D(float distanceMm, float pitchRad, float yawRad) {
    return {
        distanceMm * cosf(pitchRad) * cosf(yawRad),
        distanceMm * cosf(pitchRad) * sinf(yawRad),
        distanceMm * sinf(pitchRad)
    };
}
