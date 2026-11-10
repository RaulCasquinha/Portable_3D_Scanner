"""
Closed-form ellipsoid fit: bias, scale and non-orthogonality from static readings.

Layer: processing. numpy only.

A still sensor measures gravity only, so every corrected reading must have the
magnitude g: ||C S (U - b)|| = g. The raw readings therefore lie on an ellipsoid,
(U - b)^T A (U - b) = g^2 with A = (C S)^T (C S). The quadric is fitted by linear
least squares in one step, then b, S and C are taken out of A. The positions are
never used, only the readings, so the board may be placed in any orientation.
The misalignment M can't be seen this way (a rotation keeps the magnitude); see
alignment.py.
"""

import numpy as np

from .sfem import TERMS, CalibrationModel

__all__ = ["fit_ellipsoid", "minimum_positions"]


def minimum_positions(terms):
    """The fewest positions that can estimate these terms: 3 unknowns per term."""
    return 3 * len([term for term in TERMS if term in terms])


def fit_ellipsoid(readings_g, terms=TERMS, g=1.0):
    """
    Fit b, S and alpha to static readings.

    Args:
        readings_g: Mean reading of each position, (n, 3), g.
        terms: The terms to estimate, any of TERMS. The others stay at b = 0,
            S = 1, alpha = 0.
        g: The magnitude every corrected reading should have, g.

    Returns:
        A CalibrationModel with the fitted terms and no misalignment.

    Raises:
        ValueError: No term, fewer positions than unknowns, or readings that don't
            describe an ellipsoid (the fit is not positive definite).
    """
    readings_g = np.asarray(readings_g, dtype=float)
    has_bias, has_scale, has_nonortho = (term in terms for term in TERMS)
    unknowns = minimum_positions(terms)
    if unknowns == 0:
        raise ValueError("select at least one error term")
    if len(readings_g) < unknowns:
        raise ValueError(f"need at least {unknowns} positions for {unknowns} parameters, "
                         f"got {len(readings_g)}")

    # The design matrix: the quadratic terms the model allows, the linear terms
    # if there is a bias, and a constant
    x, y, z = readings_g.T
    if has_nonortho:
        quadratic = [x * x, y * y, z * z, 2 * x * y, 2 * x * z, 2 * y * z]
    elif has_scale:
        quadratic = [x * x, y * y, z * z]
    else:
        quadratic = [x * x + y * y + z * z]
    linear = [x, y, z] if has_bias else []
    design = np.column_stack(quadratic + linear + [np.ones_like(x)])

    # The least-squares solution with norm 1 is the last right singular vector.
    # full_matrices: with as many positions as unknowns (e.g. 6 for b and S) the
    # matrix has fewer rows than columns and this is its null vector
    solution = np.linalg.svd(design, full_matrices=True)[2][-1]

    q = solution[:len(quadratic)]
    constant = solution[-1]
    if has_nonortho:
        A = np.array([[q[0], q[3], q[4]], [q[3], q[1], q[5]], [q[4], q[5], q[2]]])
    elif has_scale:
        A = np.diag(q)
    else:
        A = q[0] * np.eye(3)
    d = solution[len(quadratic):len(quadratic) + 3] if has_bias else np.zeros(3)
    bias_g = -0.5 * np.linalg.solve(A, d)
    A = A * g ** 2 / (bias_g @ A @ bias_g - constant)     # scale so the radius is g
    try:
        L = _reverse_cholesky(A)
    except np.linalg.LinAlgError:
        raise ValueError("ellipsoid fit failed (not positive definite)") from None

    # L = C S: its diagonal is S, and below it C times S
    model = CalibrationModel()
    if has_bias:
        model.b = bias_g
    if has_scale:
        model.S = np.diag(L).copy()
    if has_nonortho:
        S = np.diag(L)
        model.alpha = np.array([L[1, 0] / S[0], L[2, 0] / S[0], L[2, 1] / S[1]])
    return model


def _reverse_cholesky(A):
    """Lower-triangular L with L^T L = A (A symmetric positive definite).

    numpy's Cholesky gives L L^T; reversing the axes before and after gives L^T L.
    """
    J = np.eye(3)[::-1]
    K = np.linalg.cholesky(J @ A @ J)
    return J @ K.T @ J
