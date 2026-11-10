"""
What the live windows (Live stream, Tilt, Errors) show, computed once per frame.

Layer: controller. No GUI code: the windows only copy these arrays into plots.

There is no true orientation, so the reference is the nearest of +-X +-Y +-Z to
the calibrated reading: it assumes the board lies on a face on a level surface.
Beyond SNAP_MAX_DEG from any axis there is no reference (NaN).
"""

from dataclasses import dataclass

import numpy as np

from ..processing.geometry import snap_to_axis

__all__ = ["SNAP_MAX_DEG", "LiveView", "live_view"]

SNAP_MAX_DEG = 15.0          # Farthest a reading may be from an axis to use it as reference
_MAX_POINTS = 3000           # Plot at most this many points per line


@dataclass(frozen=True)
class LiveView:
    """
    The readings of the plotted time window, and the means of the smoothing window.

    Attributes:
        times_s: Time of each plotted reading, (n,), s.
        raw_g, corrected_g, reference_g: Raw, calibrated and reference readings, (n, 3), g.
        mean_raw_g, mean_corrected_g, mean_reference_g: Means over the smoothing window,
            (3,), g; the reference has NaN when there is none.
        start_s, end_s: The plotted time window, s.
        has_data: False before the first reading.
    """

    times_s: np.ndarray
    raw_g: np.ndarray
    corrected_g: np.ndarray
    reference_g: np.ndarray
    mean_raw_g: np.ndarray
    mean_corrected_g: np.ndarray
    mean_reference_g: np.ndarray
    start_s: float
    end_s: float
    has_data: bool


def live_view(board, window_s, smoothing_s, source, applied):
    """
    The live data to show now.

    Args:
        board: The Board.
        window_s: How much time the plots show, s.
        smoothing_s: The window of the mean readouts, s.
        source: Where the calibrated readings come from, from board.CAL_SOURCES.
        applied: The terms of the PC fit to apply, from sfem.APPLIED_TERMS.
    """
    buffer = board.buffer
    first = buffer.first_index_after(board.time_s - window_s)
    step = max(1, (len(buffer) - first) // _MAX_POINTS)      # Decimate long windows
    raw_g = buffer.raw_g[first::step]
    corrected_g = board.corrected(raw_g, buffer.board_cal_g[first::step], source, applied)
    mean_raw_g = board.recent_raw(smoothing_s)
    mean_corrected_g = board.recent_corrected(smoothing_s, source, applied)
    return LiveView(times_s=buffer.times_s[first::step],
                    raw_g=raw_g,
                    corrected_g=corrected_g,
                    reference_g=snap_to_axis(corrected_g, SNAP_MAX_DEG),
                    mean_raw_g=mean_raw_g,
                    mean_corrected_g=mean_corrected_g,
                    mean_reference_g=snap_to_axis(mean_corrected_g, SNAP_MAX_DEG)[0],
                    start_s=board.time_s - window_s,
                    end_s=board.time_s,
                    has_data=len(buffer) > 0)
