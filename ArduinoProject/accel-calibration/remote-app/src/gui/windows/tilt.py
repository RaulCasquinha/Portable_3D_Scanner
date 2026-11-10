"""
Tilt window, in one of two modes, chosen at the top:
- roll / pitch: roll and pitch of the board;
- axis angles: the angle between each sensor axis and gravity.
Each mode has its readout table and its plots against time.

Layer: view. Roll and pitch are 0 with +Z up; an axis angle is 0 when the axis
points up (it reads +1 g) and 180 when it points down. See processing/geometry.py.
The axis plot draws the calibrated angles in the axis colours (red, green, blue) and
the raw ones in their complements, the subtractive primaries (cyan, magenta, yellow).
"""

import dearpygui.dearpygui as dpg

from ...processing.geometry import axis_angles_deg, roll_pitch_deg
from ..style import (AXIS_COLORS, CAL_COLOR, RAW_AXIS_COLORS, RAW_COLOR, REFERENCE_COLOR,
                     use_mono_font)
from ..widgets.plots import (build_time_plots, clear_xy, follow_time_plots, release_time_plots,
                             set_xy)
from .base import Window
from .stream import corrected_label

__all__ = ["MODE_TAG", "TiltWindow"]

MODE_TAG = "tilt.mode"              # The mode selector; the presets save it
_ROLL_PITCH, _AXIS_ANGLES = "roll / pitch", "axis angles"
_MODES = (_ROLL_PITCH, _AXIS_ANGLES)
_GROUPS = {_ROLL_PITCH: "tilt.roll_pitch", _AXIS_ANGLES: "tilt.axis_angles"}

_SOURCE_LINES = (("raw", RAW_COLOR), ("cal", CAL_COLOR), ("ref", REFERENCE_COLOR))
# The axis plot: one line per axis, calibrated in the axis colours (red, green, blue), then
# raw in their complements, the subtractive primaries (cyan, magenta, yellow)
_AXIS_LINES = (tuple((f"{name} (cal)", color) for name, color in zip("xyz", AXIS_COLORS))
               + tuple((f"{name} (raw)", color) for name, color in zip("xyz", RAW_AXIS_COLORS)))

# The readout of each mode: header, (name width, column widths), the format of a row,
# and the angles of a mean reading
_READOUTS = {
    _ROLL_PITCH: ("{:<6}{:>10}{:>10}".format("", "roll deg", "pitch deg"), (6, (10, 10)),
                  "\n{:<6}{:+10.3f}{:+10.3f}", roll_pitch_deg),
    _AXIS_ANGLES: ("{:<6}{:>9}{:>9}{:>9}".format("", "x-g deg", "y-g deg", "z-g deg"),
                   (6, (9, 9, 9)), "\n{:<6}{:9.3f}{:9.3f}{:9.3f}", axis_angles_deg),
}


class TiltWindow(Window):
    """Roll and pitch, or the axis angles: raw, calibrated and of the nearest axis."""

    TAG = "win_tilt"
    TITLE = "Tilt"

    def __init__(self, window_id, display):
        """
        Args:
            window_id: The id reserved for this window.
            display: The DisplaySettingsWindow, for the source and the reference.
        """
        super().__init__(window_id)
        self._display = display
        with self.window(pos=(420, 300), width=640, height=330):
            dpg.add_radio_button(list(_MODES), default_value=_ROLL_PITCH, tag=MODE_TAG,
                                 horizontal=True, callback=lambda: self._show_mode())
            dpg.add_text("", tag="tilt.readout")
            # One group of plots per mode; only the chosen one is shown and updated
            with dpg.group(tag=_GROUPS[_ROLL_PITCH]):
                plots = build_time_plots("tilt.plot.", (("roll", "roll"), ("pitch", "pitch")),
                                         1, 2, "deg", _SOURCE_LINES)
            with dpg.group(tag=_GROUPS[_AXIS_ANGLES], show=False):
                plots.update(build_time_plots("tilt.plot.", (("axes", "axis to gravity"),),
                                              1, 1, "deg", _AXIS_LINES))
            self._plots = plots
        use_mono_font("tilt.readout")
        self._show_mode()

    def mode(self):
        """The mode chosen: "roll / pitch" or "axis angles"."""
        mode = dpg.get_value(MODE_TAG)
        return mode if mode in _MODES else _ROLL_PITCH

    def set_paused(self, is_paused):
        if is_paused:
            release_time_plots(self._plots)
        else:
            follow_time_plots(self._plots)

    def update(self, live):
        self._show_mode()          # A preset may have set the mode
        if not live.has_data:
            self._show_empty_readout()
            return
        source = self._display.cal_source()
        label = corrected_label(source)
        if self._changed("source", source):
            for key in ("roll", "pitch"):
                dpg.configure_item(self._plots[key][1][1], label=label)
            for name, series in zip("xyz", self._plots["axes"][1][:3]):    # The calibrated
                dpg.configure_item(series, label=f"{name} ({label})")
        shows_reference = self._display.shows_reference()
        mode = self.mode()

        if mode == _ROLL_PITCH:
            # Roll and pitch: raw, calibrated and the reference
            angles = {"raw": roll_pitch_deg(live.raw_g),
                      "cal": roll_pitch_deg(live.corrected_g),
                      "ref": roll_pitch_deg(live.reference_g)}
            for index, key in enumerate(("roll", "pitch")):
                time_axis, (raw_series, corrected_series, reference_series) = self._plots[key]
                set_xy(raw_series, live.times_s, angles["raw"][index])
                set_xy(corrected_series, live.times_s, angles["cal"][index])
                if shows_reference:
                    set_xy(reference_series, live.times_s, angles["ref"][index])
                else:
                    clear_xy(reference_series)
                dpg.set_axis_limits(time_axis, live.start_s, live.end_s)
        else:
            # The angle of each axis to gravity: calibrated, then raw
            time_axis, axis_series = self._plots["axes"]
            corrected_angles = axis_angles_deg(live.corrected_g)
            raw_angles = axis_angles_deg(live.raw_g)
            for axis in range(3):
                set_xy(axis_series[axis], live.times_s, corrected_angles[:, axis])
                set_xy(axis_series[3 + axis], live.times_s, raw_angles[:, axis])
            dpg.set_axis_limits(time_axis, live.start_s, live.end_s)

        rows = [("raw", live.mean_raw_g), (label, live.mean_corrected_g)]
        if shows_reference:
            rows.insert(0, ("ref", live.mean_reference_g))
        header, _, row_format, angles_of = _READOUTS[mode]
        self._changed("tilt.readout", None)
        dpg.set_value("tilt.readout", header + "".join(
            row_format.format(name, *angles_of(vector)) for name, vector in rows))

    def _show_mode(self):
        """Show the plots of the chosen mode and hide the others."""
        mode = self.mode()
        if not self._changed("mode", mode):
            return
        for each, group in _GROUPS.items():
            dpg.configure_item(group, show=each == mode)
        self._show_empty_readout()     # The columns change; the numbers come next frame

    def _show_empty_readout(self):
        """--- in the readout, in the rows the readings will fill."""
        names = ["raw", corrected_label(self._display.cal_source())]
        if self._display.shows_reference():
            names.insert(0, "ref")
        header, widths, _, _ = _READOUTS[self.mode()]
        self._set_empty_readout("tilt.readout", header, names, *widths)
