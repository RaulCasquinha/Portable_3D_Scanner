"""
The calibration as C defines for app_calib_conf.h of Accel_Calib_stm32.

Layer: services. The firmware applies corrected = K (raw - bias) with constants;
it has no temperature table, so a compensated calibration exports its values at
one temperature.
"""

__all__ = ["c_defines"]


def c_defines(model, temperature_c=None, is_compensated=False):
    """
    ACCEL_CAL_BIAS_G and ACCEL_CAL_K, ready to paste.

    Args:
        model: The CalibrationModel to export.
        temperature_c: The temperature it is for, degC, written as a comment; None
            for none.
        is_compensated: True if the model is a compensated calibration at
            temperature_c: the comment says the firmware will use it everywhere.

    Returns:
        The defines, as text with a line end after each line.
    """
    if is_compensated:
        header = (f"// temperature compensated: the values at {temperature_c:+.2f} C. The "
                  "firmware has\n// no table, so it uses them at every temperature\n")
    elif temperature_c is not None:
        header = f"// calibrated at {temperature_c:+.2f} C, mean of the start and end\n"
    else:
        header = ""
    rows = [f"    {{{_c_floats(row)}}}" for row in model.matrix]
    return (header + "// corrected = K * (raw - bias)\n"
            f"#define ACCEL_CAL_BIAS_G {{{_c_floats(model.b)}}}\n"
            "#define ACCEL_CAL_K { \\\n" + ", \\\n".join(rows) + " \\\n}\n")


def _c_floats(values):
    """C float literals: 7 decimals, the precision of a float."""
    return ", ".join(f"{value:.7f}F" for value in values)
