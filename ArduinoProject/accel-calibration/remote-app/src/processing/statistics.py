"""
Error statistics of gravity readings.

Layer: processing. numpy only.
"""

import numpy as np

__all__ = ["magnitude_error_mg", "rms_magnitude_error_mg"]

_G_TO_MG = 1e3


def magnitude_error_mg(readings_g, g=1.0):
    """|reading| - g of each reading, (n,), mg."""
    return (np.linalg.norm(np.atleast_2d(readings_g), axis=1) - g) * _G_TO_MG


def rms_magnitude_error_mg(readings_g, g=1.0):
    """Rms of |reading| - g over still readings, mg: 0 for a perfect calibration."""
    return float(np.sqrt(np.mean(magnitude_error_mg(readings_g, g) ** 2)))
