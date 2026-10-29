# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

TFM (Trabalho Final de Mestrado) — a handheld 3D scanner built on an ESP32-S3. The scanner combines a VL53L0X ToF LiDAR with an LSM6DS3 IMU to capture distance + orientation pairs, converts them to 3D points on the microcontroller, and streams them over USB-Serial to a host PC.

## Development environment

The repo ships a Nix shell that provides Arduino IDE in an FHS environment:

```bash
nix-shell shell.nix   # opens Arduino IDE
```

Arduino IDE is the primary build tool. There is no CLI build system yet; compiling and flashing is done through the IDE targeting the **ESP32-S3** board.

Required Arduino libraries (install via Library Manager):
- **Pololu VL53L0X** — ToF sensor driver
- **SparkFun LSM6DS3** — IMU driver

## Architecture

The firmware follows a strict layered architecture defined in `docs/diagrams/software_architecture.d2`. All source lives under `ArduinoProject/`:

```
TFM.ino  (App — orchestrates setup/loop)
└── src/
    ├── HAL/LiDAR/   — VL53L0X driver; returns distance in mm
    ├── HAL/IMU/     — LSM6DS3 driver; integrates gyro → pitch/roll/yaw (radians)
    ├── Util/PointGeometry/  — toPoint3D(): spherical → Cartesian (mm)
    └── Data/
        ├── AppLayer/        — encodes Point3D as 15-byte binary packet
        └── TransportLayer/  — wraps Serial.write()
```

**Data flow:** `HAL_LiDAR` + `HAL_IMU` → `toPoint3D()` → `Data_AppLayer::sendPoint()` → `Data_TransportLayer::send()` → USB-Serial.

**Packet format** (AppLayer → TransportLayer):
```
0xAA 0xBB | x(4B) y(4B) z(4B) | CRC8(1B)   — 15 bytes total
```
Floats are IEEE 754 little-endian. CRC-8/SMBUS covers the 12 payload bytes.

**IMU orientation** is accumulated by pure gyroscope integration (deg/s × dt). This drifts over time; long-session accuracy requires a Madgwick or Mahony filter fusing the accelerometer.

## Report

`docs/report/paper.tex` is the LaTeX source. Build with:

```bash
cd docs/report && latexmk -pdf paper.tex
```

Bibliography is in `docs/report/bibliography/references.bib`.

## Repository layout notes

- `3DModels/` — FreeCAD source + STL + G-code for the printed enclosure; `ExtraModels/` holds downloaded STEP/SolidWorks component models used as visual references only.
- `PythonProject/` — future host-side point cloud processing (currently empty).
- `docs/diagrams/software_architecture.d2` — authoritative architecture diagram; keep it in sync when adding modules.
