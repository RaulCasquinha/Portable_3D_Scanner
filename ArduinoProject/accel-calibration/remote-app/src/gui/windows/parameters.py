"""
Parameters window: the calibration in use as a table, and the C defines to paste
into app_calib_conf.h.

Layer: view. Redrawn only when the calibration changes.
"""

import dearpygui.dearpygui as dpg

from ...processing.sfem import PARAMETER_LABELS, model_to_parameters
from ...services.report import PLACEHOLDER
from ..style import GREY, use_mono_font
from .base import Window

__all__ = ["ParametersWindow"]

_TABLE_WIDTH = 215


class ParametersWindow(Window):
    """The fitted parameters and the defines."""

    TAG = "win_params"
    TITLE = "Parameters"

    def __init__(self, window_id, board):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board, for the calibration in use.
        """
        super().__init__(window_id)
        self._board = board
        with self.window():
            with dpg.group(horizontal=True):
                with dpg.child_window(width=_TABLE_WIDTH, height=-1, border=False):
                    with dpg.table(header_row=True, borders_innerH=True, borders_outerH=True,
                                   borders_innerV=True, row_background=True,
                                   tag="parameters.table"):
                        dpg.add_table_column(label="parameter")
                        dpg.add_table_column(label="estimated")
                        for i, name in enumerate(PARAMETER_LABELS):
                            with dpg.table_row():
                                dpg.add_text(name)
                                dpg.add_text(PLACEHOLDER, tag=f"parameters.value.{i}")
                # The defines are what this app is for: always in view, beside the table
                with dpg.group():
                    dpg.add_text(_summary(None), tag="parameters.summary")
                    dpg.add_text("For app_calib_conf.h (select, Ctrl+C):", color=GREY)
                    # A read-only input, not a text, so it can be selected and copied
                    dpg.add_input_text(tag="parameters.defines", multiline=True, readonly=True,
                                       width=-1, height=-1)
        use_mono_font("parameters.summary")

    def update(self, live):
        board = self._board
        if not self._changed("calibration", board.calibration_version):
            return
        calibration = board.calibration
        values = model_to_parameters(board.model)
        for i, value in enumerate(values):
            dpg.set_value(f"parameters.value.{i}", f"{value:+.6f}" if calibration else PLACEHOLDER)
        dpg.set_value("parameters.defines", board.defines())
        dpg.set_value("parameters.summary", _summary(calibration))


def _summary(calibration):
    """The rms errors and the temperature of a calibration; PLACEHOLDER for the values
    if there is none."""
    if calibration is None:
        return ("|g| error rms {0:>8} mg before\n              {0:>8} mg after\n"
                "calibrated at {0:>7} C".format(PLACEHOLDER))
    text = ("|g| error rms {:8.3f} mg before\n              {:8.3f} mg after"
            .format(calibration.rms_before_mg, calibration.rms_after_mg))
    if calibration.is_compensated:
        temperatures_c = calibration.compensation.temperatures_c
        text += ("\ncompensated  {:+7.2f} to {:+7.2f} C\nvalues at    {:+7.2f} C"
                 .format(temperatures_c[0], temperatures_c[-1], calibration.temperature_c))
    elif calibration.temperature_c is not None:
        text += f"\ncalibrated at {calibration.temperature_c:+7.2f} C"
    return text
