# Portable 3D Scanner

Handheld 3D scanner built on an **ESP32-S3**. Combines a **VL53L0X** ToF LiDAR with an **LSM6DS3** IMU to capture distance, position + orientation data, converts them to world-space 3D points on the microcontroller, and streams them over USB-Serial to a host PC where a DearPyGUI Python app accumulates, displays, and reconstructs the point cloud in real time.

---

## Hardware

| Component | Role |
|-----------|------|
| ESP32-S3 | Main MCU |
| VL53L0X | Time-of-Flight LiDAR |
| LSM6DS3 | 6-axis IMU |

Both sensors share the same I²C bus.

The scanner works with either sensor missing — graceful degradation:

| LiDAR | IMU | Mode |
|-------|-----|------|
| OK | OK | Full scan — world-space points + pose tracking |
| OK | missing | LiDAR only — points placed with sensor at fixed origin |
| missing | OK | Pose tracking only — no point capture |
| missing | missing | No-op |

---

## Firmware architecture

```
PortableScanner3D.ino   (orchestrates setup/loop, accumulator)
└── src/
    ├── HAL/LiDAR/      HAL_LiDAR  — Pololu VL53L0X driver; distance in mm
    ├── HAL/IMU/        HAL_IMU    — Arduino_LSM6DS3 driver; gyro → orientation; accel → position
    ├── Util/PointGeometry/        toWorldPoint(): spherical → Cartesian (mm)
    └── Data/
        ├── AppLayer/   Data_AppLayer  — encodes typed binary packets
        └── TransportLayer/            — wraps Serial.write()
```

**Data flow:**

```
HAL_IMU → Orientation + Position ─┐
                                   ├─► toWorldPoint() → Point3D ─► Data_AppLayer → USB-Serial
HAL_LiDAR → distance (mm) ────────┘
```

### Scan-and-average accumulator

Raw LiDAR readings are noisy. Instead of transmitting every sample, the firmware accumulates `ACC_N = 5` consecutive valid distance readings and transmits their mean as a single point.

When the IMU is connected, an early-flush condition checks whether the device has **rotated more than 1°** on any axis **or translated more than 2 mm** since accumulation started. If either threshold is exceeded, the partial average is flushed immediately and a new window opens at the new pose. This ensures each transmitted point corresponds to a single physical position and direction.

---

## Binary packet protocol

All packets share the same wire framing over USB-CDC Serial at 115 200 baud:

```
[ 0xAA | 0xBB ] [ TYPE : 1 B ] [ LEN : 1 B ] [ PAYLOAD : LEN B ] [ CRC8 : 1 B ]
```

CRC-8/SMBUS (poly 0x07, init 0x00) is computed over `TYPE + LEN + PAYLOAD`.

| Type | ID | Payload | Description |
|------|----|---------|-------------|
| POINT | 0x01 | 24 B | `x y z` (surface point, mm) + `ox oy oz` (sensor origin at measurement time, mm) |
| POSE | 0x02 | 24 B | `px py pz` (position, mm) + `pitch roll yaw` (radians) |
| LOG | 0x03 | variable | UTF-8 status/log string |

All multi-byte fields are IEEE 754 little-endian floats.

The sensor origin `(ox, oy, oz)` embedded in each POINT packet is the device world position at the start of the accumulation window.

---

## Algorithms

### Ray-based point cloud accumulation

The POINT packet carries both the **surface point** and the **sensor origin**, which together define a **measurement ray**: a directed line from sensor to surface. The Python host uses these rays to merge redundant measurements intelligently rather than storing every raw reading:

1. For each incoming point, compute the ray direction: `normalize(surface_point − sensor_origin)`.
2. Search all stored clusters where **all three** conditions hold:
   - **Ray proximity** — the new sensor position lies within **5 mm** of the cluster's ray *line* (not just near the origin). This allows the sensor to approach or retreat along the same ray without breaking the cluster.
   - **Direction match** — ray directions agree within **1.15°** (dot product > 0.9998).
   - **Surface proximity** — the new surface point lands within **15 mm** of the cluster's current average surface point. This prevents an occluding object in front of an already-measured surface from being folded into the wrong cluster.
