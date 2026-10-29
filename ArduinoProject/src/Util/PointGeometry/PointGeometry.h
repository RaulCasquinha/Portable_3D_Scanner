/**
 * @file PointGeometry.h
 * @brief Geometric utilities for converting scanner measurements to 3D points.
 *
 * Provides the @c Point3D type and the spherical-to-Cartesian conversion
 * used to transform a (distance, orientation) pair from the scanner into
 * an absolute 3D coordinate.
 */

#pragma once

/**
 * @struct Point3D
 * @brief A point in 3D Cartesian space, coordinates in millimetres.
 */
struct Point3D {
    float x; ///< X coordinate (mm).
    float y; ///< Y coordinate (mm).
    float z; ///< Z coordinate (mm, positive up).
};

/**
 * @brief Converts a polar scanner measurement to a Cartesian 3D point.
 *
 * Applies the spherical-to-Cartesian transform:
 * @code
 *   x = d · cos(pitch) · cos(yaw)
 *   y = d · cos(pitch) · sin(yaw)
 *   z = d · sin(pitch)
 * @endcode
 *
 * Roll is not applied here; it can be corrected in post-processing on
 * the host side if needed.
 *
 * @param distanceMm Distance measured by the LiDAR, in millimetres.
 * @param pitchRad   Scanner pitch angle (rotation around X), in radians.
 * @param yawRad     Scanner yaw angle (rotation around Z), in radians.
 * @return Corresponding @c Point3D in millimetres.
 */
Point3D toPoint3D(float distanceMm, float pitchRad, float yawRad);
