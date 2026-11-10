"""
Misalignment from known reference positions (report, section Misalignment).

Layer: processing. numpy only.

The magnitude of gravity can't show a rotation, so the misalignment M needs
readings taken in known orientations: +Z up on a level surface gives phi and
theta, and +X up also gives psi.
"""

import numpy as np

from .sfem import rotation_x, rotation_y

__all__ = ["fit_alignment"]


def fit_alignment(model, z_up_g, x_up_g=None):
    """
    The misalignment angles that make the references read as their axes.

    Args:
        model: A CalibrationModel with b, S and alpha already fitted. Not changed.
        z_up_g: Mean raw reading with the board level, +Z up, (3,), g.
        x_up_g: Optional mean raw reading with +X up, (3,), g. Without it psi is 0.

    Returns:
        phi, theta, psi, rad, as an array to put in model.angles.
    """
    level = model.apply_intrinsic(z_up_g)[0]
    phi = np.arctan2(level[1], level[2])
    theta = np.arctan2(-level[0], np.hypot(level[1], level[2]))
    psi = 0.0
    if x_up_g is not None:
        # Undo phi and theta first, then the x reference only turns about z
        x_up = rotation_y(theta) @ rotation_x(phi) @ model.apply_intrinsic(x_up_g)[0]
        psi = np.arctan2(-x_up[1], x_up[0])
    return np.array([phi, theta, psi])
