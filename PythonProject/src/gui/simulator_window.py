"""Simulator control window — loads an STL model and drives the SimSession."""

from __future__ import annotations

import math
from typing import Callable

import dearpygui.dearpygui as dpg

from src.sim.sim_session import SimSession
from src.gui.point_cloud_view import PointCloudView


def _label(text: str) -> None:
    """Dim section-label text above a control."""
    dpg.add_text(text, color=(170, 170, 170, 200))


def _slider_f(tag: str, label: str, default: float,
              lo: float, hi: float, fmt: str = "%.1f",
              callback=None) -> None:
    """Labelled float slider: text above, full-width slider below."""
    _label(label)
    dpg.add_slider_float(tag=tag, label=f"##{tag}",
                         default_value=default,
                         min_value=lo, max_value=hi,
                         format=fmt, width=-1,
                         callback=callback)


class SimulatorWindow:
    """Dockable panel that controls the simulator session."""

    def __init__(self, sim: SimSession, pv: PointCloudView,
                 on_start: Callable, on_stop: Callable) -> None:
        self._sim      = sim
        self._pv       = pv
        self._on_start = on_start
        self._on_stop  = on_stop
        self._running  = False

        self._sim.on_device_move = self._on_device_move

    def build(self) -> None:
        """Create DearPyGUI items.  Must be called inside a dpg.window() context."""

        # ── Model ──────────────────────────────────────────────────────
        dpg.add_text("Model", color=(160, 160, 220, 255))
        with dpg.group(horizontal=True):
            dpg.add_button(label="Load STL / OBJ…",
                           callback=self._open_file_dialog)
            dpg.add_text("No model loaded", tag="_sw_model_label")
        dpg.add_checkbox(label="Show model", tag="_sw_show_model",
                         default_value=True, callback=self._on_show_model)
        dpg.add_separator()

        # ── Noise ──────────────────────────────────────────────────────
        dpg.add_text("Noise", color=(160, 160, 220, 255))
        _slider_f("_sw_dist_noise",   "Distance noise (mm)",    2.0,  0.0, 20.0,
                  callback=self._on_noise_change)
        _slider_f("_sw_orient_noise", "Orientation noise (°)",  0.5,  0.0,  5.0,
                  callback=self._on_noise_change)
        dpg.add_separator()

        # ── Device position ────────────────────────────────────────────
        dpg.add_text("Device position", color=(160, 160, 220, 255))
        _slider_f("_sw_azimuth",   "Azimuth (°)",      0.0, -180.0, 180.0,
                  callback=self._on_pose_change)
        _slider_f("_sw_elevation", "Elevation (°)",   math.degrees(0.2), -80.0, 80.0,
                  callback=self._on_pose_change)
        _slider_f("_sw_distance",  "Distance (mm)",  500.0,   50.0, 5000.0, "%.0f",
                  callback=self._on_pose_change)
        dpg.add_separator()

        # ── Auto orbit ─────────────────────────────────────────────────
        dpg.add_text("Auto orbit", color=(160, 160, 220, 255))
        dpg.add_checkbox(label="Enable", tag="_sw_auto_orbit",
                         default_value=False, callback=self._on_orbit_change)
        _slider_f("_sw_orbit_speed", "Azimuth speed (°/s)",   15.0,  1.0, 90.0,
                  callback=self._on_orbit_change)
        _slider_f("_sw_elev_speed",  "Elevation speed (°/s)",  5.0,  0.5, 30.0,
                  callback=self._on_orbit_change)
        _slider_f("_sw_elev_min", "Elevation min (°)", -70.0, -80.0, 0.0, "%.0f°",
                  callback=self._on_elev_range_change)
        _slider_f("_sw_elev_max", "Elevation max (°)",  70.0,   0.0, 80.0, "%.0f°",
                  callback=self._on_elev_range_change)
        dpg.add_separator()

        # ── Fast-forward ───────────────────────────────────────────────
        dpg.add_text("Simulation speed", color=(160, 160, 220, 255))
        _slider_f("_sw_time_scale", "Fast-forward (×)",  1.0,  1.0, 500.0, "%.0f×",
                  callback=self._on_time_scale_change)
        dpg.add_separator()

        # ── Start / Stop ───────────────────────────────────────────────
        dpg.add_button(label="Start Simulator", tag="_sw_btn_run",
                       width=-1, callback=self._on_toggle)

        # File dialog
        with dpg.file_dialog(tag="_sw_file_dlg",
                             label="Load 3D Model",
                             directory_selector=False,
                             show=False,
                             modal=True,
                             width=620, height=420,
                             callback=self._on_file_selected):
            dpg.add_file_extension(".stl", color=(100, 220, 100, 255),
                                   custom_text="STL")
            dpg.add_file_extension(".obj", color=(100, 180, 220, 255),
                                   custom_text="OBJ")
            dpg.add_file_extension(".*")

    # ------------------------------------------------------------------
    # Callbacks
    # ------------------------------------------------------------------

    def _open_file_dialog(self) -> None:
        dpg.show_item("_sw_file_dlg")

    def _on_file_selected(self, _s, app_data) -> None:
        path = app_data.get("file_path_name", "")
        if not path:
            return
        ok, err = self._sim.load_model(path)
        if ok:
            dpg.set_value("_sw_model_label", self._sim.mesh_filename)
            dpg.set_value("_sw_distance", self._sim.distance)
            data = self._sim.mesh_edges()
            if data:
                verts, edges = data
                self._pv.update_mesh(verts, edges)
            self._pv.show_mesh(dpg.get_value("_sw_show_model"))
        else:
            dpg.set_value("_sw_model_label", f"Error: {err}")

    def _on_show_model(self, _s, value) -> None:
        self._pv.show_mesh(value)

    def _on_noise_change(self, _s=None, _a=None) -> None:
        self._sim.set_noise(
            dpg.get_value("_sw_dist_noise"),
            dpg.get_value("_sw_orient_noise"),
        )

    def _on_pose_change(self, _s=None, _a=None) -> None:
        if dpg.get_value("_sw_auto_orbit"):
            return
        self._sim.set_device_pose(
            math.radians(dpg.get_value("_sw_azimuth")),
            math.radians(dpg.get_value("_sw_elevation")),
            dpg.get_value("_sw_distance"),
        )

    def _on_orbit_change(self, _s=None, _a=None) -> None:
        enabled = dpg.get_value("_sw_auto_orbit")
        self._sim.set_auto_orbit(
            enabled,
            dpg.get_value("_sw_orbit_speed"),
            dpg.get_value("_sw_elev_speed"),
        )
        dpg.configure_item("_sw_azimuth",   enabled=not enabled)
        dpg.configure_item("_sw_elevation", enabled=not enabled)

    def _on_elev_range_change(self, _s=None, _a=None) -> None:
        lo = dpg.get_value("_sw_elev_min")
        hi = dpg.get_value("_sw_elev_max")
        # Keep min < max
        if lo >= hi:
            lo = hi - 1.0
            dpg.set_value("_sw_elev_min", lo)
        self._sim.set_elev_range(lo, hi)

    def _on_time_scale_change(self, _s=None, _a=None) -> None:
        self._sim.set_time_scale(dpg.get_value("_sw_time_scale"))

    def _on_toggle(self) -> None:
        if self._running:
            self._running = False
            self._sim.stop()
            dpg.configure_item("_sw_btn_run", label="Start Simulator")
            self._on_stop()
        else:
            self._apply_all_settings()
            self._running = True
            self._sim.start()
            dpg.configure_item("_sw_btn_run", label="Stop Simulator")
            self._on_start()

    def _on_device_move(self, az: float, el: float, dist: float) -> None:
        pass   # sliders not synced during orbit to avoid feedback loops

    def _apply_all_settings(self) -> None:
        self._on_noise_change()
        self._on_pose_change()
        self._on_orbit_change()
        self._on_elev_range_change()
        self._on_time_scale_change()
