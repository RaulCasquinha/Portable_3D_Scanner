"""
Controls window: the status line and the capture commands.

Layer: view. Connecting and the stream are in the Connection window, pausing the
display in Display settings, the progress, temperatures and captured positions in
Calibration procedure, the board's messages in Board messages, saving the
calibration in Records.
"""

import dearpygui.dearpygui as dpg

from ..style import STATUS_COLOR, use_mono_font
from .base import Window

__all__ = ["ControlsWindow"]


class ControlsWindow(Window):
    """Commands to the board, and where the calibration stands."""

    TAG = "win_controls"
    TITLE = "Controls"

    def __init__(self, window_id, board, settings):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board.
            settings: The CalibrationSettingsWindow, for the capture settings.
        """
        super().__init__(window_id)
        self._board = board
        self._settings = settings
        with self.window():
            dpg.add_text("not connected", tag="controls.status", wrap=0, color=STATUS_COLOR)
            with dpg.group(horizontal=True):
                dpg.add_button(label="Start", tag="controls.start", callback=self._on_start_click)
                dpg.add_button(label="Capture (d)", tag="controls.capture",
                               callback=lambda: self._board.capture_position())
                dpg.add_button(label="Abort (q)", tag="controls.abort",
                               callback=lambda: self._board.abort())
        use_mono_font("controls.status")

    def update(self, live):
        dpg.set_value("controls.status", self._board.status)

    def _on_start_click(self):
        settings = self._settings
        self._board.start_capture(settings.positions(), settings.samples(),
                                  settings.references(), settings.terms())
