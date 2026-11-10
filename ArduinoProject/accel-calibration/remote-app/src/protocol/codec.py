"""
Turns the board's lines into messages, and builds what the PC sends.

Layer: protocol. Pure functions. The firmware side is
Accel_Calib_stm32/Application/App/Src/app_protocol.c, with the same lines.
Lines from the board end with \\r\\n (removed by
the transport); values are integers in micro-g (ug) and millidegrees (mdegC):

    $READ,<raw x y z ug>,<cal x y z ug>      50 Hz while streaming
    $TEMP,<mdegC>                            1 Hz, stream or not, if the sensor has one
    $STREAM,<0|1>                            the stream started or stopped
    $BEGIN,<positions>,<references>,<min separation deg>
    $CALTEMP,BEGIN|END,<mdegC>               around a capture, if the sensor has one
    $POS,<index>,<x y z ug>,<std x y z |g| ug>
    $REF,<Z|X>,<x y z ug>,<std x y z |g| ug>
    $END   or   $ABORT
    <text>                                   anything else: a message for the user

The PC sends single keys (d capture a position, q abort, r stream, s stop) and
one command line, sent after q and s (see Board._send_start):

    $START,<positions>,<samples>,<references 0-2>

The firmware also accepts c (calibrate with its own settings), but the app never
sends it: Ctrl+C copies text. Every message, with its fields, types and units, is
drawn in docs/diagrams/out/messages.svg of the project root.
"""

import numpy as np

from .messages import (CaptureAbort, CaptureBegin, CaptureEnd, CaptureTemperature, Position,
                       Reading, Reference, StreamState, Temperature, Text, Unknown)

__all__ = ["KEY_CAPTURE", "KEY_ABORT", "KEY_STREAM", "KEY_STOP", "MAX_POSITIONS",
           "MAX_SAMPLES", "MAX_REFERENCES", "STREAM_RATE_HZ", "parse_line", "format_start",
           "format_capture"]

# Keys the board reacts to
KEY_CAPTURE = "d"
KEY_ABORT = "q"
KEY_STREAM = "r"
KEY_STOP = "s"

# Limits of $START, as checked by the firmware
MAX_POSITIONS = 32
MAX_SAMPLES = 65535
MAX_REFERENCES = 2

STREAM_RATE_HZ = 50.0         # $READ lines per second

_UG_TO_G = 1e-6
_MDEGC_TO_C = 1e-3
_AXES = 3
_STD_FIELDS = 4               # Std of x, y, z and |g|
_DEFAULT_SEPARATION_DEG = 30.0  # Older firmware sends no separation in $BEGIN


def parse_line(line):
    """
    The message of one line from the board.

    Args:
        line: The line without its line end.

    Returns:
        A message from messages.py. Lines without $ are Text; unknown $ lines
        are Unknown.

    Raises:
        ValueError: A known $ line with missing or malformed fields.
    """
    if not line.startswith("$"):
        return Text(line)
    kind, *fields = line.split(",")
    parser = _PARSERS.get(kind)
    if parser is None:
        return Unknown(line)
    try:
        return parser(fields)
    except (IndexError, ValueError) as error:
        raise ValueError(f"bad {kind} line: {line!r} ({error})") from None


def format_start(positions, samples, references):
    """
    The $START command line.

    Raises:
        ValueError: A setting outside what the firmware accepts.
    """
    if not 1 <= positions <= MAX_POSITIONS:
        raise ValueError(f"positions must be 1 to {MAX_POSITIONS}, not {positions}")
    if not 1 <= samples <= MAX_SAMPLES:
        raise ValueError(f"samples must be 1 to {MAX_SAMPLES}, not {samples}")
    if not 0 <= references <= MAX_REFERENCES:
        raise ValueError(f"references must be 0 to {MAX_REFERENCES}, not {references}")
    return f"$START,{positions},{samples},{references}\n"


