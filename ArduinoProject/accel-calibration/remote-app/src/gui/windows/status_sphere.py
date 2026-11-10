"""
Status sphere window: the captured positions as gravity directions on a sphere,
where the board points now, and whether it would accept that position. Its Shading
section sets how depth is shown (the tint of the sphere, its highlight, and the
rings, outlines and dashes of what is behind); the presets save it.

Layer: view.
"""

import dearpygui.dearpygui as dpg
import numpy as np

from ...processing.geometry import unit
from ..style import AXIS_COLORS, CAPTURED, GREEN, GREY, ORANGE, RED, YELLOW
from ..widgets.orbit3d import DEFAULT_HIGHLIGHT, DEFAULT_TINT, Orbit3D
from .base import Window

__all__ = ["StatusSphereWindow"]

_SPHERE_SIZE = 290           # Starting size; the sphere then fills the window
_MIN_SPHERE_SIZE = 80
_FAINT = 70                  # Alpha of the sphere's circles
_AXIS_ALPHA = 90          # Faint: the axis lines in their colours, x y z


class StatusSphereWindow(Window):
    """The sphere of positions."""

    TAG = "win_calib"
    TITLE = "Status sphere"     # TAG kept from when it was "Calibration": layouts use it
    SPHERE_TAG = "sphere.view"

    def __init__(self, window_id, board, display):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board.
            display: The DisplaySettingsWindow, for the smoothing and the source.
        """
        super().__init__(window_id)
        self._board = board
        self._display = display
        with self.window() as window:
            # How depth is shown; closed by default so the sphere keeps the room. Above
            # the sphere: the sphere fills the space left under its last item
            with dpg.collapsing_header(label="Shading", tag="sphere.shading"):
                dpg.add_slider_int(label="tint", tag="sphere.tint",
                                   default_value=round(DEFAULT_TINT * 100), min_value=0,
                                   max_value=100, format="%d %%", width=150, clamped=True)
                dpg.add_slider_int(label="highlight", tag="sphere.highlight",
                                   default_value=round(DEFAULT_HIGHLIGHT * 100), min_value=0,
                                   max_value=100, format="%d %%", width=150, clamped=True)
                dpg.add_checkbox(label="mark what is behind", tag="sphere.marks_behind",
                                 default_value=True)
            self._sphere = Orbit3D(window, _SPHERE_SIZE, _SPHERE_SIZE)
            dpg.add_text("green: captured  orange: reference\narrow: green = d accepts, "
                         "red = rotate\nsolid: towards you  ring, outline, dashed: away",
                         color=GREY)
            dpg.add_text("", tag="sphere.placement", wrap=0)   # 0: wrap at the window edge
        dpg.add_alias(self.SPHERE_TAG, self._sphere.tag)

    def update(self, live):
        self._sphere.handle_mouse()
        if self._fit_sphere():
            self.update_slow()

    def _fit_sphere(self):
        """
        Make the sphere fill the window: all its width, and the height left under the
        texts below it (the space after the last item, from the previous frame).

        Returns:
            True if the sphere changed size.
        """
        free = dpg.get_item_state("sphere.placement").get("content_region_avail")
        if not free:
            return False
        height = self._sphere.size[1]
        return self._sphere.resize(max(_MIN_SPHERE_SIZE, free[0]),
                                   max(_MIN_SPHERE_SIZE, height + free[1]))

    def update_slow(self):
        """Redraw the sphere; a few times a second is enough."""
        board, capture = self._board, self._board.capture
        sphere = self._sphere
        sphere.set_shading(dpg.get_value("sphere.tint") / 100,
                           dpg.get_value("sphere.highlight") / 100,
                           dpg.get_value("sphere.marks_behind"))
        sphere.clear()
        for axis in range(3):
            sphere.circle(axis, (*GREY, _FAINT))
        sphere.axes(AXIS_COLORS, _AXIS_ALPHA)
        for position in capture.positions:
            sphere.point(unit(position), CAPTURED)
        for reference in capture.references.values():
            sphere.point(unit(reference), ORANGE)

        smoothing_s = self._display.smoothing_s()
        check = capture.check_placement(board.recent_raw(smoothing_s)) if len(board.buffer) \
            else None
        color = YELLOW if check is None else GREEN if check.is_ok else RED
        corrected = board.recent_corrected(smoothing_s, self._display.cal_source(),
                                           self._display.applied_terms())
        sphere.arrow(np.zeros(3), unit(corrected), color, thickness=2.5)
        dpg.set_value("sphere.placement", _placement_text(check))
        dpg.configure_item("sphere.placement", color=color)


def _placement_text(check):
    """What d would do now, for the user; "" when no capture is running."""
    if check is None:
        return ""
    if check.target == "first":
        return "first position: any orientation"
    if check.target == "position":
        return (f"nearest captured position {check.angle_deg:5.1f} deg "
                f"(min {check.limit_deg:4.1f}): "
                f"{'press d' if check.is_ok else 'rotate further'}")
    return (f"reference +{check.target} up: {check.angle_deg:5.1f} deg off "
            f"(max {check.limit_deg:4.1f}): "
            f"{'press d' if check.is_ok else f'turn +{check.target} up'}")
