"""Simulator session — generates synthetic scan data from an STL/OBJ mesh.

Mirrors the public interface of ScanSession so MainWindow can wire the same
GUI callbacks to either backend without knowing which one is active.

Simulation model
----------------
The virtual device orbits around the mesh centroid on a sphere defined by
(azimuth, elevation, distance).  On each process_pending() call:

  - Pose packets are emitted at the full frame rate (IMU simulation).
  - LiDAR fires at ``LIDAR_HZ`` Hz: a ray is cast from the device position
    toward the mesh centroid; the first intersection distance is taken,
    Gaussian noise is added, and the resulting POINT is injected into the
    cloud exactly as a real PointPacket would be.

Requires ``trimesh`` for ray casting (install via requirements.txt).
"""

from __future__ import annotations

import math
import random
import time
from typing import Callable

import numpy as np

from src.data.device_pose import DevicePose
from src.data.point_cloud import PointCloud

LIDAR_HZ = 10   # simulated LiDAR measurements per second


class SimSession:
    """Drop-in ScanSession replacement that generates data from a 3-D mesh."""

    def __init__(self) -> None:
        self._cloud = PointCloud()
        self._pose  = DevicePose()

        # Mesh state
        self._mesh        = None          # trimesh.Trimesh or None
        self._mesh_path   = ""
        self._mesh_center = np.zeros(3, dtype=np.float64)

        # Virtual device spherical coordinates around mesh center
        self._azimuth   = 0.0    # radians
        self._elevation = 0.2    # radians
        self._distance  = 500.0  # mm

        # Noise parameters
        self._dist_noise_mm    = 2.0
        self._orient_noise_deg = 0.5

        # Auto-orbit
        self._auto_orbit  = False
        self._orbit_speed = 15.0              # azimuth deg/s
        self._elev_speed  = 5.0              # elevation deg/s
        self._elev_dir    = 1                # +1 sweeping up, -1 sweeping down
        self._elev_min    = math.radians(-70.0)
        self._elev_max    = math.radians( 70.0)
        self._time_scale  = 1.0              # fast-forward multiplier

        # Timing
        self._running      = False
        self._recording    = True
        self._last_tick    = 0.0
        self._next_lidar   = 0.0
        self._last_dist_mm = 0.0

        # Callbacks — same names as ScanSession
        self.on_cloud_update:  Callable | None = None
        self.on_pose_update:   Callable | None = None
        self.on_log:           Callable | None = None
        self.on_status:        Callable | None = None
        self.on_sensor_status: Callable | None = None
        self.on_ray_update:    Callable | None = None

        # Optional callback fired when the device position changes
        # (used by SimulatorWindow to refresh sliders)
        self.on_device_move:   Callable | None = None

    # ------------------------------------------------------------------
    # ScanSession-compatible public API
    # ------------------------------------------------------------------

    def start(self, *_args, **_kwargs) -> None:
        self._cloud.clear()
        self._last_tick  = time.monotonic()
        self._next_lidar = self._last_tick
        self._running    = True
        if self.on_sensor_status:
            self.on_sensor_status(True, True)
        if self.on_status:
            self.on_status("Simulator running")
        if self.on_log:
            model = self._mesh_path.split("/")[-1] if self._mesh_path else "no model"
            self.on_log(f"[INFO] Simulator started — {model}")
        # Emit initial empty cloud
        if self.on_cloud_update:
            self.on_cloud_update(self._cloud.to_numpy())

    def stop(self) -> None:
        self._running = False
        if self.on_status:
            self.on_status("Simulator stopped")

    def reset_reference(self) -> None:
        self._cloud.clear()
        self._pose.update(0, 0, 0, 0, 0, 0)
        empty3 = np.empty((0, 3), dtype=np.float32)
        if self.on_cloud_update:
            self.on_cloud_update(empty3)
        if self.on_ray_update:
            self.on_ray_update(empty3, empty3)

    @property
    def is_recording(self) -> bool:
        return self._recording

    def start_recording(self) -> None:
        self._recording = True

    def stop_recording(self) -> None:
        self._recording = False

    def save_cloud(self, path: str) -> None:
        try:
            n = self._cloud.save_ply(path)
            if self.on_log:
                self.on_log(f"[INFO] Saved {n} points to {path}")
        except OSError as exc:
            if self.on_log:
                self.on_log(f"[ERROR] Save failed: {exc}")

    def cloud_for_reconstruction(self) -> tuple[np.ndarray, np.ndarray]:
        return self._cloud.to_numpy_with_normals()

    def reconstruct_to_stl(self, path: str) -> None:
        from src.processing.reconstruction import reconstruct_poisson
        ok, msg = reconstruct_poisson(self._cloud, path)
        if self.on_log:
            prefix = "[INFO]" if ok else "[ERROR]"
            self.on_log(f"{prefix} Reconstruction: {msg}" if ok
                        else f"{prefix} Reconstruction failed: {msg}")

    @property
    def point_count(self) -> int:
        return len(self._cloud)

    @property
    def last_dist_mm(self) -> float:
        return self._last_dist_mm

    @staticmethod
    def available_ports() -> list[str]:
        return []

    def process_pending(self) -> None:
        """Advance simulation one step.  Call every render frame."""
        if not self._running:
            return

        now = time.monotonic()
        dt  = now - self._last_tick
        self._last_tick = now

        # Auto orbit — azimuth rotates continuously, elevation bounces ±80°
        if self._auto_orbit:
            sdt = dt * self._time_scale
            self._azimuth   += math.radians(self._orbit_speed) * sdt
            self._elevation += math.radians(self._elev_speed) * self._elev_dir * sdt
            if self._elevation >= self._elev_max:
                self._elevation = self._elev_max
                self._elev_dir  = -1
            elif self._elevation <= self._elev_min:
                self._elevation = self._elev_min
                self._elev_dir  = 1
            if self.on_device_move:
                self.on_device_move(self._azimuth, self._elevation, self._distance)

        # Build true device pose
        true_pos    = self._device_pos()
        true_orient = self._device_orient()

        # Add orientation noise for the "IMU reading"
        o_std = math.radians(self._orient_noise_deg)
        noisy_orient = true_orient + np.random.normal(0, o_std, 3).astype(np.float64)

        # Emit pose
        self._pose.update(
            true_pos[0], true_pos[1], true_pos[2],
            noisy_orient[0], noisy_orient[1], noisy_orient[2],
        )
        if self.on_pose_update:
            self.on_pose_update(self._pose.position, self._pose.orientation)

        # Fire LiDAR — rate scales with fast-forward
        if now >= self._next_lidar and self._recording:
            self._next_lidar = now + 1.0 / (LIDAR_HZ * self._time_scale)
            self._fire_lidar(true_pos, true_orient, noisy_orient)

    # ------------------------------------------------------------------
    # Simulator-specific controls
    # ------------------------------------------------------------------

    def load_model(self, path: str) -> tuple[bool, str]:
        """Load an STL/OBJ mesh.  Returns (ok, error_message)."""
        try:
            import trimesh
            m = trimesh.load_mesh(path)
            if m is None or (hasattr(m, 'is_empty') and m.is_empty):
                return False, "Empty or unreadable mesh"
            # Consolidate scene → single mesh if needed
            if hasattr(m, 'dump'):
                m = m.dump(concatenate=True)
            self._mesh        = m
            self._mesh_path   = path
            self._mesh_center = np.array(m.centroid, dtype=np.float64)
            # Auto-scale distance to fit the model
            self._distance = float(np.max(m.extents)) * 2.5
            return True, ""
        except Exception as exc:
            return False, str(exc)

    def mesh_edges(self) -> tuple[np.ndarray, np.ndarray] | None:
        """Return (vertices, edge_indices) for wireframe rendering, or None."""
        if self._mesh is None:
            return None
        verts = np.array(self._mesh.vertices, dtype=np.float32)
        edges = self._mesh.edges_unique   # (E, 2)
        # Cap at 3000 edges for rendering performance
        if len(edges) > 3000:
            step  = len(edges) // 3000
            edges = edges[::step]
        return verts, edges

    def set_noise(self, dist_noise_mm: float, orient_noise_deg: float) -> None:
        self._dist_noise_mm    = dist_noise_mm
        self._orient_noise_deg = orient_noise_deg

    def set_device_pose(self, azimuth: float, elevation: float,
                        distance: float) -> None:
        self._azimuth   = azimuth
        self._elevation = elevation
        self._distance  = distance

    def set_auto_orbit(self, enabled: bool, speed_deg_s: float,
                       elev_speed_deg_s: float = 5.0) -> None:
        self._auto_orbit  = enabled
        self._orbit_speed = speed_deg_s
        self._elev_speed  = elev_speed_deg_s

    def set_elev_range(self, min_deg: float, max_deg: float) -> None:
        self._elev_min = math.radians(min_deg)
        self._elev_max = math.radians(max_deg)
        # Clamp current elevation into the new range immediately
        self._elevation = max(self._elev_min, min(self._elev_max, self._elevation))

    def set_time_scale(self, scale: float) -> None:
        self._time_scale = max(1.0, scale)

    @property
    def mesh_loaded(self) -> bool:
        return self._mesh is not None

    @property
    def mesh_filename(self) -> str:
        return self._mesh_path.split("/")[-1] if self._mesh_path else ""

    @property
    def distance(self) -> float:
        return self._distance

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _device_pos(self) -> np.ndarray:
        az, el, d = self._azimuth, self._elevation, self._distance
        offset = np.array([
            d * math.cos(el) * math.cos(az),
            d * math.cos(el) * math.sin(az),
            d * math.sin(el),
        ], dtype=np.float64)
        return self._mesh_center + offset

    def _device_orient(self) -> np.ndarray:
        """Pitch and yaw pointing from device toward mesh center."""
        toward = self._mesh_center - self._device_pos()
        dist   = np.linalg.norm(toward)
        if dist < 1e-6:
            return np.zeros(3)
        d     = toward / dist
        pitch = math.asin(float(np.clip(d[2], -1.0, 1.0)))
        yaw   = math.atan2(float(d[1]), float(d[0]))
        return np.array([pitch, 0.0, yaw], dtype=np.float64)

    def _fire_lidar(self, true_pos: np.ndarray, true_orient: np.ndarray,
                    noisy_orient: np.ndarray) -> None:
        if self._mesh is None:
            return

        pitch, _roll, yaw = true_orient
        direction = np.array([
            math.cos(pitch) * math.cos(yaw),
            math.cos(pitch) * math.sin(yaw),
            math.sin(pitch),
        ], dtype=np.float64)

        locs, _, _ = self._mesh.ray.intersects_location(
            ray_origins=true_pos[None, :],
            ray_directions=direction[None, :],
        )
        if len(locs) == 0:
            return

        # Closest hit
        dists     = np.linalg.norm(locs - true_pos, axis=1)
        true_dist = float(dists[np.argmin(dists)])

        noisy_dist = true_dist + random.gauss(0, self._dist_noise_mm)
        if noisy_dist <= 0:
            return

        # Reported point uses noisy orient + noisy distance (as firmware would)
        np_pitch, _np_roll, np_yaw = noisy_orient
        noisy_dir = np.array([
            math.cos(np_pitch) * math.cos(np_yaw),
            math.cos(np_pitch) * math.sin(np_yaw),
            math.sin(np_pitch),
        ], dtype=np.float64)

        world_pt       = true_pos + noisy_dist * noisy_dir
        self._last_dist_mm = noisy_dist

        sensor_pos = true_pos.astype(np.float32)
        self._cloud.add(float(world_pt[0]), float(world_pt[1]), float(world_pt[2]),
                        sensor_pos)

        if self.on_cloud_update:
            self.on_cloud_update(self._cloud.to_numpy())
        if self.on_ray_update:
            origins, endpoints = self._cloud.rays_numpy()
            self.on_ray_update(origins, endpoints)
