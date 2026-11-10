"""
Least-squares lines of many values against one variable.

Layer: processing. numpy only. Nothing here knows about calibration: it fits
y_j(x) = y_ref_j + slope_j (x - x_ref) for every column j of a table, with
x_ref the mean of x, so it can be used for any trend (temperature, time...).
"""

import numpy as np

__all__ = ["LinearTrend"]


class LinearTrend:
    """
    A straight line per column of values, fitted by least squares.

    Columns without a slope are constant at their mean. With hold, the values
    outside the measured range of x are those at the nearest end, because the
    lines are unknown there; without it the lines go on.

    Attributes:
        x: The measured x, sorted, (n,).
        values: The measured values in the same order, (n, m).
        x_ref: The mean of x: the lines are written about it.
        at_reference: Value of each line at x_ref, (m,).
        slope: Slope of each line per unit of x, (m,); 0 where there is none.
        residual_rms: Rms distance of the measured values from their line, (m,).
        hold: True if values outside the range are held at the ends.
    """

    def __init__(self, x, values, sloped=None, hold=False):
        """
        Args:
            x: The variable, (n,). It needs 2 or more different values.
            values: One row per x, (n, m).
            sloped: Which columns get a slope, (m,) of bool. None: all of them.
            hold: Hold the end values outside the range of x.

        Raises:
            ValueError: Fewer than 2 different x.
        """
        order = np.argsort(x)
        self.x = np.asarray(x, dtype=float)[order]
        self.values = np.atleast_2d(np.asarray(values, dtype=float)[order])
        self.hold = bool(hold)
        if len(self.x) < 2 or np.ptp(self.x) == 0:
            raise ValueError("a trend needs 2 or more different x")
        self.x_ref = float(self.x.mean())

        design = np.column_stack([np.ones(len(self.x)), self.x - self.x_ref])
        (self.at_reference, self.slope), *_ = np.linalg.lstsq(design, self.values, rcond=None)
        if sloped is not None:
            # A constant column is its mean: the same as a line with slope 0 about the mean x
            flat = ~np.asarray(sloped, dtype=bool)
            self.at_reference[flat] = self.values[:, flat].mean(axis=0)
            self.slope[flat] = 0.0
        fitted = design @ np.vstack([self.at_reference, self.slope])
        self.residual_rms = np.sqrt(((self.values - fitted) ** 2).mean(axis=0))

    @property
    def x_min(self):
        """The lowest measured x."""
        return float(self.x[0])

    @property
    def x_max(self):
        """The highest measured x."""
        return float(self.x[-1])

    def value_at(self, x):
        """The value of every line at x, (m,)."""
        if self.hold:
            x = float(np.clip(x, self.x_min, self.x_max))
        return self.at_reference + self.slope * (x - self.x_ref)
