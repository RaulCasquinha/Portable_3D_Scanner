"""
Vector helpers for gravity readings.

Layer: processing. numpy only.
"""

import numpy as np

__all__ = ["unit", "roll_pitch_deg", "axis_angles_deg", "snap_to_axis", "angle_between_deg"]

_TINY = 1e-12     # Below this a vector has no direction
_AXES = 3


def axis_angles_deg(vectors):
    """
    Angle between each sensor axis and the gravity vector (report, equation 7).

        theta_k = arccos(m_k / g) = atan2(sqrt(g^2 - m_k^2), m_k),   g = |m|

    The atan2 form is used: it stays exact near 0 and 180 deg, where arccos loses
    precision. sqrt(g^2 - m_k^2) is computed as the norm of the two other
    components, which is the same value without the cancellation.

    Args:
        vectors: One reading (3,) or many (n, 3).

    Returns:
        theta_x, theta_y, theta_z in deg, 0 (axis along gravity's reading, e.g. +Z up)
        to 180: (3,) for one reading, (n, 3) for many.
    """
    vectors = np.asarray(vectors, dtype=float)
    angles = [np.arctan2(np.linalg.norm(np.delete(vectors, k, axis=-1), axis=-1),
                         vectors[..., k])
              for k in range(_AXES)]
    return np.degrees(np.stack(angles, axis=-1))


def unit(vector):
    """The vector scaled to length 1."""
    vector = np.asarray(vector, dtype=float)
    return vector / np.linalg.norm(vector)


def roll_pitch_deg(vectors):
    """
    Roll and pitch of gravity readings, deg: (0, 0) with +Z up.

    Args:
        vectors: One reading (3,) or many (n, 3).

    Returns:
        roll, pitch: floats for one reading, (n,) arrays for many.
    """
    vectors = np.asarray(vectors, dtype=float)
    x, y, z = vectors[..., 0], vectors[..., 1], vectors[..., 2]
    return np.degrees(np.arctan2(y, z)), np.degrees(np.arctan2(-x, np.hypot(y, z)))


def snap_to_axis(vectors, max_deg):
    """
    The nearest of +-X +-Y +-Z to each vector.

    Args:
        vectors: (3,) or (n, 3).
        max_deg: Farthest a vector may be from its axis, deg.

    Returns:
        (n, 3) unit axes; rows of NaN where no axis is within max_deg.
    """
    vectors = np.atleast_2d(vectors)
    units = vectors / np.maximum(np.linalg.norm(vectors, axis=1, keepdims=True), _TINY)
    rows = np.arange(len(units))
    axis = np.abs(units).argmax(axis=1)
    axes = np.zeros_like(units)
    axes[rows, axis] = np.sign(units[rows, axis])
    axes[np.abs(units[rows, axis]) < np.cos(np.radians(max_deg))] = np.nan
    return axes


def angle_between_deg(a, b):
    """Angle between two vectors, deg, 0 to 180."""
    cosine = unit(a) @ unit(b)
    return float(np.degrees(np.arccos(np.clip(cosine, -1.0, 1.0))))
