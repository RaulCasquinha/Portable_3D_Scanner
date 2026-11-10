"""
The SFEM calibration model (Yuan et al. 2023), as in the report ACCEL_CALIB.tex.

Layer: processing. numpy only.

    corrected = M C S (raw - b) = K (raw - b)

    b       bias of each axis, g
    S       scale of each axis, diag(S_x, S_y, S_z)
    C       non-orthogonality, lower triangular [[1,0,0],[a_yx,1,0],[a_zx,a_zy,1]]
    M       misalignment, Rz(psi) Ry(theta) Rx(phi)

The 12 parameters can also be written as one vector in display units (mg, -, deg),
in the order of PARAMETER_LABELS, for tables, plots and regressions.
"""

import numpy as np

__all__ = ["TERMS", "APPLIED_TERMS", "PARAMETER_LABELS", "CalibrationModel", "rotation_x", "rotation_y",
           "rotation_z", "nonortho_matrix", "misalignment_matrix", "model_to_parameters",
           "parameters_to_model"]

# The error terms a fit can estimate, in the order of the parameter vector
TERMS = ("bias", "scale", "nonortho")

# The terms a correction can apply: the fitted ones and the misalignment M
APPLIED_TERMS = TERMS + ("misalignment",)

# The parameter vector: name and display unit of each entry
PARAMETER_LABELS = ("b_x [mg]", "b_y [mg]", "b_z [mg]", "S_x", "S_y", "S_z",
                    "a_yx [deg]", "a_zx [deg]", "a_zy [deg]",
                    "phi [deg]", "theta [deg]", "psi [deg]")

_G_TO_MG = 1e3


class CalibrationModel:
    """
    The SFEM correction of one sensor. A new model is the identity: it leaves the
    readings as they are.

    Attributes:
        b: Bias of each axis, g.
        S: Scale of each axis.
        alpha: Non-orthogonality a_yx, a_zx, a_zy, rad.
        angles: Misalignment phi, theta, psi, rad.
    """

    def __init__(self):
        self.b = np.zeros(3)
        self.S = np.ones(3)
        self.alpha = np.zeros(3)
        self.angles = np.zeros(3)

    @property
    def intrinsic_matrix(self):
        """C S: the part of K that the magnitude of gravity can estimate."""
        return nonortho_matrix(*self.alpha) @ np.diag(self.S)

    @property
    def matrix(self):
        """K = M C S, so corrected = K (raw - b)."""
        return misalignment_matrix(*self.angles) @ self.intrinsic_matrix

    def apply(self, readings_g):
        """
        Correct readings.

        Args:
            readings_g: One reading (3,) or many (n, 3), g.

        Returns:
            The corrected readings, always (n, 3), g.
        """
        return (np.atleast_2d(readings_g) - self.b) @ self.matrix.T

    def apply_intrinsic(self, readings_g):
        """Correct readings with C S only, without the misalignment M. (n, 3), g."""
        return (np.atleast_2d(readings_g) - self.b) @ self.intrinsic_matrix.T

    def with_terms(self, terms):
        """
        A copy that applies only some terms; the others are the identity
        (bias 0, scale 1, non-orthogonality 0, misalignment 0).

        Args:
            terms: The terms to keep, from APPLIED_TERMS.
        """
        other = self.copy()
        if "bias" not in terms:
            other.b = np.zeros(3)
        if "scale" not in terms:
            other.S = np.ones(3)
        if "nonortho" not in terms:
            other.alpha = np.zeros(3)
        if "misalignment" not in terms:
            other.angles = np.zeros(3)
        return other

    def copy(self):
        """An independent copy of this model."""
        other = CalibrationModel()
        other.b, other.S = self.b.copy(), self.S.copy()
        other.alpha, other.angles = self.alpha.copy(), self.angles.copy()
        return other


def rotation_x(angle_rad):
    """Rotation matrix about x."""
    c, s = np.cos(angle_rad), np.sin(angle_rad)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def rotation_y(angle_rad):
    """Rotation matrix about y."""
    c, s = np.cos(angle_rad), np.sin(angle_rad)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def rotation_z(angle_rad):
    """Rotation matrix about z."""
    c, s = np.cos(angle_rad), np.sin(angle_rad)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


def nonortho_matrix(a_yx, a_zx, a_zy):
    """C, lower triangular, from the three non-orthogonality terms (rad)."""
    return np.array([[1, 0, 0], [a_yx, 1, 0], [a_zx, a_zy, 1]])


def misalignment_matrix(phi, theta, psi):
    """M = Rz(psi) Ry(theta) Rx(phi), angles in rad."""
    return rotation_z(psi) @ rotation_y(theta) @ rotation_x(phi)


def model_to_parameters(model):
    """The 12 parameters of a model in display units (mg, -, deg), see PARAMETER_LABELS."""
    return np.r_[model.b * _G_TO_MG, model.S, np.degrees(model.alpha), np.degrees(model.angles)]


def parameters_to_model(parameters):
    """The model of a 12-parameter vector in display units, see PARAMETER_LABELS."""
    parameters = np.asarray(parameters, dtype=float)
    model = CalibrationModel()
    model.b = parameters[0:3] / _G_TO_MG
    model.S = parameters[3:6].copy()
    model.alpha = np.radians(parameters[6:9])
    model.angles = np.radians(parameters[9:12])
    return model
