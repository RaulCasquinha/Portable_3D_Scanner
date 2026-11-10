"""
The last seconds of the reading stream.

Layer: session. numpy only. Times are seconds since the buffer was cleared.
"""

import numpy as np

__all__ = ["LiveBuffer"]

_AXES = 3


class LiveBuffer:
    """
    A rolling window of readings: raw, and calibrated by the board.

    Attributes:
        times_s: Time of each reading, (n,), s.
        raw_g: Raw readings, (n, 3), g.
        board_cal_g: The same readings calibrated by the board, (n, 3), g.
    """

    def __init__(self, capacity):
        """
        Args:
            capacity: Most readings kept; older ones are dropped.
        """
        self._capacity = int(capacity)
        self.clear()

    def clear(self):
        """Forget every reading."""
        self.times_s = np.empty(0)
        self.raw_g = np.empty((0, _AXES))
        self.board_cal_g = np.empty((0, _AXES))

    def __len__(self):
        return len(self.times_s)

    def append(self, times_s, raw_g, board_cal_g):
        """Add readings, oldest first: times (n,), raw (n, 3), board_cal (n, 3)."""
        if len(times_s) == 0:
            return
        self.times_s = np.concatenate([self.times_s, times_s])[-self._capacity:]
        self.raw_g = np.concatenate([self.raw_g, raw_g])[-self._capacity:]
        self.board_cal_g = np.concatenate([self.board_cal_g, board_cal_g])[-self._capacity:]

    def first_index_after(self, time_s):
        """Index of the first reading at or after time_s."""
        return int(np.searchsorted(self.times_s, time_s))

    def recent_mean(self, which, count):
        """
        Mean of the last readings.

        Args:
            which: "raw" or "board_cal".
            count: How many of the newest readings, at least 1.

        Returns:
            (3,), g, or None while the buffer is empty.
        """
        values = self.raw_g if which == "raw" else self.board_cal_g
        if len(values) == 0:
            return None
        return values[-max(1, int(count)):].mean(axis=0)
