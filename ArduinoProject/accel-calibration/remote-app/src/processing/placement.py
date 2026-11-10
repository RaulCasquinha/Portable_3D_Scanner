"""
The checks the board makes before it accepts a position, repeated on the PC.

Layer: processing. numpy only. They let the app say, before d is pressed,
whether the board would accept the orientation it is in now. The limits come
from the board ($BEGIN) or match accel_calib.c.
"""

from .geometry import angle_between_deg

__all__ = ["REFERENCE_MAX_ANGLE_DEG", "nearest_angle_deg", "is_new_position",
           "reference_offset_deg"]

REFERENCE_MAX_ANGLE_DEG = 25.0   # As in accel_calib.c: a reference this close to its axis


def nearest_angle_deg(reading, captured):
    """Angle from a reading to the nearest captured position, deg; None if there is none."""
    if len(captured) == 0:
        return None
    return min(angle_between_deg(reading, position) for position in captured)


def is_new_position(reading, captured, min_separation_deg):
    """True if the reading is at least min_separation_deg from every captured position."""
    nearest = nearest_angle_deg(reading, captured)
    return nearest is None or nearest >= min_separation_deg


def reference_offset_deg(reading, axis):
    """Angle from a reading to the axis a reference needs (e.g. (0, 0, 1) for +Z up), deg."""
    return angle_between_deg(reading, axis)
