"""
One calibration capture: the positions and references the board measured.

Layer: session. Built from protocol messages ($BEGIN, $POS, $REF, $CALTEMP,
$END, $ABORT), saved in records as a dict (to_dict / from_dict).
"""

from dataclasses import dataclass

import numpy as np

from ..processing.placement import (REFERENCE_MAX_ANGLE_DEG, nearest_angle_deg,
                                    reference_offset_deg)
from ..protocol.messages import (CaptureAbort, CaptureBegin, CaptureEnd, CaptureTemperature,
                                 Position, Reference)

__all__ = ["EMPTY", "CAPTURING", "DONE", "ABORTED", "Capture", "Measurement", "PlacementCheck"]

# Capture states
EMPTY = "empty"
CAPTURING = "capturing"
DONE = "done"
ABORTED = "aborted"

_STD_FIELDS = 4                     # Std of x, y, z and |g|
_DEFAULT_SEPARATION_DEG = 30.0
_REFERENCE_AXES = {"Z": np.array([0.0, 0.0, 1.0]), "X": np.array([1.0, 0.0, 0.0])}


@dataclass(frozen=True)
class Measurement:
    """One measured position or reference, for tables."""

    label: str                      # "1", "2"... or "ref Z", "ref X"
    mean_g: np.ndarray              # Mean X Y Z, g
    std_g: np.ndarray               # Std of X, Y, Z and the raw |g|, g; NaN if unknown


@dataclass(frozen=True)
class PlacementCheck:
    """
    Would the board accept a position now? The same tests as the firmware, made
    on the live reading before d is pressed.
    """

    target: str                     # "first", "position", or the reference axis "Z" / "X"
    angle_deg: float                # Nearest captured position, or the offset from the axis
    limit_deg: float                # Least separation, or the most offset allowed
    is_ok: bool                     # True: d would be accepted


class Capture:
    """
    The positions and references of one capture.

    Attributes:
        state: EMPTY, CAPTURING, DONE or ABORTED.
        expected_positions, expected_references: From $BEGIN.
        positions: Mean raw reading of each position, list of (3,), g.
        position_std: Std of each position's samples, list of (4,), g.
        references: Mean raw reading of each reference by axis name, {"Z": (3,)}, g.
        reference_std: Std of each reference's samples, {"Z": (4,)}, g.
        temperature_c: Temperature at "BEGIN" and "END", only those the board sent.
        min_separation_deg: Least angle between positions the board accepts.
    """

    def __init__(self):
        self.state = EMPTY
        self.expected_positions = 0
        self.expected_references = 0
        self.positions = []
        self.position_std = []
        self.references = {}
        self.reference_std = {}
        self.temperature_c = {}
        self.min_separation_deg = _DEFAULT_SEPARATION_DEG

    # -- state ----------------------------------------------------------------

    def clear(self):
        """Forget everything: no positions, references or temperatures (state EMPTY)."""
        self.__init__()

    @property
    def is_capturing(self):
        """True between $BEGIN and $END or $ABORT."""
        return self.state == CAPTURING

    @property
    def measured_count(self):
        """Positions and references measured so far."""
        return len(self.positions) + len(self.references)

    @property
    def expected_count(self):
        """Positions and references the capture needs."""
        return self.expected_positions + self.expected_references

    @property
    def calibration_temperature_c(self):
        """Mean of the temperatures at the start and end, degC; None without both."""
        begin, end = self.temperature_c.get("BEGIN"), self.temperature_c.get("END")
        return None if begin is None or end is None else (begin + end) / 2

    def measurements(self):
        """Every measured position and reference, in the order they were taken."""
        rows = [Measurement(str(i), mean, std) for i, (mean, std) in
                enumerate(zip(self.positions, self.position_std), 1)]
        rows += [Measurement(f"ref {name}", mean, self.reference_std.get(name, _no_std()))
                 for name, mean in self.references.items()]
        return rows

    # -- messages -------------------------------------------------------------

    def handle(self, message):
        """
        Update the capture with one message from the board.

        Returns:
            True if the message changed the positions, references or state; False
            for messages that are not about the capture, and for temperatures.
        """
        if isinstance(message, CaptureTemperature):
            self.temperature_c[message.moment] = message.temperature_c
            return False
        if isinstance(message, CaptureBegin):
            self.clear()
            self.state = CAPTURING
            self.expected_positions = message.positions
            self.expected_references = message.references
            self.min_separation_deg = message.min_separation_deg
        elif isinstance(message, Position):
            self.positions.append(message.mean_g)
            self.position_std.append(message.std_g)
        elif isinstance(message, Reference):
            self.references[message.name] = message.mean_g
            self.reference_std[message.name] = message.std_g
        elif isinstance(message, CaptureEnd):
            self.state = DONE
        elif isinstance(message, CaptureAbort):
            # An aborted capture is dropped: its positions must not look like part of
            # the next one
            self.clear()
            self.state = ABORTED
        else:
            return False
        return True

    def check_placement(self, reading_g):
        """
        The test d would face with the board as it is now.

        Args:
            reading_g: Mean raw reading now, (3,), g.

        Returns:
            A PlacementCheck, or None when no capture is running.
        """
        if not self.is_capturing:
            return None
        if len(self.positions) < self.expected_positions:
            nearest = nearest_angle_deg(reading_g, self.positions)
            if nearest is None:
                return PlacementCheck("first", 0.0, self.min_separation_deg, True)
            return PlacementCheck("position", nearest, self.min_separation_deg,
                                  nearest >= self.min_separation_deg)
        name = "Z" if "Z" not in self.references else "X"     # +Z up is measured first
        offset = reference_offset_deg(reading_g, _REFERENCE_AXES[name])
        return PlacementCheck(name, offset, REFERENCE_MAX_ANGLE_DEG,
                              offset <= REFERENCE_MAX_ANGLE_DEG)

    # -- saving ---------------------------------------------------------------

    def to_dict(self):
        """The capture as plain lists and dicts, for a JSON record."""
        return {"positions_g": [p.tolist() for p in self.positions],
                "references_g": {k: v.tolist() for k, v in self.references.items()},
                "position_std_g": [s.tolist() for s in self.position_std],
                "reference_std_g": {k: v.tolist() for k, v in self.reference_std.items()},
                "temperature_c": dict(self.temperature_c),
                "calibration_temperature_c": self.calibration_temperature_c}

    @classmethod
    def from_dict(cls, data):
        """
        A finished capture from what to_dict() gave.

        Raises:
            KeyError, TypeError, ValueError: The dict is not a capture.
        """
        capture = cls()
        capture.positions = [np.array(p, dtype=float) for p in data["positions_g"]]
        capture.references = {k: np.array(v, dtype=float)
                              for k, v in data["references_g"].items()}
        capture.position_std = [_padded_std(s) for s in data.get("position_std_g", [])]
        capture.reference_std = {k: _padded_std(v)
                                 for k, v in data.get("reference_std_g", {}).items()}
        # Older files have fewer std rows than positions
        capture.position_std += [_no_std()] * (len(capture.positions) - len(capture.position_std))
        capture.temperature_c = dict(data.get("temperature_c", {}))
        capture.expected_positions = len(capture.positions)
        capture.expected_references = len(capture.references)
        capture.state = DONE
        return capture


def _no_std():
    return np.full(_STD_FIELDS, np.nan)


def _padded_std(values):
    """Std of x, y, z, |g|, with NaN for what older files don't have."""
    std = _no_std()
    values = np.asarray(values, dtype=float)[:_STD_FIELDS]
    std[:len(values)] = values
    return std
