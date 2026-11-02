"""Dedicated reconstruction control panel.

Exposes all Poisson + grouped-PCA parameters, a normal preview, and
Generate / STL actions.  Wired into MainWindow via callbacks.
"""

from __future__ import annotations

import time
from typing import Callable

import dearpygui.dearpygui as dpg
import numpy as np

from src.processing.reconstruction import (
    ReconParams, recommended_depth,
    compute_normals_preview, reconstruct_mesh, mesh_to_wireframe,
)
from src.gui.point_cloud_view import PointCloudView


class ReconstructionWindow:
    def __init__(self,
                 pv: PointCloudView,
                 get_active_cloud: Callable,
                 on_mesh_ready:    Callable,   # (o3d_mesh) → None
                 on_log:           Callable,   # (str)      → None
                 ) -> None:
        self._pv               = pv
        self._get_active_cloud = get_active_cloud
        self._on_mesh_ready    = on_mesh_ready
        self._on_log           = on_log
        self._auto_regen       = False
        self._last_regen       = 0.0
        self._regen_interval   = 5.0

    # ------------------------------------------------------------------
    # Build
    # ------------------------------------------------------------------

    def build(self) -> None:
        with dpg.group():
            # ── info row ──────────────────────────────────────────────
            with dpg.group(horizontal=True):
                dpg.add_text("Pts:", color=(140, 140, 140, 255))
                dpg.add_text("0", tag="_rw_pts")
                dpg.add_spacer(width=20)
                dpg.add_text("Rec. depth:", color=(140, 140, 140, 255))
                dpg.add_text("–", tag="_rw_rec_depth")

            dpg.add_separator()

            # ── Normal Estimation ──────────────────────────────────────
            dpg.add_text("Normal Estimation", color=(200, 200, 200, 255))
            dpg.add_spacer(height=2)

            with dpg.group(horizontal=True):
                dpg.add_text("Search radius", color=(140, 140, 140, 255))
            dpg.add_slider_float(tag="_rw_radius_pct", label="% bbox",
                                 min_value=0.5, max_value=20.0,
                                 default_value=10.0, width=180)

            with dpg.group(horizontal=True):
                dpg.add_text("Max neighbours", color=(140, 140, 140, 255))
            dpg.add_slider_int(tag="_rw_max_nn", label="pts",
                               min_value=3, max_value=100,
                               default_value=30, width=180)

            with dpg.group(horizontal=True):
                dpg.add_text("Direction filter", color=(140, 140, 140, 255))
            dpg.add_slider_float(tag="_rw_dir_filter", label="deg",
                                 min_value=0.0, max_value=180.0,
                                 default_value=45.0, width=180)
            dpg.add_text("  0° = off (use all neighbours)",
                         color=(110, 110, 110, 255))

            dpg.add_spacer(height=4)
            with dpg.group(horizontal=True):
                dpg.add_button(label="Preview Normals",
                               callback=self._on_preview_normals)
                dpg.add_checkbox(label="Show", tag="_rw_show_normals",
                                 default_value=False,
                                 callback=self._on_show_normals_toggle)
            with dpg.group(horizontal=True):
                dpg.add_text("Length", color=(140, 140, 140, 255))
                dpg.add_slider_int(tag="_rw_normal_len", label="mm",
                                   min_value=1, max_value=200,
                                   default_value=20, width=120,
                                   callback=self._on_normal_len_change)

            dpg.add_separator()

            # ── Poisson ───────────────────────────────────────────────
            dpg.add_text("Poisson", color=(200, 200, 200, 255))
            dpg.add_spacer(height=2)

            with dpg.group(horizontal=True):
                dpg.add_text("Depth", color=(140, 140, 140, 255))
            dpg.add_slider_int(tag="_rw_depth", label="(0 = auto)",
                               min_value=0, max_value=10,
                               default_value=0, width=180)

            with dpg.group(horizontal=True):
                dpg.add_text("Smoothing", color=(140, 140, 140, 255))
            dpg.add_slider_int(tag="_rw_smooth", label="iterations",
                               min_value=0, max_value=20,
                               default_value=5, width=180)

            dpg.add_separator()

            # ── Actions ───────────────────────────────────────────────
            with dpg.group(horizontal=True):
                dpg.add_button(label="Generate", tag="_rw_btn_gen",
                               callback=self.generate, enabled=False)
                dpg.add_checkbox(label="Auto", tag="_rw_chk_auto",
                                 default_value=False, enabled=False,
                                 callback=self._on_auto_toggle)
            with dpg.group(horizontal=True):
                dpg.add_checkbox(label="Show mesh", tag="_rw_chk_mesh",
                                 default_value=True, enabled=False,
                                 callback=lambda _s, v: self._pv.show_recon(v))
                dpg.add_button(label="→ STL", tag="_rw_btn_stl",
                               callback=self._on_stl, enabled=False)

    # ------------------------------------------------------------------
    # Public
    # ------------------------------------------------------------------

    def get_params(self) -> ReconParams:
        return ReconParams(
            depth         = dpg.get_value("_rw_depth"),
            radius_pct    = dpg.get_value("_rw_radius_pct"),
            max_nn        = dpg.get_value("_rw_max_nn"),
            dir_filter_deg= dpg.get_value("_rw_dir_filter"),
            smooth_iters  = dpg.get_value("_rw_smooth"),
        )

    def set_enabled(self, enabled: bool) -> None:
        dpg.configure_item("_rw_btn_gen",  enabled=enabled)
        dpg.configure_item("_rw_chk_auto", enabled=enabled)
        if not enabled:
            self._auto_regen = False
            dpg.set_value("_rw_chk_auto", False)

    def update_point_count(self, n: int, cloud=None) -> None:
        dpg.set_value("_rw_pts", str(n))
        if cloud is not None:
            rec = recommended_depth(cloud)
            dpg.set_value("_rw_rec_depth", str(rec))

    def tick(self) -> None:
        """Call every frame to handle auto-regen."""
        if (self._auto_regen
                and time.monotonic() - self._last_regen >= self._regen_interval):
            self.generate()

    def generate(self) -> None:
        cloud = self._get_active_cloud()
        if cloud is None:
            return
        params = self.get_params()
        depth_str = "auto" if params.depth == 0 else str(params.depth)
        self._on_log(f"[INFO] Running Poisson (depth={depth_str}, {len(cloud)} pts)...")
        mesh, err = reconstruct_mesh(cloud, params)
        if mesh is None:
            self._on_log(f"[ERROR] Reconstruction failed: {err}")
            return
        self._last_regen = time.monotonic()
        verts, edges = mesh_to_wireframe(mesh)
        self._pv.update_recon(verts, edges)
        self._pv.show_recon(True)
        dpg.set_value("_rw_chk_mesh", True)
        dpg.configure_item("_rw_chk_mesh", enabled=True)
        dpg.configure_item("_rw_btn_stl",  enabled=True)
        n = len(np.asarray(mesh.triangles))
        self._on_log(f"[INFO] Generated mesh: {n} triangles")
        self._on_mesh_ready(mesh)

    # ------------------------------------------------------------------
    # Callbacks
    # ------------------------------------------------------------------

    def _on_preview_normals(self) -> None:
        cloud = self._get_active_cloud()
        if cloud is None:
            self._on_log("[WARN] No active session")
            return
        params = self.get_params()
        self._on_log("[INFO] Computing normals preview...")
        pts, nrms, err = compute_normals_preview(cloud, params)
        if err:
            self._on_log(f"[ERROR] {err}")
            return
        length = float(dpg.get_value("_rw_normal_len"))
        self._pv.update_normals(pts, nrms, length)
        self._pv.show_normals(True)
        dpg.set_value("_rw_show_normals", True)
        self._on_log(f"[INFO] Normals preview: {len(pts)} vectors")

    def _on_show_normals_toggle(self, _s, value: bool) -> None:
        self._pv.show_normals(value)

    def _on_normal_len_change(self, _s, value: int) -> None:
        self._pv._normals_len = float(value)
        if self._pv._normals_visible:
            self._pv._refresh()

    def _on_auto_toggle(self, _s, value: bool) -> None:
        self._auto_regen = value

    def _on_stl(self) -> None:
        dpg.show_item("_mw_stl_dlg")
