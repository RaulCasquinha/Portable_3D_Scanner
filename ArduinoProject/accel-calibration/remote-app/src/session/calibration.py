"""
The calibration in use: its model and what it came from.

Layer: session. Two kinds:

- simple: one capture at one temperature (or with no temperature);
- temperature compensated: lines in temperature fitted to several simple
  calibrations of one sensor (processing/compensation.py). Its model follows the
  sensor's temperature.
"""

from dataclasses import dataclass, replace

from ..processing.compensation import TemperatureCompensation
from ..processing.sfem import CalibrationModel

__all__ = ["Calibration"]


@dataclass(frozen=True)
class Calibration:
    """
    One calibration. Frozen: a change makes a new one (see at_temperature).

    Attributes:
        model: The correction in use now.
        terms: The terms that were estimated, from sfem.TERMS.
        samples_per_position: Samples averaged per position, or None if unknown.
        rms_before_mg, rms_after_mg: Rms |g| error of the positions without and with
            the calibration, mg.
        temperature_c: Simple: the temperature it was calibrated at. Compensated: the
            temperature its model is at now. degC, or None.
        compensation: The temperature lines of a compensated calibration, else None.
        record: The record a compensated calibration was loaded from, saved again as
            it is; None otherwise.
    """

    model: CalibrationModel
    terms: tuple
    samples_per_position: object
    rms_before_mg: float
    rms_after_mg: float
    temperature_c: object = None
    compensation: TemperatureCompensation = None
    record: dict = None

    @property
    def is_compensated(self):
        """True for a temperature compensated calibration."""
        return self.compensation is not None

    def at_temperature(self, temperature_c):
        """
        This calibration at another temperature: a compensated one moves its model
        along the lines; a simple one stays as it is.
        """
        if not self.is_compensated:
            return self
        return replace(self, model=self.compensation.model_at(temperature_c),
                       temperature_c=float(temperature_c))
