"""
The temperatures the sensor sent ($TEMP).

Layer: session. Stays empty with a sensor that has no thermometer (ADXL345), so
nothing may depend on it.
"""

from collections import deque

__all__ = ["TemperatureLog"]

_HISTORY = 3600       # Readings kept: one hour at 1 Hz

class TemperatureLog:
    """
    The latest temperatures.

    Attributes:
        times_s: Time of each temperature, s.
        temperatures_c: The temperatures, degC.
    """

    def __init__(self):
        self.times_s = deque(maxlen=_HISTORY)
        self.temperatures_c = deque(maxlen=_HISTORY)

    def __len__(self):
        return len(self.temperatures_c)

    @property
    def latest_c(self):
        """The last temperature, degC, or None if the sensor sent none."""
        return float(self.temperatures_c[-1]) if self.temperatures_c else None

    def append(self, time_s, temperature_c):
        """Add one temperature."""
        self.times_s.append(time_s)
        self.temperatures_c.append(temperature_c)
