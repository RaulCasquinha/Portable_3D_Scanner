# PythonProject

DearPyGUI host application — real-time 3D point cloud visualiser and mesh reconstructor.

## Running

```bash
# From the repo root:
nix-shell shell.nix
cd PythonProject
python3 main.py
```

The nix shell creates a Python 3.12 venv with all dependencies on first run.

## Files

| File | Description |
|------|-------------|
| `main.py` | Entry point — creates DPG context, builds windows, runs the render loop |
| `requirements.txt` | Python dependencies: dearpygui, numpy, pyserial, trimesh, open3d |

## Structure

```
PythonProject/
├── main.py
├── requirements.txt
└── src/
    ├── app/            ScanSession — real device backend
    ├── sim/            SimSession  — simulator backend
    ├── protocol/       Binary packet decoder (app_layer.py)
    ├── transport/      Non-blocking serial reader
    ├── data/           PointCloud accumulator + DevicePose
    ├── processing/     Mesh reconstruction (Poisson + normals)
    └── gui/            All DearPyGUI windows and views
```

See [`src/README.md`](src/README.md) for per-module details.
