"""Interactive 3D point cloud view — fills its parent window.

Uses a perspective projection onto a DearPyGUI scatter/line plot.
The actual plot pixel dimensions are read each frame so the view
scales correctly when the window is resized or re-docked.

Controls
--------
- Left drag      : orbit (azimuth / elevation)
- Shift+left drag: pan   (translate focus point)
- Scroll wheel   : zoom  (change camera distance)
"""

from __future__ import annotations

import math
import time

import dearpygui.dearpygui as dpg
import numpy as np

_AXIS_LEN = 50.0
_FALLBACK_W, _FALLBACK_H = 800, 600
_GRID_STEP = 100.0   # mm — ground plane grid spacing
_GRID_N    = 5       # lines each side → ±500 mm

# Depth-colour buckets: bucket 0 = farthest (cool blue), 7 = nearest (warm red)
_N_DEPTH = 8

# Maximum points projected each frame — display cap keeps the UI responsive
# regardless of how many clusters have accumulated.
_MAX_DISPLAY_PTS = 6000

# Minimum seconds between data-triggered redraws (camera events bypass this).
_DATA_REFRESH_INTERVAL = 1.0 / 30.0   # 30 fps cap
_DEPTH_COLORS: list[tuple[int, int, int]] = [
    ( 60,  90, 200),   # 0 far  — deep blue
    ( 70, 150, 220),   # 1
    ( 80, 200, 220),   # 2      — cyan
    (100, 210, 150),   # 3      — teal-green
    (170, 220,  90),   # 4      — yellow-green
    (230, 200,  60),   # 5      — yellow
    (240, 130,  50),   # 6      — orange
    (250,  70,  50),   # 7 near — red
]


