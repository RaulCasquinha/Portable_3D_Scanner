"""
Temperature compensation of the SFEM parameters.

Layer: processing. numpy only.

Calibrations of one sensor at several temperatures give a parameter vector per
temperature. Each parameter group chosen to change with temperature gets the
least-squares line p(T) = p_ref + slope (T - T_ref); the others are their mean.
Bias and scale really change with temperature; the non-orthogonality and the
alignment are mechanical, so their spread between captures is mostly where the
board was placed, and a slope there fits noise.
"""

import numpy as np

from .regression import LinearTrend
from .sfem import parameters_to_model

__all__ = ["SLOPE_GROUPS", "SLOPE_GROUP_NAMES", "DEFAULT_SLOPE_GROUPS",
           "TemperatureCompensation"]

# The parameter groups that can get a slope: name, short label, first index in the vector
SLOPE_GROUPS = (("bias", "b", 0), ("scale", "S", 3), ("nonortho", "alpha", 6),
                ("alignment", "angles", 9))
SLOPE_GROUP_NAMES = tuple(name for name, _, _ in SLOPE_GROUPS)
DEFAULT_SLOPE_GROUPS = ("bias", "scale")

_GROUP_SIZE = 3


class TemperatureCompensation:
    """
    The SFEM parameters as lines in temperature.

    Attributes:
        trend: The LinearTrend of the 12 parameters (display units) against
            temperature, degC.
        slope_groups: The groups that have a slope, from SLOPE_GROUP_NAMES.
    """

    def __init__(self, temperatures_c, parameters, slope_groups=SLOPE_GROUP_NAMES, hold=False):
        """
        Args:
            temperatures_c: Temperature of each calibration, (n,), degC.
            parameters: The 12 parameters of each calibration, (n, 12), display units.
            slope_groups: The groups that get a slope; the others are their mean.
            hold: Outside the measured temperatures, keep the values at the nearest end.

        Raises:
            ValueError: Fewer than 2 different temperatures.
        """
        self.slope_groups = tuple(name for name in SLOPE_GROUP_NAMES if name in slope_groups)
        sloped = np.zeros(len(SLOPE_GROUPS) * _GROUP_SIZE, dtype=bool)
        for name, _, first in SLOPE_GROUPS:
            sloped[first:first + _GROUP_SIZE] = name in self.slope_groups
        try:
            self.trend = LinearTrend(temperatures_c, parameters, sloped, hold)
        except ValueError:
            raise ValueError("the calibrations need 2 or more temperatures") from None

    @property
    def temperatures_c(self):
        """The temperatures of the calibrations, sorted, degC."""
        return self.trend.x

    @property
    def parameters(self):
        """The parameters of the calibrations, in the order of temperatures_c."""
        return self.trend.values

    @property
    def reference_c(self):
        """The mean temperature, degC: the lines are written about it."""
        return self.trend.x_ref

    @property
    def hold(self):
        """True if the values outside the range are held at the ends."""
        return self.trend.hold

    def parameters_at(self, temperature_c):
        """The 12 parameters at this temperature, display units."""
        return self.trend.value_at(temperature_c)

    def model_at(self, temperature_c):
        """The CalibrationModel at this temperature."""
        return parameters_to_model(self.parameters_at(temperature_c))
