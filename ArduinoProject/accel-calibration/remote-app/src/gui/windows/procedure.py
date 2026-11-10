"""
Calibration procedure window: the temperature now and at the start and end of the
capture, a progress bar of the measured positions and references, and the table of
every one of them (mean and std of x, y, z, |g|). Values not there yet show ---.

Layer: view.
"""

import dearpygui.dearpygui as dpg

from ...services.report import (MEASUREMENT_HEADER, capture_temperature_text,
                                measurement_table, temperature_row)
from ..style import GREY, use_mono_font
from .base import Window

__all__ = ["ProcedureWindow"]

_REFERENCE_NAMES = ("Z", "X")       # The order the board measures them: +Z up, then +X up


class ProcedureWindow(Window):
    """The temperatures, the progress and the captured positions."""

    TAG = "win_procedure"
    TITLE = "Calibration procedure"

    def __init__(self, window_id, board, settings):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board, for the capture and the temperature.
            settings: The CalibrationSettingsWindow, for the positions and references
                before a capture.
        """
        super().__init__(window_id)
        self._board = board
        self._settings = settings
        with self.window():
            # Always shown, with --- for what the board hasn't sent (a sensor without a
            # thermometer, and before a capture nothing is measured)
            dpg.add_text("", tag="procedure.temperature")
            dpg.add_separator()
            dpg.add_text("captured positions", color=GREY)
            dpg.add_progress_bar(tag="procedure.progress", width=-1, overlay="")
            dpg.add_text(MEASUREMENT_HEADER, tag="procedure.measurements_header")
            with dpg.child_window(tag="procedure.measurements_window", height=-1,
                                  border=False, horizontal_scrollbar=True):
                dpg.add_text("", tag="procedure.measurements")
        for tag in ("procedure.temperature", "procedure.measurements_header",
                    "procedure.measurements"):
            use_mono_font(tag)

    def update_slow(self):
        """The temperatures, the progress and the table: they change at most once a second."""
        board = self._board
        capture = board.capture
        temperature_text = (temperature_row("temperature", board.temperature_now_c) + "\n"
                            + capture_temperature_text(capture))
        if self._changed("temperature", temperature_text):
            dpg.set_value("procedure.temperature", temperature_text)

        # During and after a capture, what the board announced; before one, what the
        # settings ask for
        if capture.expected_count > 0:
            positions, references = capture.expected_positions, capture.expected_references
        else:
            positions, references = self._settings.positions(), self._settings.references()
        reference_names = _REFERENCE_NAMES[:references]
        total = positions + len(reference_names)
        if self._changed("progress", (capture.measured_count, total)):
            dpg.set_value("procedure.progress", capture.measured_count / total if total else 0.0)
            dpg.configure_item("procedure.progress",
                               overlay=f"{capture.measured_count} / {total}")

        table = measurement_table(capture.measurements(), positions, reference_names)
        if self._changed("measurements", table):
            dpg.set_value("procedure.measurements", table)
            # Keep the newest measured row in view: the --- rows come after it
            fraction = min(1.0, capture.measured_count / (table.count("\n") + 1))
            window = "procedure.measurements_window"
            dpg.set_y_scroll(window, dpg.get_y_scroll_max(window) * fraction)
