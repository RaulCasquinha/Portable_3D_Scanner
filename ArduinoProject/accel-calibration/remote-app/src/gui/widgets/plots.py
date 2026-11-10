"""
Plot helpers: coloured line series and a grid of plots sharing a time axis.

Layer: view.
"""

import dearpygui.dearpygui as dpg
import numpy as np

from ..style import fit_padding_theme, line_theme

__all__ = ["add_line", "set_xy", "clear_xy", "build_time_plots", "release_time_plots",
           "follow_time_plots"]

_NAN_LIST = [float("nan")]     # A series must hold at least one point


def add_line(axis, label, color):
    """A line series on a y axis, in this colour, starting empty."""
    series = dpg.add_line_series(_NAN_LIST, _NAN_LIST, label=label, parent=axis)
    dpg.bind_item_theme(series, line_theme(color))
    return series


def set_xy(series, x, y):
    """Replace the points of a series. Empty data shows nothing."""
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    if len(x) == 0:
        x, y = np.array([np.nan]), np.array([np.nan])
    dpg.set_value(series, [x.tolist(), y.tolist()])


def clear_xy(series):
    """Remove every point of a series."""
    set_xy(series, [], [])


def build_time_plots(tag_prefix, plots, rows, columns, unit, lines):
    """
    A grid of plots against time, with linked time axes.

    Args:
        tag_prefix: Each plot is tagged tag_prefix + its key.
        plots: (key, title) of each plot, in reading order.
        rows, columns: The grid.
        unit: The y axis label.
        lines: (label, color) of the lines of every plot; or a dict {key: lines}
            when the plots have different lines.

    Returns:
        {key: (time axis, [series of each line])}.
    """
    lines_of = lines if isinstance(lines, dict) else {key: lines for key, _ in plots}
    built = {}
    with dpg.subplots(rows, columns, width=-1, height=-1, link_all_x=True):
        for index, (key, title) in enumerate(plots):
            is_bottom = index >= (rows - 1) * columns     # The bottom row labels the time axis
            with dpg.plot(label=title, tag=f"{tag_prefix}{key}") as plot:
                dpg.bind_item_theme(plot, fit_padding_theme())
                dpg.add_plot_legend(horizontal=True)
                time_axis = dpg.add_plot_axis(dpg.mvXAxis, label="time [s]" if is_bottom else "",
                                              no_tick_labels=not is_bottom)
                value_axis = dpg.add_plot_axis(dpg.mvYAxis, label=unit, auto_fit=True)
                built[key] = (time_axis, [add_line(value_axis, label, color)
                                          for label, color in lines_of[key]])
    return built


def release_time_plots(plots):
    """
    Free the axes of time plots so they can be zoomed and panned: the time axis
    stops following the data and the value axis stops fitting it every frame.

    Args:
        plots: What build_time_plots() returned.
    """
    for time_axis, series in plots.values():
        value_axis = dpg.get_item_parent(series[0])
        dpg.set_axis_limits_auto(time_axis)
        dpg.configure_item(value_axis, auto_fit=False)


def follow_time_plots(plots):
    """Undo release_time_plots(): the value axes fit the data again. The caller sets
    the time axis limits each frame, as before."""
    for _, series in plots.values():
        dpg.configure_item(dpg.get_item_parent(series[0]), auto_fit=True)
