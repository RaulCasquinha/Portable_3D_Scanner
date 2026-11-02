# src

All Python source modules, organised by concern.

## Module map

```
src/
├── app/            ScanSession — owns the serial connection lifecycle
├── sim/            SimSession  — synthetic data via ray-casting on a loaded mesh
├── protocol/       Packet decoder — byte stream → typed Python objects
├── transport/      Non-blocking serial I/O
├── data/           In-memory domain objects (point cloud, device pose)
├── processing/     Offline algorithms (Poisson reconstruction, normal estimation)
└── gui/            All DearPyGUI panels and views
```

## Dependency direction

```
gui  ──►  app / sim  ──►  protocol  ──►  transport
  └──►  data  ◄──────────────────────────────────
  └──►  processing
```

`gui` never imports from `protocol` or `transport` directly — it only talks to sessions and data objects.

## Sessions

Both `ScanSession` and `SimSession` expose the same callback interface:

| Callback | Fired when |
|----------|-----------|
| `on_cloud_update(pts)` | New point added to the cloud |
| `on_pose_update(pos, orient)` | Device pose changed |
| `on_ray_update(origins, endpoints)` | Ray set updated |
| `on_log(msg)` | Log message received |
| `on_status(msg)` | Connection status changed |
| `on_sensor_status(lidar_ok, imu_ok)` | Boot sensor report |

`MainWindow._active_session` holds the currently active backend; only one runs at a time.
