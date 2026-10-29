# Util / PointGeometry

Geometric utility that converts a polar scanner measurement into a Cartesian 3D point.

## Responsibility

Given a distance reading from the LiDAR and an orientation from the IMU, computes the `(x, y, z)` coordinates of the scanned surface point using a spherical-to-Cartesian transform:

```
x = d · cos(pitch) · cos(yaw)
y = d · cos(pitch) · sin(yaw)
z = d · sin(pitch)
```

Roll is intentionally ignored here; it can be applied as a post-processing correction on the host side.

## Dependencies

- `<math.h>` — `cosf`, `sinf`

## Usage

```cpp
#include "PointGeometry.h"

Point3D p = toPoint3D(distanceMm, orient.pitch, orient.yaw);
```
