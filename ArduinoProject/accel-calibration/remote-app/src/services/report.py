"""
The texts the GUI shows in monospace: tables of measurements and records.

Layer: services. Plain strings, no GUI code. Numbers have a fixed width and a
forced sign so that columns stay aligned and don't jump as values change.
"""

import numpy as np

from ..processing.sfem import PARAMETER_LABELS
from ..storage.record_format import compensation_from_record, is_compensated

__all__ = ["PLACEHOLDER", "RECORD_HEADER", "MEASUREMENT_HEADER", "placeholder_row",
           "temperature_row", "capture_temperature_text", "measurement_table", "record_row",
           "record_text"]

_G_TO_MG = 1e3
_G_TO_UG = 1e6

# Where a value goes while there is none (not connected, not measured yet), so the
# readouts keep their shape
PLACEHOLDER = "---"

RECORD_HEADER = "{:<20}{:<14}{:>4}{:>6}{:>9}{:>9}{:>9}".format(
    "saved", "sensor", "n", "k", "T [C]", "raw mg", "cal mg")

# Mean in mg and std in ug of each axis and of |g|, for every measurement
_MEASUREMENT_COLUMNS = "{:<6}" + "{:>9}{:>7}" * 4
MEASUREMENT_HEADER = _MEASUREMENT_COLUMNS.format(
    "pos", "x mean", "std", "y mean", "std", "z mean", "std", "|g|mean", "std") + \
    "\n" + _MEASUREMENT_COLUMNS.format("", *(["mg", "ug"] * 4))

_POSITION_HEADER = "{:<5}{:>11}{:>11}{:>11}{:>11}{:>11}{:>9}".format(
    "", "x mg", "y mg", "z mg", "raw err mg", "cal err mg", "std ug")


def placeholder_row(name, name_width, column_widths):
    """A table row with PLACEHOLDER in every column, right-aligned like the numbers."""
    return f"{name:<{name_width}}" + "".join(f"{PLACEHOLDER:>{width}}"
                                            for width in column_widths)


def temperature_row(name, temperature_c):
    """One aligned line: a name and a temperature, or PLACEHOLDER for None."""
    value = PLACEHOLDER if temperature_c is None else f"{temperature_c:+7.2f}"
    return f"{name:<18}{value:>7} C"


def capture_temperature_text(capture):
    """Temperatures at the start and end of a capture, and their change and mean;
    PLACEHOLDER for those the board hasn't sent."""
    begin, end = capture.temperature_c.get("BEGIN"), capture.temperature_c.get("END")
    has_both = begin is not None and end is not None
    return "\n".join((temperature_row("calibration start", begin),
                      temperature_row("calibration end", end),
                      temperature_row("change", end - begin if has_both else None),
                      temperature_row("calibration mean", capture.calibration_temperature_c)))


def measurement_table(measurements, expected_positions=0, expected_references=()):
    """
    One row per measured position and reference: mean and std of x, y, z and |g|.

    The |g| mean is the magnitude of the mean reading; its std is the std of the
    raw |g| of the samples, as the board sends it. Positions not measured yet, up to
    expected_positions, and the expected references not measured yet (names such as
    "Z", "X", in the order the board measures them) get a row of PLACEHOLDER. The
    references follow the positions.
    """
    positions = [row for row in measurements if row.label.isdigit()]
    references = {row.label: row for row in measurements if not row.label.isdigit()}
    rows = [_measurement_row(row) for row in positions]
    rows += [placeholder_row(str(number), 6, (9, 7) * 4)
             for number in range(len(positions) + 1, expected_positions + 1)]
    labels = [f"ref {name}" for name in expected_references]
    labels += [label for label in references if label not in labels]
    rows += [_measurement_row(references[label]) if label in references
             else placeholder_row(label, 6, (9, 7) * 4) for label in labels]
    return "\n".join(rows)


def _measurement_row(measurement):
    """One measured position or reference, as a row of measurement_table."""
    mean_mg = measurement.mean_g * _G_TO_MG
    std_ug = measurement.std_g * _G_TO_UG
    values = [mean_mg[0], std_ug[0], mean_mg[1], std_ug[1], mean_mg[2], std_ug[2],
              np.linalg.norm(mean_mg), std_ug[3]]
    return ("{:<6}" + "{:>+9.2f}{:>7.1f}" * 4).format(measurement.label, *values)


def record_row(file_name, record):
    """One line of the records list. A file that is not a record still gets a line."""
    try:
        rms = record["magnitude_error_rms_mg"]
        temperature = ("interp" if is_compensated(record) else
                       _optional(record["calibration_temperature_c"], "+.2f"))
        return "{:<20}{:<14.13}{:>4}{:>6}{:>9}{:>9.3f}{:>9.3f}".format(
            record["saved_at"].replace("T", " "), record["sensor"],
            len(record["capture"]["positions_g"]), record["samples_per_position"],
            temperature, rms["before"], rms["after"])
    except (KeyError, TypeError, ValueError, AttributeError):
        return f"{file_name}: not a readable record"


def record_text(record):
    """Everything about a record, for the Records window."""
    try:
        if is_compensated(record):
            return _compensated_text(record)
        return _simple_text(record)
    except (KeyError, IndexError, TypeError, ValueError, AttributeError) as error:
        return f"this record is incomplete: {error!r}"


