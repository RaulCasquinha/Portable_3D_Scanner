# Util / PointGeometry

Converts a polar scanner measurement into a world-space Cartesian 3D point.

## Responsibility

Given a distance reading from the LiDAR, the device position, and the device orientation, computes the `(x, y, z)` world coordinates of the scanned surface point using a spherical-to-Cartesian transform:

```
dir_x = cos(pitch) * cos(yaw)
dir_y = cos(pitch) * sin(yaw)
dir_z = sin(pitch)

point = devicePos + distance * dir
```

Roll is intentionally omitted — it rotates around the sensor's pointing axis and does not change where a 1-D distance sensor aims.

## Dependencies

- `<math.h>` — `cosf`, `sinf`

## Usage

```cpp
#include "PointGeometry.h"

Position    pos    = {0, 0, 0};
Orientation orient = {pitch, roll, yaw};  // radians

Point3D p = toWorldPoint(distanceMm, pos, orient);
// p.x, p.y, p.z — world coordinates in mm
```
