# data — Domain objects

In-memory data structures that accumulate and hold the scan state.

## Files

| File | Description |
|------|-------------|
| `point_cloud.py` | `PointCloud` — ray-cluster accumulator; merges incoming points by ray proximity, direction, and surface distance |
| `device_pose.py` | `DevicePose` — holds the latest position (mm) and orientation (rad) from POSE packets |

## PointCloud — ray-cluster merge algorithm

Each incoming `PointPacket` carries a surface point and a sensor origin, defining a measurement ray. A new point is merged into an existing cluster when **all three** conditions hold:

1. **Ray proximity** — sensor origin within **5 mm** of the cluster's ray line (cylinder test, not sphere)
2. **Direction match** — ray directions agree within **1.15°** (dot product > 0.9998)
3. **Surface proximity** — new surface point within **15 mm** of cluster centroid (occlusion guard)

Among multiple matching clusters, the one whose ray line passes closest to the new sensor is preferred. If no cluster matches, a new one is started.

### Output

`to_numpy_with_normals()` returns `(points, outward_normals)`:
- `points` — one centroid per cluster (float32, mm)
- `outward_normals` — negated ray direction per cluster (unit vectors pointing away from the surface toward the sensor)
