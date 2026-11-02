/**
 * @file PointGeometry.h
 * @brief Shared spatial types and the world-space point calculation.
 *
 * Defines the three geometric primitives used across the firmware
 * (@c Orientation, @c Position, @c Point3D) and provides the function
 * that combines a LiDAR distance with the device's current position and
 * orientation to produce an absolute 3D point in the reference frame.
 */

#pragma once

/**
 * @struct Orientation
 * @brief Scanner orientation angles in radians, accumulated by the IMU.
 */
struct Orientation {
    float pitch; ///< Rotation around X — nose up (positive) / down (negative).
    float roll;  ///< Rotation around Y — tilt left/right.
    float yaw;   ///< Rotation around Z — heading.
};

/**
 * @struct Position
 * @brief Device position in the reference frame, in millimetres.
 *
 * The origin is wherever @c HAL_IMU::setReference() was last called.
 */
struct Position {
    float x; ///< X coordinate (mm).
    float y; ///< Y coordinate (mm).
    float z; ///< Z coordinate (mm, positive up).
};

/**
 * @struct Point3D
 * @brief A scanned surface point in the reference frame, in millimetres.
 */
struct Point3D {
    float x; ///< X coordinate (mm).
    float y; ///< Y coordinate (mm).
    float z; ///< Z coordinate (mm).
};

/**
 * @brief Computes a scanned surface point in the world reference frame.
 *
 * Combines the device's current position with the LiDAR distance and the
 * pointing direction derived from the device orientation:
 * @code
 *   dir  = [ cos(pitch)·cos(yaw),  cos(pitch)·sin(yaw),  sin(pitch) ]
 *   point = devicePos + distanceMm · dir
 * @endcode
 *
 * Roll does not affect the 1-D LiDAR pointing direction and is ignored
 * here; it can be applied in post-processing on the host if needed.
 *
 * @param distanceMm  Distance measured by the LiDAR (mm).
 * @param devicePos   Device position relative to the reference origin (mm).
 * @param orient      Device orientation relative to the reference frame.
 * @return            Scanned surface point in the reference frame (mm).
 */
Point3D toWorldPoint(float distanceMm, const Position& devicePos, const Orientation& orient);
