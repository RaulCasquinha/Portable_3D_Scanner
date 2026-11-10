"""
Merge simple calibrations of one sensor into a temperature compensated record.

Layer: services. The record stores one table row per calibration; the lines are
fitted from the table each time it is read (record_format.compensation_from_record),
so the record holds the data and not only the result.
"""

import time

import numpy as np

from ..processing.compensation import DEFAULT_SLOPE_GROUPS, SLOPE_GROUP_NAMES
from ..processing.sfem import TERMS
from ..processing.statistics import rms_magnitude_error_mg
from ..storage.record_format import (COMPENSATED_METHOD, compensation_from_record,
                                     is_compensated, record_parameters, record_temperature_c)
from .export import c_defines

__all__ = ["usable_records", "compensated_record"]


def usable_records(records):
    """The records that can be merged: simple, readable, with a temperature."""
    return [record for record in records
            if record is not None and not is_compensated(record)
            and record_temperature_c(record) is not None
            and record_parameters(record) is not None]


def compensated_record(records, slope_groups=DEFAULT_SLOPE_GROUPS, hold=False):
    """
    A new temperature compensated record.

    Args:
        records: Record dicts; those not usable_records() are left out.
        slope_groups: The parameter groups that get a slope, from SLOPE_GROUP_NAMES.
        hold: Outside the measured temperatures, keep the values at the nearest end.

    Returns:
        The record, ready to save. Its plain fields are at the mean temperature.

    Raises:
        ValueError: Records of several sensors, fewer than 2 temperatures, or an
            incomplete record.
    """
    usable = usable_records(records)
    if len({record["sensor"] for record in usable}) > 1:
        raise ValueError("tick the records of one sensor only")
    if len({round(record_temperature_c(record), 2) for record in usable}) < 2:
        raise ValueError("tick plain calibrations at 2 or more temperatures")

    table = [{"temperature_c": record_temperature_c(record),
              "bias_g": record["bias_g"], "scale": record["scale"],
              "nonortho_deg": record["nonortho_deg"], "alignment_deg": record["alignment_deg"],
              "source": record["saved_at"]}
             for record in sorted(usable, key=record_temperature_c)]
    options = {"slope_groups": [name for name in SLOPE_GROUP_NAMES if name in slope_groups],
               "hold_outside_range": bool(hold)}
    compensation = compensation_from_record({"temperature_table": table, **options})

    usable.sort(key=lambda record: record["saved_at"])
    newest = usable[-1]
    try:
        capture, raw_g, corrected_g = _merged_capture(usable, newest, compensation)
        samples = newest["samples_per_position"]
    except (KeyError, TypeError, ValueError) as error:
        raise ValueError(f"a ticked record is incomplete: {error!r}") from None

    reference_c = compensation.reference_c
    model = compensation.model_at(reference_c)
    return {"sensor": newest["sensor"],
            "saved_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "calibration_temperature_c": reference_c,
            "method": COMPENSATED_METHOD,
            "terms": [term for term in TERMS if any(term in r["terms"] for r in usable)],
            "samples_per_position": samples,
            "bias_g": model.b.tolist(),
            "scale": model.S.tolist(),
            "nonortho_deg": np.degrees(model.alpha).tolist(),
            "alignment_deg": np.degrees(model.angles).tolist(),
            "K": model.matrix.tolist(),
            # Each capture corrected with the lines at its own temperature
            "magnitude_error_rms_mg": {"before": rms_magnitude_error_mg(raw_g),
                                       "after": rms_magnitude_error_mg(corrected_g)},
            "defines": c_defines(model, reference_c, is_compensated=True),
            "temperature_table": table,
            **options,
            "capture": capture}


def _merged_capture(records, newest, compensation):
    """
    The positions of all the records as one capture (the references of the newest),
    and every position raw and corrected at its record's temperature, (n, 3) each.
    """
    raw, corrected, positions, stds = [], [], [], []
    for record in records:
        positions_g = np.array(record["capture"]["positions_g"], dtype=float)
        raw.append(positions_g)
        model = compensation.model_at(record_temperature_c(record))
        corrected.append(model.apply(positions_g))
        positions += record["capture"]["positions_g"]
        stds += record["capture"].get("position_std_g", [])
    capture = {"positions_g": positions, "position_std_g": stds,
               "references_g": newest["capture"]["references_g"],
               "reference_std_g": newest["capture"].get("reference_std_g", {}),
               "temperature_c": {}, "calibration_temperature_c": None}
    return capture, np.vstack(raw), np.vstack(corrected)
