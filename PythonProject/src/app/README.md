# app — Real device session

Manages the USB-serial connection to the ESP32-S3.

## Files

| File | Description |
|------|-------------|
| `scan_session.py` | `ScanSession` — starts/stops the serial transport, feeds bytes through the protocol decoder, and fires GUI callbacks |

## Responsibilities

- Lists available serial ports (`available_ports()`)
- Starts a background read thread via `SerialTransport`
- Decodes incoming bytes with `AppLayerDecoder` → `PointPacket` / `PosePacket` / `LogPacket`
- Accumulates points into `PointCloud`
- Fires `on_cloud_update`, `on_pose_update`, `on_ray_update`, `on_log`, `on_sensor_status`
- Handles USB-CDC reconnection (device reboot mid-scan)
- `save_cloud(path)` — writes current cloud to a PLY file
- `reset_reference()` — sets current pose as the new origin