3. If a match is found, fold the new distance into that cluster's running mean. Among multiple matches, the cluster whose ray line passes closest to the new sensor is preferred.
4. If no match exists, start a new cluster.

The final point cloud contains one **averaged centroid per unique ray cluster**. Re-scanning the same surface from anywhere along the same ray sharpens accuracy; a different direction or a different surface creates a separate cluster. Measuring a surface from behind produces an opposite direction vector (dot ≈ −1) and never merges with front-side measurements.

Without IMU (sensor origin always `(0,0,0)`) ray merging is disabled and points accumulate as-is.

### Surface reconstruction (Poisson)

The **Generate** button reconstructs a triangle mesh from the point cloud using **Screened Poisson Surface Reconstruction**:

1. **Pre-processing** — voxel downsampling (0.5 % of bounding-box diagonal) to remove redundant overlapping clusters; statistical outlier removal (2σ, 20 neighbours) to discard stray measurements.
2. **Normal assignment** — each downsampled point inherits the outward normal of its nearest original cluster. The outward normal is the negated ray direction (sensor→surface flipped to surface→sensor), which is guaranteed to face the correct side for exterior scanning.
3. **Poisson solve** — depth is chosen automatically from the cleaned point count so sparse clouds get a simpler surface instead of noisy high-frequency geometry:

   | Cleaned points | Poisson depth | Feature scale |
   |---------------|--------------|---------------|
   | < 320 | 5 | coarse |
   | 320 – 2 500 | 6 – 7 | medium |
   | 2 500 – 20 000 | 7 – 8 | fine |
   | > 20 000 | 9 | very fine |

4. **Laplacian smoothing** — 5 iterations smooth out residual noise from sparse or noisy input without discarding valid low-density regions.

The result is a watertight mesh exported as STL.

### 3D projection

The viewer uses a custom perspective projection rather than DearPyGUI's built-in equal-aspect mode (which fights manual camera control):

- Camera defined by **azimuth**, **elevation**, **distance** around a **focus point**.
- Each frame: `eye = focus + spherical_offset`, `fwd = -offset/|offset|`, `right = fwd × up_ref`, `up = right × fwd`.
- Points projected as `sx = cam_x / cam_z * focal`, `sy = cam_y / cam_z * focal` where `focal = min(w, h) * 0.7`.
- Axis limits are locked every frame by `dpg.set_axis_limits` to prevent DearPyGUI's auto-fit from overriding the manual orbit.
- **Depth colouring** — the cloud is split into 8 series bucketed by `cam_z` (distance from camera along view direction); each series gets a colour from blue (far) to red (near).

---

## Python host application

`PythonProject/` — real-time point cloud visualiser and mesh reconstructor built with **DearPyGUI**.

### Running

```bash
nix-shell shell.nix          # sets up Python 3.12 venv with all dependencies
cd PythonProject
python3 main.py
```

### Features

- Dockable, resizable windows — layout saved to `layout.ini`
- **3D Point Cloud** panel:
  - Orbit camera: left-drag
  - Pan: Shift + left-drag (translates the focus point)
  - Zoom: scroll wheel
  - Depth-coloured points (blue = far, red = near)
  - Adjustable point size (1–20 px slider)
  - Ground-plane grid ±500 mm, 100 mm spacing
  - Hover annotation: world coordinates + measurement distance
- **Show / Hide** toggles for cloud, measurement rays, imported model wireframe, reconstruction wireframe
- **Live Info** panel — position (mm), orientation (°), cluster count, last LiDAR distance
- **Log** panel — device boot messages, warnings, errors
- **Orbit center** — drag-float controls to shift the camera focus point; reset to origin
- **Controls** toolbar (two rows):
  - Row 1: Port, Refresh, Connect, Disconnect, Simulator, status, point count, sensor status
  - Row 2: Reset Reference, Stop/Start, Save PLY, **Generate**, Auto (re-generate every 5 s), **Show mesh**, **→ STL**, point size, Cloud/Rays toggles, Ctr (orbit center)

### Reconstruction window

Click **Reconstruction** in the toolbar to open the dedicated reconstruction panel:

