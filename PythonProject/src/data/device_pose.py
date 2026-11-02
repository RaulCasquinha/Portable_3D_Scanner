"""Holds the most recent device position and orientation received from firmware."""

from __future__ import annotations

import numpy as np


class DevicePose:
    """Stores the latest POSE packet values.

    Positions are in millimetres; angles are in radians as sent by the
    firmware. Conversion to degrees for display is left to the GUI layer.
    """

    def __init__(self) -> None:
        self._position    = np.zeros(3, dtype=np.float32)  # x, y, z  (mm)
        self._orientation = np.zeros(3, dtype=np.float32)  # pitch, roll, yaw (rad)

    def update(self, px: float, py: float, pz: float,
               pitch: float, roll: float, yaw: float) -> None:
        """Replace the stored pose with new values from a POSE packet."""
        self._position[:]    = (px, py, pz)
        self._orientation[:] = (pitch, roll, yaw)

    @property
    def position(self) -> np.ndarray:
        """(3,) float32 array: [x, y, z] in mm."""
        return self._position.copy()

    @property
    def orientation(self) -> np.ndarray:
        """(3,) float32 array: [pitch, roll, yaw] in radians."""
        return self._orientation.copy()
