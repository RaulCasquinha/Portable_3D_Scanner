"""
Temperature analysis window: one parameter of the ticked records against the
temperature they were calibrated at, the calibration in use, and a button to
merge the ticked records into a temperature compensated calibration.

Layer: view. Optional: nothing else depends on it, and with a sensor that has no
thermometer it stays empty.
"""

import dearpygui.dearpygui as dpg

from ...processing.compensation import DEFAULT_SLOPE_GROUPS, SLOPE_GROUPS
from ...processing.sfem import PARAMETER_LABELS, model_to_parameters
from ..style import CAL_COLOR, GREY, ORANGE, YELLOW, fit_padding_theme, line_theme, marker_theme
from .base import Window

__all__ = ["TemperatureWindow"]

_DEFAULT_RANGE_C = (0.0, 50.0)   # x range of the line before any temperature is known
_LINE_MARGIN_C = 1.0             # The line goes this far past the outer points
_X_PADDING = 0.1                 # Keep the outer points off the frame


class TemperatureWindow(Window):
    """Parameters against temperature."""

    TAG = "win_temp"
    TITLE = "Temperature analysis"

    def __init__(self, window_id, board, browser):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board, for the calibration in use and the temperature now.
            browser: The RecordBrowser, for the ticked records.
        """
        super().__init__(window_id)
        self._board = board
        self._browser = browser
        with self.window(pos=(420, 100), width=640, height=480):
            with dpg.group(horizontal=True):
                dpg.add_combo(PARAMETER_LABELS, default_value=PARAMETER_LABELS[0],
                              tag="temperature.parameter", width=170)
                dpg.add_button(label="New compensated calibration", tag="temperature.create",
                               callback=self._on_create_click)
            with dpg.group(horizontal=True):
                dpg.add_text("slope for:", tag="temperature.slope_label")
                for name, label, _ in SLOPE_GROUPS:
                    dpg.add_checkbox(label=label, tag=f"temperature.slope.{name}",
                                     default_value=name in DEFAULT_SLOPE_GROUPS)
                dpg.add_checkbox(label="hold outside range", tag="temperature.hold",
                                 default_value=False)
            dpg.add_text("", tag="temperature.info", color=GREY)
            with dpg.plot(tag="temperature.plot", width=-1, height=-1) as plot:
                dpg.bind_item_theme(plot, fit_padding_theme(_X_PADDING))
                dpg.add_plot_legend(horizontal=True)
                dpg.add_plot_axis(dpg.mvXAxis, label="temperature [C]", tag="temperature.x")
                with dpg.plot_axis(dpg.mvYAxis, label=PARAMETER_LABELS[0], tag="temperature.y"):
                    dpg.add_line_series([], [], label="in use", tag="temperature.in_use")
                    dpg.add_scatter_series([], [], label="its calibrations",
                                           tag="temperature.in_use_points")
                    dpg.add_scatter_series([], [], label="ticked records",
                                           tag="temperature.ticked")
                    dpg.add_scatter_series([], [], label="now", tag="temperature.now")
            dpg.bind_item_theme("temperature.in_use", line_theme(ORANGE))
            dpg.bind_item_theme("temperature.in_use_points", marker_theme(ORANGE, 7))
            dpg.bind_item_theme("temperature.ticked", marker_theme(CAL_COLOR))
            dpg.bind_item_theme("temperature.now", marker_theme(YELLOW, 8))

    def update(self, live):
        board, browser = self._board, self._browser
        index = PARAMETER_LABELS.index(dpg.get_value("temperature.parameter"))
        # The list and the ticks, not the selection: a click on a row keeps the zoom
        ticks = (tuple(browser.rows), tuple(sorted(browser.checked_rows)), index)
        ticks_changed = self._changed("ticks", ticks)
        if ticks_changed:
            self._draw_ticked(index)
        if self._changed("in_use", (board.calibration_version, board.temperature_now_c, index)) \
                or ticks_changed:
            self._draw_in_use(index)
        # Fit the axes for new data only, not as a compensated calibration follows the
        # temperature, so the plot can be zoomed
        calibration = board.calibration
        in_use_key = (None if calibration is None else
                      calibration.record.get("saved_at") if calibration.is_compensated else
                      board.calibration_version)
        if self._changed("fit", in_use_key) or ticks_changed:
            dpg.fit_axis_data("temperature.x")
            dpg.fit_axis_data("temperature.y")

    def _draw_ticked(self, index):
        """The ticked simple records as points, and how many are shown."""
        points, missing = self._browser.ticked_points()
        dpg.set_value("temperature.ticked", [[t for t, _ in points],
                                             [float(p[index]) for _, p in points]])
        dpg.configure_item("temperature.y", label=PARAMETER_LABELS[index])
        if points or missing:
            info = f"{len(points)} ticked records"
        else:
            info = "tick plain calibrations in Records to show them"
        if missing:
            info += f", {missing} not shown: compensated, without a temperature or unreadable"
        dpg.set_value("temperature.info", info)

    def _draw_in_use(self, index):
        """The calibration in use as a line in temperature (flat unless compensated),
        the calibrations a compensated one was fitted to, and where it is now."""
        board = self._board
        calibration = board.calibration
        if calibration is None:
            for tag in ("temperature.in_use", "temperature.in_use_points", "temperature.now"):
                dpg.set_value(tag, [[], []])
            return
        compensation = calibration.compensation
        dpg.set_value("temperature.in_use_points",
                      [list(compensation.temperatures_c), list(compensation.parameters[:, index])]
                      if compensation is not None else [[], []])

        now_c = calibration.temperature_c if compensation is not None else (
            board.temperature_now_c if board.temperature_now_c is not None
            else calibration.temperature_c)
        xs = [t for t, _ in self._browser.ticked_points()[0]]
        if compensation is not None:
            xs += list(compensation.temperatures_c)
        if now_c is not None:
            xs.append(now_c)
        low, high = (min(xs) - _LINE_MARGIN_C, max(xs) + _LINE_MARGIN_C) if xs \
            else _DEFAULT_RANGE_C
        value_now = float(model_to_parameters(calibration.model)[index])
        if compensation is not None:
            # The regression line, with a bend at each end of the range if it is held
            line_x = sorted({low, high, compensation.temperatures_c[0],
                             compensation.temperatures_c[-1]})
            line_y = [float(compensation.parameters_at(t)[index]) for t in line_x]
        else:
            line_x, line_y = [low, high], [value_now, value_now]
        dpg.set_value("temperature.in_use", [line_x, line_y])
        dpg.set_value("temperature.now", [[now_c], [value_now]] if now_c is not None
                      else [[], []])

    def _on_create_click(self):
        slope_groups = [name for name, _, _ in SLOPE_GROUPS
                        if dpg.get_value(f"temperature.slope.{name}")]
        self._browser.create_compensated(slope_groups, dpg.get_value("temperature.hold"))