- **Normal Estimation** — grouped-PCA parameters: search radius (% of bbox diagonal), max neighbours, direction filter angle
- **Preview Normals** — computes and renders the surface normals as orange lines in the 3D viewport; length is adjustable
- **Poisson** — depth slider (0 = auto-recommended from point count), Laplacian smoothing iterations
- **Generate** — runs the full Poisson pipeline; overlays the wireframe in orange
- **Auto** — re-generates every 5 seconds as more points arrive
- **Show mesh** / **→ STL** — toggle visibility or save the last mesh to a file

### Simulator

The **Simulator** panel generates synthetic scan data without a physical device.

- **Load STL / OBJ** — import any mesh; the virtual device automatically orbits at a sensible distance; the mesh wireframe is overlaid in blue
- **Show model** toggle — show/hide the imported mesh wireframe (independent from the reconstruction wireframe)
- **Noise controls** — Gaussian noise on distance (mm) and orientation (°)
- **Device position** — manual azimuth, elevation, distance sliders
- **Auto orbit** — rotates the virtual device continuously at a chosen speed; elevation bounces between configurable min/max
- **Fast-forward** — up to 500× real-time for rapid point accumulation

### Python architecture

```
main.py
└── src/
    ├── app/scan_session.py             serial → protocol → cloud → GUI callbacks
    ├── sim/sim_session.py              simulator; same interface as ScanSession
    ├── protocol/app_layer.py           byte-stream packet decoder
    ├── transport/serial_transport.py   non-blocking serial read
    ├── data/
    │   ├── point_cloud.py              ray-cluster accumulator + to_numpy_with_normals()
    │   └── device_pose.py              latest POSE packet values
    ├── processing/
    │   └── reconstruction.py           grouped-PCA normals + Poisson + STL export
    └── gui/
        ├── main_window.py              toolbar, session switching, docking host
        ├── point_cloud_view.py         3D projection, orbit/pan/zoom, depth colour, dual mesh layers, normal vectors
        ├── reconstruction_window.py    dedicated reconstruction control panel
        ├── simulator_window.py         STL loader, noise/orbit controls
        ├── device_view.py              Live Info panel
        └── log_view.py                 scrollable log panel
```

Each directory has its own `README.md` with per-file descriptions.

---

## Repository layout

Each directory contains a `README.md` with per-file descriptions.

```
Portable_3D_Scanner/
├── 3DModels/                        FreeCAD source, STL, G-code for printed enclosure
│   └── ExtraModels/                 Downloaded STEP/SolidWorks component reference models
├── docs/
│   ├── datasheets/                  VL53L0X and ESP32-S3 datasheets (PDF)
│   ├── diagrams/                    D2 architecture diagram source + rendered SVG
│   └── report/
│       ├── paper.tex                LaTeX report source
│       ├── paper.pdf                Compiled thesis PDF
│       └── bibliography/references.bib
├── ArduinoProject/
│   ├── Doxyfile                     Doxygen configuration
│   ├── generate_docs.sh             API doc generator script
│   └── PortableScanner3D/
│       ├── PortableScanner3D.ino    main sketch (setup/loop/accumulator)
│       └── src/                     layered firmware modules
├── PythonProject/
│   ├── main.py
│   ├── requirements.txt             dearpygui, numpy, pyserial, trimesh, open3d
│   └── src/                         see Python architecture above
├── shell.nix                        Nix dev shell (Arduino IDE + Python 3.12 venv)
├── .gitignore
└── README.md
```

---

## Building

### Firmware

Open Arduino IDE via `nix-shell shell.nix`. Install these libraries via Library Manager:

- **VL53L0X** by Pololu
- **Arduino_LSM6DS3** by Arduino

Target board: **ESP32-S3**. Set **Tools → USB CDC On Boot → Enabled** so `Serial` maps to USB-CDC.

### Python dependencies

The nix shell creates a Python 3.12 venv automatically and pip-installs all dependencies on first run. open3d requires Python 3.12 — it has no wheel for 3.13. Always enter the shell from the repo root:

```bash
cd Portable_3D_Scanner
nix-shell shell.nix
```

### Report

```bash
cd docs/report && latexmk -pdf paper.tex
```
