"""Ray-averaged point cloud.

Each measurement is a ray: the sensor position (origin) and the direction
toward the measured surface.  When a new measurement arrives, three conditions
must all hold for it to merge into an existing cluster:

  1. **Ray proximity** — the new sensor position lies within ``_PERP_THRESH_MM``
     of the stored cluster's ray *line* (not just near the origin).  This lets
     the sensor approach or retreat along the same ray without breaking the
     cluster.  A projection-range guard also ensures the sensor is on the
     correct side (not past the surface).

  2. **Direction match** — the angle between the two ray directions is below
     ``_DIR_THRESH_COS`` (≈ 1.15°).

  3. **Surface proximity** — the new surface point must land within
     ``_SURFACE_THRESH_MM`` of the cluster's current average surface point.
     This prevents an occluding object in front of an already-measured surface
     from being folded into the wrong cluster.

Without sensor pose info (no IMU) ``add`` falls back to plain accumulation
because merging requires a known sensor position.
"""

from __future__ import annotations

import numpy as np

_PERP_THRESH_MM    = 5.0     # mm   — max perpendicular distance from ray line
_DIR_THRESH_COS    = 0.9998  # ≈ cos(1.15°)
_SURFACE_THRESH_MM = 15.0    # mm   — max distance between new and stored surface point


class PointCloud:
    def __init__(self) -> None:
        # Parallel lists — one entry per unique ray cluster
        self._origins:   list[np.ndarray] = []   # sensor pos at first measurement
        self._dirs:      list[np.ndarray] = []   # unit direction toward surface
        self._sum_dists: list[float]      = []   # sum of merged distance readings
        self._counts:    list[int]        = []   # how many readings merged

    # ------------------------------------------------------------------
    # Mutation
    # ------------------------------------------------------------------

    def add(self, x: float, y: float, z: float,
            sensor_pos: np.ndarray | None = None) -> None:
        """Merge (x, y, z) into an existing ray cluster or start a new one.

        Pass *sensor_pos* to enable ray-based merging.  Without it the point
        is stored as-is with no merging.
        """
        world_pt = np.array([x, y, z], dtype=np.float64)

        if sensor_pos is None:
            self._origins.append(np.zeros(3))
            norm = np.linalg.norm(world_pt)
            self._dirs.append(world_pt / (norm + 1e-9))
            self._sum_dists.append(float(norm))
            self._counts.append(1)
            return

        pos = np.asarray(sensor_pos, dtype=np.float64)
        rel  = world_pt - pos
        dist = float(np.linalg.norm(rel))
        if dist < 1e-6:
            return
        direction = rel / dist

        best = -1
        if self._origins:
            origins   = np.array(self._origins)   # (N, 3)
            dirs      = np.array(self._dirs)      # (N, 3)
            avg_dists = (np.array(self._sum_dists)
                         / np.array(self._counts, dtype=np.float64))  # (N,)

            # --- condition 1: sensor lies on the stored ray line ---
            # v = vector from each cluster origin to new sensor position
            v      = pos - origins                              # (N, 3)
            t      = (v * dirs).sum(axis=1)                    # (N,) — projection along ray
            perp2  = np.maximum(0.0, (v * v).sum(axis=1) - t * t)  # (N,) — perp dist²

            sensor_on_ray = (
                (t >= -_PERP_THRESH_MM) &              # not significantly behind origin
                (t <= avg_dists + _PERP_THRESH_MM) &   # not past the surface
                (perp2 < _PERP_THRESH_MM ** 2)         # within cylinder radius
            )

            # --- condition 2: direction match ---
            dot_p = dirs @ direction                           # (N,)
            dir_ok = dot_p > _DIR_THRESH_COS

            # --- condition 3: new surface point near cluster surface ---
            cluster_surfaces = origins + avg_dists[:, None] * dirs  # (N, 3)
            new_surface      = pos + dist * direction                # (3,)
            surf_d2 = ((cluster_surfaces - new_surface) ** 2).sum(axis=1)
            surf_ok = surf_d2 < _SURFACE_THRESH_MM ** 2

            mask = sensor_on_ray & dir_ok & surf_ok
            if mask.any():
                perp2[~mask] = np.inf
                best = int(np.argmin(perp2))

        if best >= 0:
            # Convert measured distance to distance from cluster origin along the ray.
            # A sensor that moved 100 mm closer and measured 400 mm is still 500 mm
            # from the origin — t accounts for the sensor offset along the ray.
            dist_from_origin = float(t[best]) + dist
            self._sum_dists[best] += dist_from_origin
            self._counts[best]    += 1
        else:
            self._origins.append(pos.copy())
            self._dirs.append(direction.copy())
            self._sum_dists.append(dist)
            self._counts.append(1)

    def clear(self) -> None:
        self._origins.clear()
        self._dirs.clear()
        self._sum_dists.clear()
        self._counts.clear()

    # ------------------------------------------------------------------
    # Read-out
    # ------------------------------------------------------------------

    def rays_numpy(self) -> tuple[np.ndarray, np.ndarray]:
        """Return (origins, endpoints) as two (N, 3) float32 arrays."""
        if not self._origins:
            empty = np.empty((0, 3), dtype=np.float32)
            return empty, empty
        origins   = np.array(self._origins, dtype=np.float32)
        dirs      = np.array(self._dirs, dtype=np.float32)
        dists     = (np.array(self._sum_dists) / np.array(self._counts, dtype=np.float64))
        endpoints = (origins + dists[:, None].astype(np.float32) * dirs)
        return origins, endpoints.astype(np.float32)

    def to_numpy_with_normals(self) -> tuple[np.ndarray, np.ndarray]:
        """Return (points, outward_normals) as (N, 3) float32 arrays.

        Normals point from the surface back toward the sensor (outward for
        exterior scanning), which is the direction Poisson reconstruction expects.
        """
        if not self._origins:
            empty = np.empty((0, 3), dtype=np.float32)
            return empty, empty
        origins = np.array(self._origins)
        dirs    = np.array(self._dirs)
        dists   = np.array(self._sum_dists) / np.array(self._counts, dtype=np.float64)
        points  = (origins + dists[:, None] * dirs).astype(np.float32)
        normals = (-dirs).astype(np.float32)   # flip: sensor→surface → surface→sensor
        return points, normals

    def to_numpy(self) -> np.ndarray:
        """Return a (N, 3) float32 array of averaged surface positions."""
        if not self._origins:
            return np.empty((0, 3), dtype=np.float32)
        origins = np.array(self._origins)
        dirs    = np.array(self._dirs)
        dists   = np.array(self._sum_dists) / np.array(self._counts, dtype=np.float64)
        return (origins + dists[:, None] * dirs).astype(np.float32)

    def save_ply(self, path: str) -> int:
        pts = self.to_numpy()
        with open(path, "w") as f:
            f.write("ply\nformat ascii 1.0\n")
            f.write(f"element vertex {len(pts)}\n")
            f.write("property float x\nproperty float y\nproperty float z\n")
            f.write("end_header\n")
            for x, y, z in pts:
                f.write(f"{x:.4f} {y:.4f} {z:.4f}\n")
        return len(pts)

    def __len__(self) -> int:
        return len(self._origins)
