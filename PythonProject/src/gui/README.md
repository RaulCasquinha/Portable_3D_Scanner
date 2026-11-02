# gui — DearPyGUI windows and views

All user interface code. Each file is one independent panel or view.

## Files

| File | Description |
|------|-------------|
| `main_window.py` | `MainWindow` — two-row toolbar, docking host, session wiring, STL save dialog |
| `point_cloud_view.py` | `PointCloudView` — interactive 3D projection with orbit/pan/zoom; depth-coloured cloud; dual mesh layers; normal vectors |
| `reconstruction_window.py` | `ReconstructionWindow` — grouped-PCA parameters, normal preview, Poisson controls, Generate/Auto/STL |
| `simulator_window.py` | `SimulatorWindow` — STL/OBJ loader, noise sliders, device position/orbit controls |
| `device_view.py` | `DeviceView` — "Live Info" panel: position, orientation, cluster count, last LiDAR distance |
| `log_view.py` | `LogView` — scrollable log panel for device messages and status |

## PointCloudView details

- **Projection**: custom perspective, not DPG built-in; `focal = min(w, h) * 0.7`
- **Camera**: orbit (azimuth, elevation, distance) around a focus point; left-drag = orbit, Shift+drag = pan, scroll = zoom
- **Display cap**: at most `_MAX_DISPLAY_PTS = 6000` points projected per frame regardless of cloud size
- **Depth colour**: 8 buckets from blue (far) to red (near) based on camera-Z distance
- **Dual mesh layers**: `_pcv_mesh` (imported model, blue) and `_pcv_recon` (Poisson result, orange) are independent series
- **Normals**: `_pcv_normals` orange line series — shown after "Preview Normals" in the Reconstruction window

## Session / window contract

`MainWindow._active_session` is the single source of truth for the running backend. All callbacks (`_on_cloud`, `_on_generate`, etc.) use it directly. `set_enabled()` / `update_point_count()` / `tick()` on `ReconstructionWindow` are called from `MainWindow` to keep the two windows in sync.