class PointCloudView:
    """3D orbit-camera scatter plot that fills its parent window."""

    def __init__(self) -> None:
        self._azimuth   = 0.8
        self._elevation = 0.4
        self._distance  = 800.0
        self._focus     = np.zeros(3, dtype=np.float64)   # pan target
        self._dragging    = False
        self._last_drag_x = 0.0
        self._last_drag_y = 0.0
        self._point_size: int = 3

        # Fired with (x, y, z) whenever _focus changes (pan or set_focus)
        self.on_focus_change = None

        self._cloud: np.ndarray = np.empty((0, 3), dtype=np.float32)
        self._ray_origins:    np.ndarray = np.empty((0, 3), dtype=np.float32)
        self._ray_endpoints:  np.ndarray = np.empty((0, 3), dtype=np.float32)
        self._device_pos    = np.zeros(3, dtype=np.float32)
        self._device_orient = np.zeros(3, dtype=np.float32)

        self._mesh_edges: np.ndarray | None = None   # imported model wireframe
        self._mesh_visible: bool = True
        self._recon_edges: np.ndarray | None = None  # Poisson reconstruction wireframe
        self._recon_visible: bool = True
        self._normals_pts:  np.ndarray = np.empty((0, 3), dtype=np.float32)
        self._normals_dirs: np.ndarray = np.empty((0, 3), dtype=np.float32)
        self._normals_len:  float = 20.0
        self._normals_visible: bool = False

        # Projected cloud kept for hover hit-test (display subset only)
        self._proj_cloud: np.ndarray = np.empty((0, 2), dtype=np.float32)
        self._proj_valid: np.ndarray = np.empty(0, dtype=bool)
        self._display_idx: np.ndarray = np.empty(0, dtype=np.int32)  # indices into _cloud

        # Throttle for data-driven refreshes
        self._last_refresh: float = 0.0
        self._grid_pts3d: np.ndarray | None = None  # cached grid 3-D points

    def build(self) -> None:
        """Create all DearPyGUI items. Must be called inside a dpg.window() context."""
        with dpg.plot(tag="_pcv_plot", width=-1, height=-1,
                      no_title=True, no_mouse_pos=True,
                      pan_button=dpg.mvMouseButton_Middle,
                      no_box_select=True):
            dpg.add_plot_axis(dpg.mvXAxis, tag="_pcv_xax",
                              no_gridlines=True, no_tick_labels=True,
                              no_tick_marks=True)
            with dpg.plot_axis(dpg.mvYAxis, tag="_pcv_yax",
                               no_gridlines=True, no_tick_labels=True,
                               no_tick_marks=True):
                dpg.add_line_series([], [], tag="_pcv_grid",
                                    label="Grid", parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_mesh",
                                    label="Model", parent="_pcv_yax", show=True)
                dpg.add_line_series([], [], tag="_pcv_recon",
                                    label="Recon", parent="_pcv_yax", show=True)
                dpg.add_line_series([], [], tag="_pcv_normals",
                                    label="Normals", parent="_pcv_yax", show=False)
                dpg.add_line_series([], [], tag="_pcv_ax_x", label="X",
                                    parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_ax_y", label="Y",
                                    parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_ax_z", label="Z",
                                    parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_rays",
                                    label="Rays", parent="_pcv_yax")
                # One scatter series per depth bucket
                for b in range(_N_DEPTH):
                    dpg.add_scatter_series([], [], tag=f"_pcv_cloud_{b}",
                                           label=f"Cloud{b}", parent="_pcv_yax")
                dpg.add_scatter_series([], [], tag="_pcv_dev",
                                       label="Device", parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_dev_ax1",
                                    parent="_pcv_yax")
                dpg.add_line_series([], [], tag="_pcv_dev_ax2",
                                    parent="_pcv_yax")
            dpg.add_plot_annotation(tag="_pcv_annot", label="",
                                    default_value=(0.0, 0.0),
                                    color=(255, 255, 100, 220),
                                    clamped=True, offset=(10, -10),
                                    show=False)

        self._apply_themes()

        with dpg.handler_registry(tag="_pcv_handlers"):
            dpg.add_mouse_click_handler(button=0,   callback=self._on_press)
            dpg.add_mouse_release_handler(button=0, callback=self._on_release)
            dpg.add_mouse_drag_handler(button=0, threshold=0, callback=self._on_drag)
            dpg.add_mouse_wheel_handler(callback=self._on_wheel)
            dpg.add_mouse_move_handler(callback=self._on_move)

        self._refresh()

    # ------------------------------------------------------------------
    # Public update API
    # ------------------------------------------------------------------

    def update_cloud(self, pts: np.ndarray) -> None:
        self._cloud = pts
        self._refresh_data()

    def update_rays(self, origins: np.ndarray, endpoints: np.ndarray) -> None:
        self._ray_origins   = origins
        self._ray_endpoints = endpoints
        self._refresh_data()

    def update_device(self, pos: np.ndarray, orient: np.ndarray) -> None:
        self._device_pos    = pos.astype(np.float32)
        self._device_orient = orient.astype(np.float32)
        self._refresh_data()

    def _refresh_data(self) -> None:
        """Throttled refresh for data callbacks — caps at _DATA_REFRESH_INTERVAL."""
        now = time.monotonic()
        if now - self._last_refresh >= _DATA_REFRESH_INTERVAL:
            self._last_refresh = now
            self._refresh()

    def update_mesh(self, vertices: np.ndarray,
                    edge_indices: np.ndarray) -> None:
        v = vertices.astype(np.float32)
        self._mesh_edges = np.stack(
            [v[edge_indices[:, 0]], v[edge_indices[:, 1]]], axis=1
        )  # (E, 2, 3)
        self._refresh()

    def update_recon(self, vertices: np.ndarray, edge_indices: np.ndarray) -> None:
        v = vertices.astype(np.float32)
        self._recon_edges = np.stack(
            [v[edge_indices[:, 0]], v[edge_indices[:, 1]]], axis=1
        )
        self._refresh()

    def update_normals(self, pts: np.ndarray, dirs: np.ndarray,
                       length: float = 20.0) -> None:
        self._normals_pts  = pts.astype(np.float32)
        self._normals_dirs = dirs.astype(np.float32)
        self._normals_len  = float(length)
        self._refresh()

    def show_normals(self, visible: bool) -> None:
        self._normals_visible = visible
        dpg.configure_item("_pcv_normals", show=visible)
        if not visible:
            dpg.set_value("_pcv_normals", [[], []])
        else:
            self._refresh()

    def show_recon(self, visible: bool) -> None:
        self._recon_visible = visible
        dpg.configure_item("_pcv_recon", show=visible)
        if not visible:
            dpg.set_value("_pcv_recon", [[], []])

    def show_mesh(self, visible: bool) -> None:
        self._mesh_visible = visible
        dpg.configure_item("_pcv_mesh", show=visible)
        if not visible:
            dpg.set_value("_pcv_mesh", [[], []])

    def show_cloud(self, visible: bool) -> None:
        for b in range(_N_DEPTH):
            dpg.configure_item(f"_pcv_cloud_{b}", show=visible)

    def show_rays(self, visible: bool) -> None:
        dpg.configure_item("_pcv_rays", show=visible)

    def set_focus(self, x: float, y: float, z: float) -> None:
        self._focus = np.array([x, y, z], dtype=np.float64)
        self._refresh()

    def set_point_size(self, size: int) -> None:
        """Resize all cloud scatter points by updating the shared size theme."""
        self._point_size = max(1, size)
        tid = "_pcv_size_theme"
        # Must unbind before DPG will allow deletion of a bound theme
        if dpg.does_item_exist(tid):
            dpg.bind_item_theme("_pcv_plot", 0)
            dpg.delete_item(tid)
        with dpg.theme(tag=tid):
            with dpg.theme_component(dpg.mvScatterSeries):
                dpg.add_theme_style(dpg.mvPlotStyleVar_MarkerSize,
                                    float(self._point_size),
                                    category=dpg.mvThemeCat_Plots)
        dpg.bind_item_theme("_pcv_plot", tid)

    # ------------------------------------------------------------------
    # Projection
    # ------------------------------------------------------------------

    def _plot_size(self) -> tuple[int, int]:
        try:
            sz = dpg.get_item_rect_size("_pcv_plot")
            w, h = int(sz[0]), int(sz[1])
            if w > 10 and h > 10:
                return w, h
        except Exception:
            pass
        return _FALLBACK_W, _FALLBACK_H

    def _camera_basis(self) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        """Return (eye, right, up, fwd) for the current camera state."""
        az, el = self._azimuth, self._elevation
        offset = np.array([
            self._distance * math.cos(el) * math.cos(az),
            self._distance * math.cos(el) * math.sin(az),
            self._distance * math.sin(el),
        ], dtype=np.float64)
        eye = self._focus + offset
        fwd = -offset / (np.linalg.norm(offset) + 1e-12)
        up_ref = np.array([0.0, 0.0, 1.0])
        if abs(np.dot(fwd, up_ref)) > 0.95:
            up_ref = np.array([0.0, 1.0, 0.0])
        right = np.cross(fwd, up_ref)
        right /= np.linalg.norm(right)
        up = np.cross(right, fwd)
        return eye, right, up, fwd

    def _project(self, pts: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        """Project 3D points → (proj_xy, valid_mask, cam_z)."""
        if len(pts) == 0:
            empty2 = np.empty((0, 2), dtype=np.float32)
            return empty2, np.empty(0, dtype=bool), np.empty(0, dtype=np.float32)

        w, h = self._plot_size()
        eye, right, up, fwd = self._camera_basis()

        rel   = pts.astype(np.float64) - eye
        cam_x = rel @ right
        cam_y = rel @ up
        cam_z = rel @ fwd

        focal = min(w, h) * 0.7
        valid = cam_z > 10.0
        z_s   = np.where(valid, cam_z, 1.0)

        sx = cam_x / z_s * focal
        sy = cam_y / z_s * focal
        return (np.column_stack([sx, sy]).astype(np.float32),
                valid,
                cam_z.astype(np.float32))

    # ------------------------------------------------------------------
    # Redraw
    # ------------------------------------------------------------------

    def _reset_axis_limits(self) -> None:
        """Lock the plot axes so DPG never auto-fits against our projection."""
        w, h = self._plot_size()
        focal   = min(w, h) * 0.7
        y_scale = focal * 1.2
        x_scale = y_scale * w / h
        dpg.set_axis_limits("_pcv_xax", -x_scale, x_scale)
        dpg.set_axis_limits("_pcv_yax", -y_scale,  y_scale)

    def _draw_grid(self) -> None:
        if self._grid_pts3d is None:
            pts_list = []
            ext = _GRID_N * _GRID_STEP
            for i in range(-_GRID_N, _GRID_N + 1):
                v = i * _GRID_STEP
                pts_list += [[v, -ext, 0], [v,  ext, 0], [np.nan, np.nan, np.nan]]
                pts_list += [[-ext, v, 0], [ ext, v, 0], [np.nan, np.nan, np.nan]]
            self._grid_pts3d = np.array(pts_list, dtype=np.float32)

        pts = self._grid_pts3d
        gp, gv, _ = self._project(pts)
        # Vectorized: NaN rows stay NaN, valid rows get projected coords
        xs = np.where(gv, gp[:, 0], np.nan)
        ys = np.where(gv, gp[:, 1], np.nan)
        dpg.set_value("_pcv_grid", [xs.tolist(), ys.tolist()])

    def _draw_edge_series(self, edges: np.ndarray | None,
                          visible: bool, tag: str) -> None:
        if edges is None or not visible:
            return
        E   = len(edges)
        pts = np.empty((E * 3, 3), dtype=np.float32)
        pts[0::3] = edges[:, 0]
        pts[1::3] = edges[:, 1]
        pts[2::3] = np.nan

        mp, mv, _ = self._project(pts)
        both = mv[0::3] & mv[1::3]
        if both.any():
            result = np.full((int(both.sum()) * 3, 2), np.nan, dtype=np.float32)
            result[0::3] = mp[0::3][both]
            result[1::3] = mp[1::3][both]
            dpg.set_value(tag, [result[:, 0].tolist(), result[:, 1].tolist()])
        else:
            dpg.set_value(tag, [[], []])

    def _draw_normals(self) -> None:
        if not self._normals_visible or len(self._normals_pts) == 0:
            return
        tips = self._normals_pts + self._normals_dirs * self._normals_len
        N    = len(self._normals_pts)
        pts  = np.empty((N * 3, 3), dtype=np.float32)
        pts[0::3] = self._normals_pts
        pts[1::3] = tips
        pts[2::3] = np.nan
        pp, pv, _ = self._project(pts)
        o_idx = np.arange(N) * 3
        e_idx = o_idx + 1
        both  = pv[o_idx] & pv[e_idx]
        if not both.any():
            dpg.set_value("_pcv_normals", [[], []])
            return
        n_valid = int(both.sum())
        result  = np.full((n_valid * 3, 2), np.nan, dtype=np.float32)
        result[0::3] = pp[o_idx[both]]
        result[1::3] = pp[e_idx[both]]
        dpg.set_value("_pcv_normals", [result[:, 0].tolist(), result[:, 1].tolist()])

    def _draw_mesh(self) -> None:
        self._draw_edge_series(self._mesh_edges,  self._mesh_visible,  "_pcv_mesh")
        self._draw_edge_series(self._recon_edges, self._recon_visible, "_pcv_recon")
        self._draw_normals()

    def _refresh(self) -> None:
        self._draw_grid()
        self._draw_mesh()

        AXIS = 100.0
        origin = np.zeros((1, 3), dtype=np.float32)
        tips   = np.array([[AXIS,0,0],[0,AXIS,0],[0,0,AXIS]], dtype=np.float32)
        op, ov, _ = self._project(origin)
        tp, tv, _ = self._project(tips)
        if ov[0]:
            o = op[0]
            for tag, i in [("_pcv_ax_x", 0), ("_pcv_ax_y", 1), ("_pcv_ax_z", 2)]:
                if tv[i]:
                    t = tp[i]
                    dpg.set_value(tag, [[o[0], t[0]], [o[1], t[1]]])

        # Measurement rays (vectorized)
        if len(self._ray_origins) > 0:
            R = len(self._ray_origins)
            all_pts = np.empty((R * 3, 3), dtype=np.float32)
            all_pts[0::3] = self._ray_origins
            all_pts[1::3] = self._ray_endpoints
            all_pts[2::3] = np.nan
            rp, rv, _ = self._project(all_pts)
            o_idx = np.arange(R) * 3
            e_idx = o_idx + 1
            both  = rv[o_idx] & rv[e_idx]
            if both.any():
                n_v    = int(both.sum())
                result = np.full((n_v * 3, 2), np.nan, dtype=np.float32)
                result[0::3] = rp[o_idx[both]]
                result[1::3] = rp[e_idx[both]]
                dpg.set_value("_pcv_rays", [result[:, 0].tolist(), result[:, 1].tolist()])
            else:
                dpg.set_value("_pcv_rays", [[], []])
        else:
            dpg.set_value("_pcv_rays", [[], []])

        # Cloud — colour by depth bucket; display cap keeps projection fast
        if len(self._cloud) > 0:
            N = len(self._cloud)
            if N > _MAX_DISPLAY_PTS:
                stride = N // _MAX_DISPLAY_PTS
                self._display_idx = np.arange(0, N, stride, dtype=np.int32)[:_MAX_DISPLAY_PTS]
            else:
                self._display_idx = np.arange(N, dtype=np.int32)
            display_cloud = self._cloud[self._display_idx]

            proj, valid, cam_z_all = self._project(display_cloud)
            self._proj_cloud = proj
            self._proj_valid = valid

            vis_idx = np.where(valid)[0]
            if len(vis_idx) > 0:
                vis_pts = proj[vis_idx]
                vis_z   = cam_z_all[vis_idx]
                z_min, z_max = float(vis_z.min()), float(vis_z.max())
                z_range = max(z_max - z_min, 1.0)
                buckets = np.clip(
                    ((vis_z - z_min) / z_range * _N_DEPTH).astype(int),
                    0, _N_DEPTH - 1,
                )
                for b in range(_N_DEPTH):
                    mask = buckets == b
                    if mask.any():
                        bpts = vis_pts[mask]
                        dpg.set_value(f"_pcv_cloud_{b}",
                                      [bpts[:, 0].tolist(), bpts[:, 1].tolist()])
                    else:
                        dpg.set_value(f"_pcv_cloud_{b}", [[], []])
            else:
                for b in range(_N_DEPTH):
                    dpg.set_value(f"_pcv_cloud_{b}", [[], []])
        else:
            self._display_idx = np.empty(0, dtype=np.int32)
            self._proj_cloud  = np.empty((0, 2), dtype=np.float32)
            self._proj_valid  = np.empty(0, dtype=bool)
            for b in range(_N_DEPTH):
                dpg.set_value(f"_pcv_cloud_{b}", [[], []])

        dp, dv, _ = self._project(self._device_pos.reshape(1, 3))
        if dv[0]:
            dpg.set_value("_pcv_dev", [[dp[0, 0]], [dp[0, 1]]])

            pitch, roll, yaw = self._device_orient.tolist()
            fwd_w = self._device_pos + _AXIS_LEN * np.array([
                math.cos(pitch) * math.cos(yaw),
                math.cos(pitch) * math.sin(yaw),
                math.sin(pitch)], dtype=np.float32)
            rgt_w = self._device_pos + _AXIS_LEN * np.array([
                -math.sin(yaw), math.cos(yaw), 0.0], dtype=np.float32)

            for tag, tip in [("_pcv_dev_ax1", fwd_w), ("_pcv_dev_ax2", rgt_w)]:
                ep, ev, _ = self._project(tip.reshape(1, 3))
                if ev[0]:
                    dpg.set_value(tag, [
                        [dp[0, 0], ep[0, 0]],
                        [dp[0, 1], ep[0, 1]]
                    ])

        self._reset_axis_limits()

    # ------------------------------------------------------------------
    # Mouse handlers
    # ------------------------------------------------------------------

    def _on_press(self, _s, _a) -> None:
        self._dragging    = dpg.is_item_hovered("_pcv_plot")
        self._last_drag_x = 0.0
        self._last_drag_y = 0.0

    def _on_release(self, _s, _a) -> None:
        self._dragging = False

    def _on_drag(self, _s, app_data) -> None:
        if not self._dragging:
            return
        cum_x, cum_y = app_data[1], app_data[2]
        dx = cum_x - self._last_drag_x
        dy = cum_y - self._last_drag_y
        self._last_drag_x = cum_x
        self._last_drag_y = cum_y

        shift = (dpg.is_key_down(dpg.mvKey_LShift)
                 or dpg.is_key_down(dpg.mvKey_RShift))
        if shift:
            # Pan: translate the focus point in the camera plane
            w, h = self._plot_size()
            focal = min(w, h) * 0.7
            scale = self._distance / focal
            _, right, up, _ = self._camera_basis()
            # Drag right/down → scene follows (positive right/negative up)
            self._focus -= (right * dx - up * dy) * scale
            if self.on_focus_change:
                self.on_focus_change(*self._focus.tolist())
        else:
            self._azimuth  -= dx * 0.005
            self._elevation = max(-1.4, min(1.4, self._elevation + dy * 0.005))
        self._refresh()

    def _on_wheel(self, _s, app_data) -> None:
        if not dpg.is_item_hovered("_pcv_plot"):
            return
        self._distance = max(100.0, self._distance - app_data * 50.0)
        self._refresh()

    def _on_move(self, _s, _a) -> None:
        if not dpg.is_item_hovered("_pcv_plot") or len(self._proj_cloud) == 0:
            dpg.configure_item("_pcv_annot", show=False)
            return

        mp    = dpg.get_plot_mouse_pos()
        mouse = np.array([mp[0], mp[1]], dtype=np.float32)

        valid_idx = np.where(self._proj_valid)[0]
        if len(valid_idx) == 0:
            dpg.configure_item("_pcv_annot", show=False)
            return

        vp = self._proj_cloud[valid_idx]
        d2 = np.sum((vp - mouse) ** 2, axis=1)
        ni = int(np.argmin(d2))
        if d2[ni] > 20 ** 2:
            dpg.configure_item("_pcv_annot", show=False)
            return

        disp_i = valid_idx[ni]                          # index into display subset
        ci     = int(self._display_idx[disp_i]) if len(self._display_idx) > disp_i else disp_i
        pt = self._cloud[ci]

        dist_str = ""
        if ci < len(self._ray_origins) and ci < len(self._ray_endpoints):
            meas_d = float(np.linalg.norm(
                self._ray_endpoints[ci].astype(np.float64)
                - self._ray_origins[ci].astype(np.float64)
            ))
            dist_str = f"\nd = {meas_d:.1f} mm"

        px, py = float(self._proj_cloud[disp_i, 0]), float(self._proj_cloud[disp_i, 1])
        label = f"({pt[0]:.1f}, {pt[1]:.1f}, {pt[2]:.1f}){dist_str}"
        dpg.configure_item("_pcv_annot", label=label,
                           default_value=(px, py), show=True)

    # ------------------------------------------------------------------
    # Themes
    # ------------------------------------------------------------------

    def _apply_themes(self) -> None:
        def _scatter(r, g, b, sz=3):
            with dpg.theme() as t:
                with dpg.theme_component(dpg.mvScatterSeries):
                    dpg.add_theme_color(dpg.mvPlotCol_MarkerFill,   (r, g, b, 200),
                                        category=dpg.mvThemeCat_Plots)
                    dpg.add_theme_color(dpg.mvPlotCol_MarkerOutline, (r, g, b, 200),
                                        category=dpg.mvThemeCat_Plots)
                    dpg.add_theme_style(dpg.mvPlotStyleVar_MarkerSize, sz,
                                        category=dpg.mvThemeCat_Plots)
            return t

        def _line(r, g, b, thickness=2, a=220):
            with dpg.theme() as t:
                with dpg.theme_component(dpg.mvLineSeries):
                    dpg.add_theme_color(dpg.mvPlotCol_Line, (r, g, b, a),
                                        category=dpg.mvThemeCat_Plots)
                    dpg.add_theme_style(dpg.mvPlotStyleVar_LineWeight, thickness,
                                        category=dpg.mvThemeCat_Plots)
            return t

        dpg.bind_item_theme("_pcv_grid",    _line(80,  80,  80,  thickness=1, a=90))
        dpg.bind_item_theme("_pcv_mesh",    _line(100, 140, 200, thickness=1, a=70))
        dpg.bind_item_theme("_pcv_recon",   _line(180, 120,  60, thickness=1, a=140))
        dpg.bind_item_theme("_pcv_normals", _line(255, 140,  30, thickness=1, a=200))
        dpg.bind_item_theme("_pcv_rays",    _line(60, 180, 200, thickness=1))
        dpg.bind_item_theme("_pcv_dev",     _scatter(255, 60, 60, sz=8))
        dpg.bind_item_theme("_pcv_ax_x",    _line(255, 80,  80))
        dpg.bind_item_theme("_pcv_ax_y",    _line(80,  220, 80))
        dpg.bind_item_theme("_pcv_ax_z",    _line(80,  120, 255))
        dpg.bind_item_theme("_pcv_dev_ax1", _line(255, 180, 60))
        dpg.bind_item_theme("_pcv_dev_ax2", _line(60,  200, 255))

        # Per-series depth colours (no size here — size lives in the plot theme)
        for b in range(_N_DEPTH):
            r, g, bl = _DEPTH_COLORS[b]
            with dpg.theme(tag=f"_pcv_cloud_theme_{b}"):
                with dpg.theme_component(dpg.mvScatterSeries):
                    dpg.add_theme_color(dpg.mvPlotCol_MarkerFill,
                                        (r, g, bl, 200),
                                        category=dpg.mvThemeCat_Plots)
                    dpg.add_theme_color(dpg.mvPlotCol_MarkerOutline,
                                        (r, g, bl, 200),
                                        category=dpg.mvThemeCat_Plots)
            dpg.bind_item_theme(f"_pcv_cloud_{b}", f"_pcv_cloud_theme_{b}")

        # Plot-level size theme — one rebuild per set_point_size() call
        self.set_point_size(self._point_size)