def format_capture(capture_data, min_separation_deg=_DEFAULT_SEPARATION_DEG):
    """
    The lines a board would send for a saved capture, for tests without a board.

    Args:
        capture_data: A capture as saved in a record (Capture.to_dict()).
        min_separation_deg: Separation to put in $BEGIN, deg.

    Returns:
        A list of lines, from $BEGIN to $END.
    """
    positions = capture_data["positions_g"]
    references = capture_data["references_g"]
    position_std = capture_data.get("position_std_g", [])
    reference_std = capture_data.get("reference_std_g", {})
    temperature = capture_data.get("temperature_c", {})

    lines = [f"$BEGIN,{len(positions)},{len(references)},{min_separation_deg:g}"]
    if "BEGIN" in temperature:
        lines.append(f"$CALTEMP,BEGIN,{_to_mdegc(temperature['BEGIN'])}")
    for i, mean in enumerate(positions):
        std = position_std[i] if i < len(position_std) else []
        lines.append(f"$POS,{i + 1},{_to_ug_fields(mean)},{_to_ug_fields(_full_std(std))}")
    for name, mean in references.items():
        std = _full_std(reference_std.get(name, []))
        lines.append(f"$REF,{name},{_to_ug_fields(mean)},{_to_ug_fields(std)}")
    if "END" in temperature:
        lines.append(f"$CALTEMP,END,{_to_mdegc(temperature['END'])}")
    lines.append("$END")
    return lines


# -- parsers of each kind of line ------------------------------------------------

def _parse_reading(fields):
    values = _ug_to_g(fields)
    if len(values) != 2 * _AXES:
        raise ValueError(f"needs {2 * _AXES} values")
    return Reading(raw_g=values[:_AXES], board_cal_g=values[_AXES:])


def _parse_temperature(fields):
    return Temperature(int(fields[0]) * _MDEGC_TO_C)


def _parse_stream(fields):
    return StreamState(fields[0] == "1")


def _parse_begin(fields):
    separation = float(fields[2]) if len(fields) > 2 else _DEFAULT_SEPARATION_DEG
    return CaptureBegin(int(fields[0]), int(fields[1]), separation)


def _parse_capture_temperature(fields):
    if fields[0] not in ("BEGIN", "END"):
        raise ValueError("expected BEGIN or END")
    return CaptureTemperature(fields[0], int(fields[1]) * _MDEGC_TO_C)


def _parse_position(fields):
    return Position(int(fields[0]), _mean(fields[1:4]), _full_std(_ug_to_g(fields[4:8])))


def _parse_reference(fields):
    if fields[0] not in ("Z", "X"):
        raise ValueError("expected Z or X")
    return Reference(fields[0], _mean(fields[1:4]), _full_std(_ug_to_g(fields[4:8])))


_PARSERS = {
    "$READ": _parse_reading,
    "$TEMP": _parse_temperature,
    "$STREAM": _parse_stream,
    "$BEGIN": _parse_begin,
    "$CALTEMP": _parse_capture_temperature,
    "$POS": _parse_position,
    "$REF": _parse_reference,
    "$END": lambda fields: CaptureEnd(),
    "$ABORT": lambda fields: CaptureAbort(),
}


# -- units ------------------------------------------------------------------------

def _ug_to_g(fields):
    """Integer micro-g fields to an array in g."""
    return np.array([int(field) for field in fields], dtype=float) * _UG_TO_G


def _mean(fields):
    """Three micro-g fields: a mean X Y Z in g."""
    values = _ug_to_g(fields)
    if len(values) != _AXES:
        raise ValueError(f"needs {_AXES} values")
    return values


def _full_std(values):
    """Std of x, y, z, |g|, with NaN for what older firmware or files don't have."""
    std = np.full(_STD_FIELDS, np.nan)
    values = np.asarray(values, dtype=float)[:_STD_FIELDS]
    std[:len(values)] = values
    return std


def _to_ug_fields(values_g):
    """Values in g to comma-separated integer micro-g; NaN as 0."""
    return ",".join(str(int(round(0.0 if np.isnan(value) else value / _UG_TO_G)))
                    for value in np.asarray(values_g, dtype=float))


def _to_mdegc(temperature_c):
    return int(round(temperature_c / _MDEGC_TO_C))
