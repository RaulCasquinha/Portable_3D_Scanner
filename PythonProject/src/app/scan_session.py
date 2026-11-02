"""Orchestrates the scan session: connects transport -> protocol -> data
and exposes plain Python callbacks for the GUI layer."""

from __future__ import annotations

import time
from typing import Callable

import numpy as np

from src.data.device_pose import DevicePose
from src.data.point_cloud import PointCloud
from src.protocol.app_layer import AppLayer, LogPacket, PointPacket, PosePacket
from src.transport.serial_transport import SerialTransport


class ScanSession:
    """Top-level controller for one scanning session."""

    def __init__(self) -> None:
        self._transport = SerialTransport()
        self._protocol  = AppLayer()
        self._cloud     = PointCloud()
        self._pose      = DevicePose()

        self.on_cloud_update:  Callable[[np.ndarray], None]              | None = None
        self.on_pose_update:   Callable[[np.ndarray, np.ndarray], None]  | None = None
        self.on_log:           Callable[[str], None]                     | None = None
        self.on_status:        Callable[[str], None]                     | None = None
        self.on_sensor_status: Callable[[bool, bool], None]              | None = None
        self.on_ray_update:    Callable[[np.ndarray, np.ndarray], None]  | None = None

        self._lidar_ok      = False
        self._imu_ok        = False
        self._recording     = True
        self._last_dist_mm: float = 0.0

        self._first_data    = True
        self._connect_time  = 0.0
        self._warned_nodata = False

        self._protocol.on_point  = self._on_point
        self._protocol.on_pose   = self._on_pose
        self._protocol.on_log    = self._on_log
        self._protocol.on_debug  = self._on_proto_debug
        self._transport.on_error = self._on_error

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def start(self, port: str, baud: int = 115200) -> None:
        self._first_data    = True
        self._lidar_ok      = False
        self._imu_ok        = False
        self._connect_time  = time.monotonic()
        self._warned_nodata = False
        self._transport.open(port, baud)
        self._emit_status(f"Connected - {port} @ {baud}")

    def stop(self) -> None:
        self._transport.close()
        self._emit_status("Disconnected")

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
            self._emit_log(f"[INFO] Saved {n} points to {path}")
        except OSError as exc:
            self._emit_log(f"[ERROR] Save failed: {exc}")

    def cloud_for_reconstruction(self) -> tuple[np.ndarray, np.ndarray]:
        return self._cloud.to_numpy_with_normals()

    def reconstruct_to_stl(self, path: str) -> None:
        from src.processing.reconstruction import reconstruct_poisson
        ok, msg = reconstruct_poisson(self._cloud, path)
        if ok:
            self._emit_log(f"[INFO] Reconstruction: {msg}")
        else:
            self._emit_log(f"[ERROR] Reconstruction failed: {msg}")

    def process_pending(self) -> None:
        """Drain the serial queue and decode packets. Call every render frame."""
        now  = time.monotonic()
        data = self._transport.read_pending()

        if not data:
            if (self._first_data
                    and not self._warned_nodata
                    and now - self._connect_time > 5.0):
                self._warned_nodata = True
                self._emit_log("[WARN] No data after 5 s - check port, firmware, and power")
            return

        if self._first_data:
            self._first_data = False

        self._protocol.feed(data)

    @property
    def point_count(self) -> int:
        return len(self._cloud)

    @property
    def last_dist_mm(self) -> float:
        return self._last_dist_mm

    @staticmethod
    def available_ports() -> list[str]:
        return SerialTransport.list_ports()

    # ------------------------------------------------------------------
    # Internal handlers
    # ------------------------------------------------------------------

    def _on_point(self, pkt: PointPacket) -> None:
        if not self._recording:
            return
        sensor_pos = np.array([pkt.ox, pkt.oy, pkt.oz], dtype=np.float32)
        dx, dy, dz = pkt.x - pkt.ox, pkt.y - pkt.oy, pkt.z - pkt.oz
        self._last_dist_mm = float(np.sqrt(dx*dx + dy*dy + dz*dz))
        self._cloud.add(pkt.x, pkt.y, pkt.z, sensor_pos)
        if self.on_cloud_update:
            self.on_cloud_update(self._cloud.to_numpy())
        if self.on_ray_update:
            origins, endpoints = self._cloud.rays_numpy()
            self.on_ray_update(origins, endpoints)

    def _on_pose(self, pkt: PosePacket) -> None:
        self._pose.update(pkt.px, pkt.py, pkt.pz, pkt.pitch, pkt.roll, pkt.yaw)
        if self.on_pose_update:
            self.on_pose_update(self._pose.position, self._pose.orientation)

    def _on_proto_debug(self, msg: str) -> None:
        if any(k in msg for k in ("CRC mismatch", "Struct decode", "Unknown packet")):
            self._emit_log(f"[WARN] {msg}")

    def _on_log(self, pkt: LogPacket) -> None:
        msg = pkt.message.strip()
        self._parse_sensor_status(msg)
        for prefix in ("[INFO]", "[WARN]", "[ERROR]"):
            if msg.startswith(prefix):
                body = msg[len(prefix):].strip()
                self._emit_log(f"{prefix} (device) {body}")
                return
        self._emit_log(f"[DEVICE] {msg}")

    def _parse_sensor_status(self, msg: str) -> None:
        if not self.on_sensor_status:
            return
        low = msg.lower()
        if "lidar ok" in low:
            self._lidar_ok = True
            self.on_sensor_status(self._lidar_ok, self._imu_ok)
        elif "lidar not found" in low:
            self._lidar_ok = False
            self.on_sensor_status(self._lidar_ok, self._imu_ok)
        elif "imu ok" in low:
            self._imu_ok = True
            self.on_sensor_status(self._lidar_ok, self._imu_ok)
        elif "imu not found" in low:
            self._imu_ok = False
            self.on_sensor_status(self._lidar_ok, self._imu_ok)

    def _on_error(self, msg: str) -> None:
        self._emit_status(f"Error: {msg}")
        self._emit_log(f"[ERROR] {msg}")

    def _emit_status(self, msg: str) -> None:
        if self.on_status:
            self.on_status(msg)

    def _emit_log(self, msg: str) -> None:
        if self.on_log:
            self.on_log(msg)
