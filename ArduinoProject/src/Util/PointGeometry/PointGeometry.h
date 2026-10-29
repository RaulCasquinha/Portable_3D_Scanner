#pragma once

struct Point3D {
    float x;
    float y;
    float z;
};

// Converts a distance (mm) and orientation into a 3D Cartesian point.
Point3D toPoint3D(float distanceMm, float pitchRad, float yawRad);
