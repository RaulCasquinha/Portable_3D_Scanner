"""Live Info panel — fixed-width live data readout."""

from __future__ import annotations

import math

import dearpygui.dearpygui as dpg
import numpy as np


class DeviceView:
    """Fixed-width numeric readout for live scan data.

    Sections: Position, Orientation, Scan statistics.
    All number fields use a consistent character width so the layout
    does not shift as values change.
    """

    def build(self) -> None:
        """Create DearPyGUI items. Must be called inside a dpg.window() context."""
        with dpg.table(header_row=False,
                       policy=dpg.mvTable_SizingStretchProp,
                       width=-1):
            dpg.add_table_column(init_width_or_weight=0.50)
            dpg.add_table_column(init_width_or_weight=0.50)

            self._section("Position")
            self._row("X (mm)",    "_lv_x")
            self._row("Y (mm)",    "_lv_y")
            self._row("Z (mm)",    "_lv_z")

            self._spacer()
            self._section("Orientation")
            self._row("Pitch (°)", "_lv_pitch")
            self._row("Roll  (°)", "_lv_roll")
            self._row("Yaw   (°)", "_lv_yaw")

            self._spacer()
            self._section("Scan")
            self._row("Clusters",     "_lv_clusters")
            self._row("Last dist (mm)", "_lv_dist")

    # ------------------------------------------------------------------

    def _section(self, title: str) -> None:
        with dpg.table_row():
            dpg.add_text(title, color=(160, 160, 220, 255))
            dpg.add_text("")

    def _spacer(self) -> None:
        with dpg.table_row():
            dpg.add_text("")
            dpg.add_text("")

    def _row(self, label: str, tag: str) -> None:
        with dpg.table_row():
            dpg.add_text(f"  {label}")
            dpg.add_text("       ---", tag=tag)

    # ------------------------------------------------------------------
    # Update methods
    # ------------------------------------------------------------------

    def update_pose(self, pos: np.ndarray, orient: np.ndarray) -> None:
        """:param pos:    (3,) float — x, y, z in mm.
           :param orient: (3,) float — pitch, roll, yaw in radians."""
        dpg.set_value("_lv_x",     f"{pos[0]:>+9.1f}")
        dpg.set_value("_lv_y",     f"{pos[1]:>+9.1f}")
        dpg.set_value("_lv_z",     f"{pos[2]:>+9.1f}")
        dpg.set_value("_lv_pitch", f"{math.degrees(orient[0]):>+9.1f}")
        dpg.set_value("_lv_roll",  f"{math.degrees(orient[1]):>+9.1f}")
        dpg.set_value("_lv_yaw",   f"{math.degrees(orient[2]):>+9.1f}")

    def update_scan(self, clusters: int, last_dist_mm: float) -> None:
        dpg.set_value("_lv_clusters", f"{clusters:>9d}")
        dpg.set_value("_lv_dist",     f"{last_dist_mm:>9.1f}")
