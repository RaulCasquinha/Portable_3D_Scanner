"""
A calibration record: the dict saved as calibrations/<sensor>/<time>.json.

Layer: storage. Converts between records and the session objects (Calibration,
Capture). Records already saved must keep loading, so fields are only added.

Fields of every record:

    sensor, saved_at                     who and when ("2026-11-03T15:19:51")
    method                               "ellipsoid-fit", or COMPENSATED_METHOD
    terms                                estimated terms, from sfem.TERMS
    samples_per_position
    calibration_temperature_c            degC or null
    bias_g, scale, nonortho_deg, alignment_deg, K
    magnitude_error_rms_mg               {"before": mg, "after": mg}
    defines                              the C defines for app_calib_conf.h
    capture                              Capture.to_dict()

A temperature compensated record also has:

    temperature_table      one row per simple calibration: temperature_c, bias_g,
                           scale, nonortho_deg, alignment_deg, source (its saved_at)
    slope_groups           groups with a slope, from compensation.SLOPE_GROUP_NAMES
    hold_outside_range     bool

Its plain fields are the values at the mean temperature, and its capture holds
the positions of all its calibrations, so it reads like a simple record.
"""

import numpy as np

from ..processing.compensation import SLOPE_GROUP_NAMES, TemperatureCompensation
from ..processing.sfem import TERMS, CalibrationModel
from ..session.calibration import Calibration
from ..session.capture import Capture

__all__ = ["FIT_METHOD", "COMPENSATED_METHOD", "is_compensated", "record_parameters",
           "record_temperature_c", "record_from_calibration", "calibration_from_record",
           "compensation_from_record"]

FIT_METHOD = "ellipsoid-fit"
COMPENSATED_METHOD = "temperature-compensated"

_G_TO_MG = 1e3


def is_compensated(record):
    """True for a temperature compensated record."""
    return isinstance(record, dict) and "temperature_table" in record


def record_parameters(record):
    """The 12 parameters of a record (or of a temperature table row) in display units,
    as sfem.PARAMETER_LABELS; None if it hasn't got them all."""
    try:
        return np.concatenate([np.asarray(record["bias_g"], dtype=float) * _G_TO_MG,
                               np.asarray(record["scale"], dtype=float),
                               np.asarray(record["nonortho_deg"], dtype=float),
                               np.asarray(record["alignment_deg"], dtype=float)])
    except (KeyError, TypeError, ValueError):
        return None


def record_temperature_c(record):
    """The temperature a record was calibrated at, degC, or None."""
    value = record.get("calibration_temperature_c") if isinstance(record, dict) else None
    return None if value is None else float(value)


def record_from_calibration(calibration, capture, sensor, saved_at, defines):
    """
    The record of a calibration.

    Args:
        calibration: The Calibration to save.
        capture: The Capture it was fitted to.
        sensor: The sensor's name.
        saved_at: Save time, "YYYY-MM-DDTHH:MM:SS".
        defines: The C defines text (services/export.py).

    Returns:
        The record dict. A compensated calibration is its loaded record again,
        under this sensor and time.
    """
    if calibration.record is not None:
        return dict(calibration.record, sensor=sensor, saved_at=saved_at)
    model = calibration.model
    return {"sensor": sensor,
            "saved_at": saved_at,
            "calibration_temperature_c": capture.calibration_temperature_c,
            "method": FIT_METHOD,
            "terms": list(calibration.terms),
            "samples_per_position": calibration.samples_per_position,
            "bias_g": model.b.tolist(),
            "scale": model.S.tolist(),
            "nonortho_deg": np.degrees(model.alpha).tolist(),
            "alignment_deg": np.degrees(model.angles).tolist(),
            "K": model.matrix.tolist(),
            "magnitude_error_rms_mg": {"before": calibration.rms_before_mg,
                                       "after": calibration.rms_after_mg},
            "defines": defines,
            "capture": capture.to_dict()}


def calibration_from_record(record):
    """
    The calibration and capture of a record. A compensated one is at its mean
    temperature.

    Raises:
        ValueError: The record is incomplete or malformed.
    """
    try:
        model = CalibrationModel()
        model.b = np.array(record["bias_g"], dtype=float)
        model.S = np.array(record["scale"], dtype=float)
        model.alpha = np.radians(np.array(record["nonortho_deg"], dtype=float))
        model.angles = np.radians(np.array(record["alignment_deg"], dtype=float))
        capture = Capture.from_dict(record["capture"])
        rms = record["magnitude_error_rms_mg"]
        compensation = compensation_from_record(record) if is_compensated(record) else None
        calibration = Calibration(
            model=model,
            terms=tuple(term for term in TERMS if term in record["terms"]),
            samples_per_position=record["samples_per_position"],
            rms_before_mg=float(rms["before"]),
            rms_after_mg=float(rms["after"]),
            temperature_c=record_temperature_c(record),
            compensation=compensation,
            record=record if compensation is not None else None)
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        raise ValueError(f"this record cannot be loaded: {error!r}") from None
    if compensation is not None:
        calibration = calibration.at_temperature(compensation.reference_c)
    return calibration, capture


def compensation_from_record(record):
    """
    The TemperatureCompensation of a compensated record.

    Raises:
        ValueError: The table has fewer than 2 complete rows or 2 temperatures.
    """
    rows = record["temperature_table"]
    parameters = [record_parameters(row) for row in rows]
    if len(rows) < 2 or any(p is None for p in parameters):
        raise ValueError("the temperature table needs 2 or more complete rows")
    return TemperatureCompensation([row["temperature_c"] for row in rows], parameters,
                                   record.get("slope_groups", SLOPE_GROUP_NAMES),
                                   record.get("hold_outside_range", False))