def _simple_text(record):
    """A simple record: result, parameters and every position."""
    K = np.array(record["K"], dtype=float)
    bias_g = np.array(record["bias_g"], dtype=float)
    capture = record["capture"]
    positions = capture["positions_g"]
    rms = record["magnitude_error_rms_mg"]
    lines = [f"{record['sensor']}   {record['saved_at'].replace('T', ' ')}",
             f"{record['method']}, terms: {' '.join(record['terms'])}, "
             f"{len(positions)} positions x {record['samples_per_position']} samples"]
    temperature = capture.get("temperature_c", {})
    if temperature:
        lines.append("temperature {} C start, {} C end".format(
            _optional(temperature.get("BEGIN"), "+.2f"), _optional(temperature.get("END"), "+.2f")))
    lines.append(f"|g| error rms {rms['before']:8.3f} mg before, {rms['after']:8.3f} mg after")
    if len(positions) <= 3 * len(record["terms"]):
        # As many unknowns as equations (the normal 6 positions for bias and scale): the
        # fit passes through every position, so the error after it is 0 by construction
        lines.append("exact fit: as many positions as unknowns, so the error after is 0; "
                     "repeat the calibration to check it")
    lines += ["",
              "bias [mg]   {:+10.3f}{:+10.3f}{:+10.3f}".format(*(bias_g * _G_TO_MG)),
              "scale       {:+10.6f}{:+10.6f}{:+10.6f}".format(*record["scale"]),
              "alpha [deg] {:+10.4f}{:+10.4f}{:+10.4f}".format(*record["nonortho_deg"]),
              "phi theta psi [deg] {:+9.4f}{:+9.4f}{:+9.4f}".format(*record["alignment_deg"]),
              "", _POSITION_HEADER]
    stds = capture.get("position_std_g", [])
    lines += [_position_row(str(i), position, stds[i] if i < len(stds) else None, K, bias_g)
              for i, position in enumerate(positions)]
    lines += [_position_row(f"ref {name}", position,
                            capture.get("reference_std_g", {}).get(name), K, bias_g)
              for name, position in capture["references_g"].items()]
    return "\n".join(lines)


def _compensated_text(record):
    """A compensated record: its lines, and the calibrations they were fitted to."""
    table = record["temperature_table"]
    compensation = compensation_from_record(record)
    temperatures_c = compensation.temperatures_c
    trend = compensation.trend
    rms = record["magnitude_error_rms_mg"]
    lines = [f"{record['sensor']}   {record['saved_at'].replace('T', ' ')}",
             f"temperature compensated: linear regression of {len(table)} calibrations, "
             f"{temperatures_c[0]:+.2f} to {temperatures_c[-1]:+.2f} C",
             f"terms: {' '.join(record['terms'])}, "
             f"{len(record['capture']['positions_g'])} positions in all",
             f"|g| error rms {rms['before']:8.3f} mg before, {rms['after']:8.3f} mg after "
             "(each capture with the line at its own temperature)",
             f"slope for: {' '.join(compensation.slope_groups) or 'none'} (the others are the "
             f"mean); outside the range: "
             f"{'held at the nearest end' if compensation.hold else 'the lines go on'}",
             "", f"p(T) = p_ref + slope (T - {compensation.reference_c:+.2f} C)",
             "{:<12}{:>14}{:>14}{:>14}".format("", "p_ref", "slope /C", "residual rms")]
    lines += ["{:<12}{:+14.6f}{:+14.3e}{:14.3e}".format(name, *values) for name, *values in
              zip(PARAMETER_LABELS, trend.at_reference, trend.slope, trend.residual_rms)]
    lines += ["", "the calibrations it is fitted to:",
              "{:<9}{:>10}{:>10}{:>10}{:>11}{:>11}{:>11}".format(
                  "T [C]", "b_x mg", "b_y mg", "b_z mg", "S_x", "S_y", "S_z")]
    lines += ["{:<+9.2f}{:+10.3f}{:+10.3f}{:+10.3f}{:+11.6f}{:+11.6f}{:+11.6f}".format(
        row["temperature_c"], *(np.array(row["bias_g"]) * _G_TO_MG), *row["scale"])
        for row in table]
    lines += ["", "{:<9}{:>10}{:>10}{:>10}{:>11}{:>11}{:>11}".format(
        "T [C]", "a_yx deg", "a_zx deg", "a_zy deg", "phi deg", "theta deg", "psi deg")]
    lines += ["{:<+9.2f}{:+10.4f}{:+10.4f}{:+10.4f}{:+11.4f}{:+11.4f}{:+11.4f}".format(
        row["temperature_c"], *row["nonortho_deg"], *row["alignment_deg"]) for row in table]
    lines += ["", "made from:"]
    lines += ["{:<+9.2f}{}".format(row["temperature_c"], row.get("source", "?").replace("T", " "))
              for row in table]
    return "\n".join(lines)


def _position_row(label, position, std, K, bias_g):
    """A position: raw mean, |g| error before and after K (U - b), std of the raw |g|."""
    U = np.asarray(position, dtype=float)
    std_g_ug = float(std[3]) * _G_TO_UG if std is not None and len(std) > 3 else float("nan")
    return "{:<5}{:+11.3f}{:+11.3f}{:+11.3f}{:+11.3f}{:+11.3f}{:9.1f}".format(
        label, *(U * _G_TO_MG), (np.linalg.norm(U) - 1.0) * _G_TO_MG,
        (np.linalg.norm(K @ (U - bias_g)) - 1.0) * _G_TO_MG, std_g_ug)


def _optional(value, spec):
    """A number formatted with spec, or PLACEHOLDER for None."""
    return PLACEHOLDER if value is None else format(value, spec)
