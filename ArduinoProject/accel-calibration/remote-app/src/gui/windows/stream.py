"""
Live stream window: the mean reading as a table, and x, y, z and |g| against time.
Pause display in Display settings freezes it (see set_paused).

Layer: view. The data comes in the frame's LiveView (controller/live_view.py).
"""

import dearpygui.dearpygui as dpg
import numpy as np

from ...controller.board import BUFFER_S, CAL_SOURCE_BOARD
from ..style import CAL_COLOR, GREY, RAW_COLOR, REFERENCE_COLOR, use_mono_font
from ..widgets.plots import (build_time_plots, clear_xy, follow_time_plots, release_time_plots,
                             set_xy)
from .base import Window

__all__ = ["TIME_SCALE_TAG", "MIN_TIME_SCALE_S", "StreamWindow", "corrected_label"]

TIME_SCALE_TAG = "stream.time_scale"   # Also saved in presets
MIN_TIME_SCALE_S = 1.0

_G_TO_MG = 1e3
_PLOTS = (("x", "x axis"), ("y", "y axis"), ("z", "z axis"), ("g", "|g|"))
_READOUT_HEADER = "{:<6}{:>11}{:>11}{:>11}{:>10}".format("", "x mg", "y mg", "z mg", "|g| mg")
_READOUT_WIDTHS = (6, (11, 11, 11, 10))      # Name, then each value column


def corrected_label(source):
    """What to call calibrated readings: "board" when they come from the board."""
    return "board" if source == CAL_SOURCE_BOARD else "cal"


class StreamWindow(Window):
    """The live readings: raw, calibrated and the nearest axis."""

    TAG = "win_stream"
    TITLE = "Live stream"

    def __init__(self, window_id, display):
        """
        Args:
            window_id: The id reserved for this window.
            display: The DisplaySettingsWindow, for the source and the reference.
        """
        super().__init__(window_id)
        self._display = display
        with self.window():
            dpg.add_text("", tag="stream.readout")
            with dpg.group(horizontal=True):
                dpg.add_text("time scale:", color=GREY)
                dpg.add_slider_float(tag=TIME_SCALE_TAG, default_value=BUFFER_S,
                                     min_value=MIN_TIME_SCALE_S, max_value=BUFFER_S, width=-1,
                                     format="%.1f s", clamped=True)
            self._plots = build_time_plots("stream.plot.", _PLOTS, 2, 2, "g",
                                           (("raw", RAW_COLOR), ("cal", CAL_COLOR),
                                            ("ref", REFERENCE_COLOR)))
        use_mono_font("stream.readout")
        self._show_empty_readout()

    def time_scale_s(self):
        """How much time the live plots show, s."""
        return float(np.clip(dpg.get_value(TIME_SCALE_TAG), MIN_TIME_SCALE_S, BUFFER_S))

    def _show_empty_readout(self):
        """--- in the readout, in the rows the readings will fill."""
        names = ["raw", corrected_label(self._display.cal_source())]
        if self._display.shows_reference():
            names.insert(0, "ref")
        self._set_empty_readout("stream.readout", _READOUT_HEADER, names, *_READOUT_WIDTHS)

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
        label = corrected_label(source)
        if self._changed("source", source):
            for _, series in self._plots.values():
                dpg.configure_item(series[1], label=label)
        shows_reference = self._display.shows_reference()

        lines = {"g": (np.linalg.norm(live.raw_g, axis=1),
                       np.linalg.norm(live.corrected_g, axis=1),
                       np.ones(len(live.times_s)))}     # The reference |g| is always 1 g
        for axis, key in enumerate("xyz"):
            lines[key] = (live.raw_g[:, axis], live.corrected_g[:, axis],
                          live.reference_g[:, axis])
        for key, (raw, corrected, reference) in lines.items():
            time_axis, (raw_series, corrected_series, reference_series) = self._plots[key]
            set_xy(raw_series, live.times_s, raw)
            set_xy(corrected_series, live.times_s, corrected)
            if shows_reference:
                set_xy(reference_series, live.times_s, reference)
            else:
                clear_xy(reference_series)
            dpg.set_axis_limits(time_axis, live.start_s, live.end_s)

        rows = [("raw", live.mean_raw_g), (label, live.mean_corrected_g)]
        if shows_reference:
            rows.insert(0, ("ref", live.mean_reference_g))
        self._changed("stream.readout", None)
        dpg.set_value("stream.readout", _READOUT_HEADER + "".join(
            "\n{:<6}{:+11.3f}{:+11.3f}{:+11.3f}{:10.3f}".format(
                name, *(vector * _G_TO_MG), np.linalg.norm(vector) * _G_TO_MG)
            for name, vector in rows))
