/**
 * @file PointGeometry.cpp
 * @brief Implementation of toWorldPoint().
 */

#include "PointGeometry.h"
#include <math.h>

Point3D toWorldPoint(float distanceMm, const Position& devicePos, const Orientation& orient) {
    // Direction the sensor is pointing in world space (spherical → Cartesian).
    // Roll is omitted: it rotates around the pointing axis and does not change
    // where a 1-D distance sensor aims.
    float dir_x = cosf(orient.pitch) * cosf(orient.yaw);
    float dir_y = cosf(orient.pitch) * sinf(orient.yaw);
    float dir_z = sinf(orient.pitch);

    return {
        devicePos.x + distanceMm * dir_x,
        devicePos.y + distanceMm * dir_y,
        devicePos.z + distanceMm * dir_z
    };
}
