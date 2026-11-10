"""
Display settings window: how the readings are shown. Pause display (freezes the
live windows to study them; the board keeps measuring), where calibrated values
come from, which fitted terms are applied to them, the smoothing of the numbers and
the arrow (optional: the mean of the last so many seconds) and whether the nearest
axis reference is drawn. None of these change the fit or the defines.

Layer: view. The live windows and the Status sphere read the settings through this
window's methods. The presets (in Calibration settings) save them too.
"""

import dearpygui.dearpygui as dpg

from ...controller.board import CAL_SOURCES
from ...processing.sfem import APPLIED_TERMS
from .base import Window

__all__ = ["TERM_LABELS", "MIN_SMOOTHING_S", "MAX_SMOOTHING_S", "DisplaySettingsWindow"]

TERM_LABELS = {"bias": "b", "scale": "S", "nonortho": "alpha", "misalignment": "M"}
DEFAULT_SMOOTHING_S = 0.5
MIN_SMOOTHING_S = 0.01
MAX_SMOOTHING_S = 10.0


class DisplaySettingsWindow(Window):
    """How the readings are shown."""

    TAG = "win_display"
    TITLE = "Display settings"
    TEXT_INPUTS = ("display.smoothing_s",)

    def __init__(self, window_id):
        """
        Args:
            window_id: The id reserved for this window.
        """
        super().__init__(window_id)
        self._is_paused = False
        with self.window():
            # Freezes the display only; the board keeps streaming
            dpg.add_button(label="Pause display", tag="display.pause",
                           callback=self._on_pause_click)
            dpg.add_combo(list(CAL_SOURCES), default_value=CAL_SOURCES[0],
                          tag="display.cal_source", label="cal source", width=170)
            # Off: that term is left out of the calibrated values shown (bias 0, scale 1,
            # angles 0), to see what it does. The fit and the defines don't change.
            with dpg.group(horizontal=True):
                dpg.add_text("apply:")
                for term in APPLIED_TERMS:
                    dpg.add_checkbox(label=TERM_LABELS[term], tag=f"display.apply.{term}",
                                     default_value=True)
            # Optional: off, the readouts and the arrow show the newest reading
            with dpg.group(horizontal=True):
                dpg.add_checkbox(label="smoothing", tag="display.smoothing",
                                 default_value=True, callback=lambda: self._enable_time())
                dpg.add_input_float(tag="display.smoothing_s",
                                    default_value=DEFAULT_SMOOTHING_S,
                                    min_value=MIN_SMOOTHING_S, max_value=MAX_SMOOTHING_S,
                                    min_clamped=True, max_clamped=True, step=0.05,
                                    step_fast=0.5, format="%.2f s", width=130)
            dpg.add_checkbox(label="show nearest axis reference", tag="display.show_reference",
                             default_value=True)

    @property
    def is_paused(self):
        """True while the live display (Live stream, Tilt, Errors) is frozen by Pause."""
        return self._is_paused

    def _on_pause_click(self):
        self._is_paused = not self._is_paused
        dpg.set_item_label("display.pause",
                           "Resume display" if self._is_paused else "Pause display")

    def cal_source(self):
        """Where calibrated readings come from, from board.CAL_SOURCES."""
        return dpg.get_value("display.cal_source")

    def applied_terms(self):
        """The terms of the PC fit applied to the shown values, from sfem.APPLIED_TERMS."""
        return tuple(term for term in APPLIED_TERMS if dpg.get_value(f"display.apply.{term}"))

    def smoothing_s(self):
        """The time the readouts and the arrow average, s; 0 with smoothing off (the
        newest reading)."""
        if not dpg.get_value("display.smoothing"):
            return 0.0
        return float(dpg.get_value("display.smoothing_s"))

    def update(self, live):
        self._enable_time()        # A preset may have set the tick box

    def _enable_time(self):
        """The smoothing time can be changed only while smoothing is on."""
        is_on = bool(dpg.get_value("display.smoothing"))
        if self._changed("smoothing", is_on):
            dpg.configure_item("display.smoothing_s", enabled=is_on)

    def shows_reference(self):
        """True to draw the nearest axis reference."""
        return dpg.get_value("display.show_reference")
