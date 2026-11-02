# sim — Simulator session

Generates synthetic scan data by ray-casting against a loaded 3D mesh. Same callback interface as `ScanSession` — `MainWindow` can swap between them without changing its wiring.

## Files

| File | Description |
|------|-------------|
| `sim_session.py` | `SimSession` — ray-casts an STL/OBJ mesh at configurable device positions |

## Features

- **Load STL / OBJ** — any mesh via `trimesh`
- **Configurable noise** — Gaussian noise on distance (mm) and orientation (°)
- **Manual device control** — azimuth, elevation, distance sliders
- **Auto orbit** — rotates the virtual device continuously; elevation bounces between min/max
- **Fast-forward** — up to 500× real-time for rapid point accumulation
- **Same interface** — fires `on_cloud_update`, `on_pose_update`, `on_ray_update`, `on_log` just like the real session
