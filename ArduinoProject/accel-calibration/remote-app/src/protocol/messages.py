"""
The messages the board sends, one dataclass per kind of line.

Layer: protocol. Values are already in g and degC: codec.py converts the integer
micro-g and millidegrees of the lines. See codec.py for the line format.
"""

from dataclasses import dataclass

import numpy as np

__all__ = ["Reading", "Temperature", "StreamState", "CaptureBegin", "CaptureTemperature",
           "Position", "Reference", "CaptureEnd", "CaptureAbort", "Text", "Unknown"]


@dataclass(frozen=True)
class Reading:
    """$READ: one sample of the stream, 50 Hz while streaming."""

    raw_g: np.ndarray          # Raw X Y Z, g
    board_cal_g: np.ndarray    # X Y Z calibrated by the board with its flashed constants, g


@dataclass(frozen=True)
class Temperature:
    """$TEMP: the sensor's temperature, 1 Hz. Never sent by a sensor without one."""

    temperature_c: float


@dataclass(frozen=True)
class StreamState:
    """$STREAM: the board started or stopped streaming."""

    is_streaming: bool


@dataclass(frozen=True)
class CaptureBegin:
    """$BEGIN: a capture started."""

    positions: int             # Positions to capture
    references: int            # Reference positions after them, 0 to 2
    min_separation_deg: float  # Least angle between two positions the board accepts


@dataclass(frozen=True)
class CaptureTemperature:
    """$CALTEMP: the temperature at the start (BEGIN) or end (END) of a capture."""

    moment: str                # "BEGIN" or "END"
    temperature_c: float


@dataclass(frozen=True)
class Position:
    """$POS: the mean of one accepted position and the std of its samples."""

    index: int                 # 1 for the first position
    mean_g: np.ndarray         # Mean X Y Z, g
    std_g: np.ndarray          # Std of X, Y, Z and the raw |g|, g; NaN if not sent


@dataclass(frozen=True)
class Reference:
    """$REF: the mean of one reference position (+Z up or +X up)."""

    name: str                  # "Z" or "X": the axis that points up
    mean_g: np.ndarray         # Mean X Y Z, g
    std_g: np.ndarray          # Std of X, Y, Z and the raw |g|, g; NaN if not sent


@dataclass(frozen=True)
class CaptureEnd:
    """$END: the capture finished; the PC fits it."""


@dataclass(frozen=True)
class CaptureAbort:
    """$ABORT: the capture was stopped."""


@dataclass(frozen=True)
class Text:
    """A line without $: a message for the user (prompt, countdown, error)."""

    text: str


@dataclass(frozen=True)
class Unknown:
    """A $ line this app doesn't know, e.g. from a newer firmware. Ignored."""

    line: str
