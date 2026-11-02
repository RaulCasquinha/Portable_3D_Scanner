"""Main window — dockable panels wired to the ScanSession or SimSession.

DearPyGUI docking is enabled via configure_app(docking=True).  Each panel
(toolbar, 3-D view, live info, log, simulator) is an independent window that
the user can freely dock, undock, and resize.  The layout is persisted to
``layout.ini`` next to main.py and restored on the next launch.

Only one backend session (real or simulator) is active at a time.  Connecting
the real device disables the simulator start button and vice versa.
"""

from __future__ import annotations

import os
import numpy as np
import dearpygui.dearpygui as dpg

from src.app.scan_session import ScanSession
from src.sim.sim_session import SimSession
from src.gui.point_cloud_view import PointCloudView
from src.gui.device_view import DeviceView
from src.gui.log_view import LogView
from src.gui.simulator_window import SimulatorWindow
from src.gui.reconstruction_window import ReconstructionWindow

_LAYOUT_FILE = "layout.ini"


class MainWindow:
    """Builds all dockable panels and wires them to the active session."""

    def __init__(self, session: ScanSession, sim_session: SimSession) -> None:
        self._session     = session
        self._sim_session = sim_session
        self._pv = PointCloudView()
        self._dv = DeviceView()
        self._lv = LogView()
        self._sim_win = SimulatorWindow(
            sim_session, self._pv,
            on_start=self._on_sim_start,
            on_stop=self._on_sim_stop,
        )
        self._o3d_mesh       = None
        self._active_session = None
        self._recon_win      = ReconstructionWindow(
            pv               = self._pv,
            get_active_cloud = lambda: self._active_session._cloud if self._active_session else None,
            on_mesh_ready    = self._on_mesh_ready,
            on_log           = self._lv.append,
        )

    def build(self) -> None:
        """Create all DearPyGUI items.  Must be called after create_context()."""
        self._build_toolbar()
        self._build_cloud_window()
        self._build_device_window()
        self._build_log_window()
        self._build_sim_window()
        self._build_recon_window()
        self._build_save_dialog()
        self._pv.on_focus_change = self._on_focus_change

    def tick(self) -> None:
        """Call every frame to drive auto-regen in the reconstruction window."""
        self._recon_win.tick()

    # ------------------------------------------------------------------
    # Panel builders
    # ------------------------------------------------------------------

    def _build_toolbar(self) -> None:
        with dpg.window(
            tag="_win_toolbar",
            label="Controls",
            width=1200, height=86,
            pos=(0, 0),
            no_scrollbar=True,
            no_scroll_with_mouse=True,
        ):
            # ── Row 1: connection + status ─────────────────────────────
            with dpg.group(horizontal=True):
                dpg.add_text("Port:")
                dpg.add_combo(self._session.available_ports(),
                              tag="_mw_port", width=160)
                dpg.add_button(label="Refresh", width=60,
                               callback=self._refresh_ports)
                dpg.add_spacer(width=4)
                dpg.add_button(label="Connect",    callback=self._on_connect)
                dpg.add_button(label="Disconnect", tag="_mw_btn_disc",
                               callback=self._on_disconnect, enabled=False)
                dpg.add_spacer(width=4)
                dpg.add_button(label="Simulator", tag="_mw_btn_sim",
                               callback=self._on_show_sim)
                dpg.add_spacer(width=20)
                dpg.add_text("[x]", tag="_mw_dot", color=(80, 80, 80, 255))
                dpg.add_text("Disconnected", tag="_mw_status")
                dpg.add_spacer(width=20)
                dpg.add_text("Points: 0", tag="_mw_pts")
                dpg.add_spacer(width=20)
                dpg.add_text("LiDAR:", color=(140, 140, 140, 255))
                dpg.add_text("?", tag="_mw_lidar", color=(140, 140, 140, 255))
                dpg.add_spacer(width=10)
                dpg.add_text("IMU:", color=(140, 140, 140, 255))
                dpg.add_text("?", tag="_mw_imu", color=(140, 140, 140, 255))

            # ── Row 2: scan ops + view ─────────────────────────────────
            with dpg.group(horizontal=True):
                dpg.add_button(label="Reset Reference", tag="_mw_btn_reset",
                               callback=self._on_reset_reference, enabled=False)
                dpg.add_button(label="Stop", tag="_mw_btn_rec",
                               callback=self._on_record_toggle, enabled=False)
                dpg.add_button(label="Save PLY", tag="_mw_btn_save",
                               callback=self._on_save, enabled=False)
                dpg.add_spacer(width=4)
                dpg.add_button(label="Reconstruction",
                               callback=lambda: dpg.configure_item("_win_recon", show=True))
                dpg.add_spacer(width=10)
                dpg.add_text("Pt:", color=(140, 140, 140, 255))
                dpg.add_slider_int(tag="_mw_pt_size",
                                   min_value=1, max_value=20,
                                   default_value=3, width=80,
                                   callback=lambda _s, v: self._pv.set_point_size(v))
                dpg.add_spacer(width=6)
                dpg.add_checkbox(label="Cloud", tag="_mw_show_cloud",
                                 default_value=True,
                                 callback=lambda _s, v: self._pv.show_cloud(v))
                dpg.add_checkbox(label="Rays", tag="_mw_show_rays",
                                 default_value=True,
                                 callback=lambda _s, v: self._pv.show_rays(v))
                dpg.add_spacer(width=10)
                dpg.add_text("Ctr:", color=(140, 140, 140, 255))
                dpg.add_drag_float(tag="_mw_cx", label="##cx", width=60,
                                   speed=1.0, format="%.0f",
                                   callback=self._on_center_drag)
                dpg.add_drag_float(tag="_mw_cy", label="##cy", width=60,
                                   speed=1.0, format="%.0f",
                                   callback=self._on_center_drag)
                dpg.add_drag_float(tag="_mw_cz", label="##cz", width=60,
                                   speed=1.0, format="%.0f",
                                   callback=self._on_center_drag)
                dpg.add_button(label="0", width=22,
                               callback=self._on_center_reset)

    def _build_cloud_window(self) -> None:
        with dpg.window(
            tag="_win_cloud",
            label="3D Point Cloud",
            width=820, height=610,
            pos=(0, 90),
        ):
            self._pv.build()

    def _build_device_window(self) -> None:
        with dpg.window(
            tag="_win_device",
            label="Live Info",
            width=360, height=270,
            pos=(825, 90),
        ):
            self._dv.build()

    def _build_log_window(self) -> None:
        with dpg.window(
            tag="_win_log",
            label="Log",
            width=360, height=350,
            pos=(825, 365),
        ):
            self._lv.build()

    def _build_sim_window(self) -> None:
        with dpg.window(
            tag="_win_sim",
            label="Simulator",
            width=360, height=420,
            pos=(825, 60),
            show=False,
        ):
            self._sim_win.build()

    def _build_recon_window(self) -> None:
        with dpg.window(
            tag="_win_recon",
            label="Reconstruction",
            width=320, height=540,
            pos=(825, 60),
            show=False,
        ):
            self._recon_win.build()

    def _build_save_dialog(self) -> None:
        with dpg.file_dialog(
            tag="_mw_save_dlg",
            label="Save Point Cloud",
            directory_selector=False,
            show=False,
            modal=True,
            width=600, height=400,
            default_filename="scan",
            default_path=os.path.expanduser("~"),
            callback=self._on_save_dialog,
        ):
            dpg.add_file_extension(".ply", color=(100, 220, 100, 255),
                                   custom_text="PLY")
            dpg.add_file_extension(".*")

        with dpg.file_dialog(
            tag="_mw_stl_dlg",
            label="Save STL",
            directory_selector=False,
            show=False,
            modal=True,
            width=600, height=400,
            default_filename="model",
            default_path=os.path.expanduser("~"),
            callback=self._on_stl_dialog,
        ):
            dpg.add_file_extension(".stl", color=(100, 200, 255, 255),
                                   custom_text="STL")
            dpg.add_file_extension(".*")

    # ------------------------------------------------------------------
    # Session wiring
    # ------------------------------------------------------------------

    def _wire_session(self, session) -> None:
        session.on_cloud_update  = self._on_cloud
        session.on_pose_update   = self._on_pose
        session.on_ray_update    = self._pv.update_rays
        session.on_log           = self._lv.append
        session.on_status        = lambda msg: dpg.set_value("_mw_status", msg)
        session.on_sensor_status = self._on_sensor_status

    def _unwire_session(self, session) -> None:
        session.on_cloud_update  = None
        session.on_pose_update   = None
        session.on_ray_update    = None
        session.on_log           = None
        session.on_status        = None
        session.on_sensor_status = None

    # ------------------------------------------------------------------
    # Toolbar callbacks
    # ------------------------------------------------------------------

    def _on_connect(self) -> None:
        port = dpg.get_value("_mw_port")
        if not port:
            return
        self._active_session = self._session
        self._wire_session(self._session)
        self._session.start(port)
        self._session.start_recording()
        dpg.configure_item("_mw_btn_disc",  enabled=True)
        dpg.configure_item("_mw_btn_reset", enabled=True)
        dpg.configure_item("_mw_btn_rec",   enabled=True, label="Stop")
        dpg.configure_item("_mw_btn_save",  enabled=True)
        dpg.configure_item("_mw_btn_sim",   enabled=False)
        dpg.configure_item("_mw_dot", color=(60, 200, 60, 255))
        dpg.set_value("_mw_dot", "[o]")
        self._recon_win.set_enabled(True)

    def _on_disconnect(self) -> None:
        self._session.stop()
        self._unwire_session(self._session)
        self._active_session = None
        dpg.configure_item("_mw_btn_disc",  enabled=False)
        dpg.configure_item("_mw_btn_reset", enabled=False)
        dpg.configure_item("_mw_btn_rec",   enabled=False, label="Stop")
        dpg.configure_item("_mw_btn_save",  enabled=False)
        dpg.configure_item("_mw_btn_sim",   enabled=True)
        dpg.configure_item("_mw_dot", color=(80, 80, 80, 255))
        dpg.set_value("_mw_dot", "[x]")
        self._recon_win.set_enabled(False)

    def _on_show_sim(self) -> None:
        dpg.configure_item("_win_sim", show=True)

    def _on_sim_start(self) -> None:
        """Called by SimulatorWindow when simulation starts."""
        self._active_session = self._sim_session
        self._wire_session(self._sim_session)
        dpg.configure_item("_mw_btn_reset", enabled=True)
        dpg.configure_item("_mw_btn_rec",   enabled=True, label="Stop")
        dpg.configure_item("_mw_btn_save",  enabled=True)
        dpg.configure_item("_mw_btn_disc",  enabled=False)
        dpg.configure_item("_mw_port",      enabled=False)
        dpg.configure_item("_mw_dot", color=(200, 160, 40, 255))
        dpg.set_value("_mw_dot", "[s]")
        self._recon_win.set_enabled(True)

    def _on_sim_stop(self) -> None:
        """Called by SimulatorWindow when simulation stops."""
        self._unwire_session(self._sim_session)
        self._active_session = None
        dpg.configure_item("_mw_btn_reset", enabled=False)
        dpg.configure_item("_mw_btn_rec",   enabled=False, label="Stop")
        dpg.configure_item("_mw_btn_save",  enabled=False)
        dpg.configure_item("_mw_port",      enabled=True)
        dpg.configure_item("_mw_dot", color=(80, 80, 80, 255))
        dpg.set_value("_mw_dot", "[x]")
        self._recon_win.set_enabled(False)

    def _on_reset_reference(self) -> None:
        if self._active_session:
            self._active_session.reset_reference()

    def _on_record_toggle(self) -> None:
        s = self._active_session
        if s is None:
            return
        if s.is_recording:
            s.stop_recording()
            dpg.configure_item("_mw_btn_rec", label="Start")
        else:
            s.start_recording()
            dpg.configure_item("_mw_btn_rec", label="Stop")

    def _on_save(self) -> None:
        dpg.show_item("_mw_save_dlg")

    def _on_save_dialog(self, _sender, app_data) -> None:
        path = app_data.get("file_path_name", "")
        if not path:
            return
        if not path.endswith(".ply"):
            path += ".ply"
        if self._active_session:
            self._active_session.save_cloud(path)

    def _refresh_ports(self) -> None:
        dpg.configure_item("_mw_port", items=self._session.available_ports())

    # ------------------------------------------------------------------
    # Session callbacks
    # ------------------------------------------------------------------

    def _on_sensor_status(self, lidar_ok: bool, imu_ok: bool) -> None:
        def _update(tag: str, ok: bool) -> None:
            dpg.set_value(tag, "OK" if ok else "None")
            dpg.configure_item(tag,
                               color=(60, 200, 60, 255) if ok else (200, 80, 80, 255))
        _update("_mw_lidar", lidar_ok)
        _update("_mw_imu",   imu_ok)

    def _on_cloud(self, pts: np.ndarray) -> None:
        self._pv.update_cloud(pts)
        s = self._active_session
        if s is not None:
            n = s.point_count
            dpg.set_value("_mw_pts", f"Points: {n}")
            self._dv.update_scan(n, s.last_dist_mm)
            self._recon_win.update_point_count(n, s._cloud)

    def _on_pose(self, pos: np.ndarray, orient: np.ndarray) -> None:
        self._pv.update_device(pos, orient)
        self._dv.update_pose(pos, orient)

    def _on_generate(self) -> None:
        self._recon_win.generate()

    def _on_mesh_ready(self, mesh) -> None:
        self._o3d_mesh = mesh

    def _on_stl(self) -> None:
        if self._o3d_mesh is not None:
            dpg.show_item("_mw_stl_dlg")

    def _on_stl_dialog(self, _s, app_data) -> None:
        from src.processing.reconstruction import save_mesh_stl
        path = app_data.get("file_path_name", "")
        if not path:
            return
        if not path.endswith(".stl"):
            path += ".stl"
        ok, msg = save_mesh_stl(self._o3d_mesh, path)
        prefix = "[INFO]" if ok else "[ERROR]"
        self._lv.append(f"{prefix} {msg}")

    def _on_focus_change(self, x: float, y: float, z: float) -> None:
        dpg.set_value("_mw_cx", x)
        dpg.set_value("_mw_cy", y)
        dpg.set_value("_mw_cz", z)

    def _on_center_drag(self) -> None:
        self._pv.set_focus(
            dpg.get_value("_mw_cx"),
            dpg.get_value("_mw_cy"),
            dpg.get_value("_mw_cz"),
        )

    def _on_center_reset(self) -> None:
        dpg.set_value("_mw_cx", 0.0)
        dpg.set_value("_mw_cy", 0.0)
        dpg.set_value("_mw_cz", 0.0)
        self._pv.set_focus(0.0, 0.0, 0.0)
