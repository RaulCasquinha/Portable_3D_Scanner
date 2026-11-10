"""
Errors window: the reading minus the nearest axis, before and after calibration.

Layer: view. Positive: the sensor reads too high. Without a reference (more than
SNAP_MAX_DEG from any axis) the axis errors are NaN; |g| needs no axis.
"""

import dearpygui.dearpygui as dpg
import numpy as np

from ...controller.board import CAL_SOURCE_BOARD
from ..style import CAL_COLOR, RAW_COLOR, use_mono_font
from ..widgets.plots import build_time_plots, follow_time_plots, release_time_plots, set_xy
from .base import Window

__all__ = ["ErrorsWindow"]

_G_TO_MG = 1e3
_PLOTS = (("x", "x axis"), ("y", "y axis"), ("z", "z axis"), ("g", "|g|"))
_READOUT_HEADER = "{:<17}{:>11}{:>11}{:>11}{:>11}".format("", "x mg", "y mg", "z mg", "|g| mg")
_READOUT_WIDTHS = (17, (11, 11, 11, 11))       # Name, then each value column


class ErrorsWindow(Window):
    """How far the readings are from the reference."""

    TAG = "win_errors"
    TITLE = "Errors"

    def __init__(self, window_id, display):
        """
        Args:
            window_id: The id reserved for this window.
            display: The DisplaySettingsWindow, for the source.
        """
        super().__init__(window_id)
        self._display = display
        with self.window():
            dpg.add_text("", tag="errors.readout")
            self._plots = build_time_plots("errors.plot.", _PLOTS, 2, 2, "mg",
                                           (("raw error", RAW_COLOR),
                                            ("calibrated error", CAL_COLOR)))
        use_mono_font("errors.readout")
        self._show_empty_readout()

    def _show_empty_readout(self):
        """--- in the readout, in the rows the readings will fill."""
        names = ("raw error", _corrected_error_name(self._display.cal_source()))
        self._set_empty_readout("errors.readout", _READOUT_HEADER, names, *_READOUT_WIDTHS)

    def set_paused(self, is_paused):
        if is_paused:
            release_time_plots(self._plots)
        else:
            follow_time_plots(self._plots)

    def update(self, live):
        if not live.has_data:
            self._show_empty_readout()
            return
        source = self._display.cal_source()
        corrected_name = "board" if source == CAL_SOURCE_BOARD else "cal"
        if self._changed("source", source):
            for _, series in self._plots.values():
                dpg.configure_item(series[1], label=corrected_name)

        # |g| is compared with 1 g, the axes with the nearest axis
        errors = {"g": ((np.linalg.norm(live.raw_g, axis=1) - 1.0),
                        (np.linalg.norm(live.corrected_g, axis=1) - 1.0))}
        for axis, key in enumerate("xyz"):
            errors[key] = (live.raw_g[:, axis] - live.reference_g[:, axis],
                           live.corrected_g[:, axis] - live.reference_g[:, axis])
        for key, (raw_error, corrected_error) in errors.items():
            time_axis, (raw_series, corrected_series) = self._plots[key]
            set_xy(raw_series, live.times_s, raw_error * _G_TO_MG)
            set_xy(corrected_series, live.times_s, corrected_error * _G_TO_MG)
            dpg.set_axis_limits(time_axis, live.start_s, live.end_s)

        rows = (("raw error", live.mean_raw_g),
                (_corrected_error_name(source), live.mean_corrected_g))
        self._changed("errors.readout", None)
        dpg.set_value("errors.readout", _READOUT_HEADER + "".join(
            "\n{:<17}{:+11.3f}{:+11.3f}{:+11.3f}{:+11.3f}".format(
                name, *((vector - live.mean_reference_g) * _G_TO_MG),
                (np.linalg.norm(vector) - 1.0) * _G_TO_MG)
            for name, vector in rows))


def _corrected_error_name(source):
    """The readout row of the calibrated error: board or PC calibration."""
    return f"{'board' if source == CAL_SOURCE_BOARD else 'calibrated'} error"
