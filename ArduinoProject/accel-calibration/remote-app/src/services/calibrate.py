"""
Calibrate a capture: the ellipsoid fit, then the alignment to the references.

Layer: services.
"""

import numpy as np

from ..processing.alignment import fit_alignment
from ..processing.ellipsoid_fit import fit_ellipsoid
from ..processing.statistics import rms_magnitude_error_mg
from ..session.calibration import Calibration

__all__ = ["calibrate"]


def calibrate(capture, terms, samples_per_position=None):
    """
    The calibration of a finished capture.

    Args:
        capture: A Capture with all its positions; references are optional.
        terms: The terms to estimate, from sfem.TERMS.
        samples_per_position: Samples per position, kept with the calibration.

    Returns:
        A simple Calibration at the capture's temperature.

    Raises:
        ValueError: The capture is incomplete, or the fit failed.
    """
    if not capture.positions or len(capture.positions) < capture.expected_positions:
        raise ValueError("no complete capture to fit")
    positions_g = np.array(capture.positions)
    try:
        model = fit_ellipsoid(positions_g, terms)
        if "Z" in capture.references:
            model.angles = fit_alignment(model, capture.references["Z"],
                                         capture.references.get("X"))
    except np.linalg.LinAlgError as error:
        raise ValueError(str(error)) from None
    return Calibration(model=model,
                       terms=tuple(terms),
                       samples_per_position=samples_per_position,
                       rms_before_mg=rms_magnitude_error_mg(positions_g),
                       rms_after_mg=rms_magnitude_error_mg(model.apply(positions_g)),
                       temperature_c=capture.calibration_temperature_c)
