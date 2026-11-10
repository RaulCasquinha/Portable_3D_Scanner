# processing

A reusable library for accelerometer calibration: **numpy only**. It knows nothing of files,
ports or the GUI, so it can be copied into another project or tested in a Python shell.

| Module | Main item | Does |
|---|---|---|
| `sfem.py` | `CalibrationModel` | the correction `K (raw - b)`, `K = M C S`; the 12 parameters as a vector |
| `ellipsoid_fit.py` | `fit_ellipsoid()` | b, S and alpha from still readings, in closed form |
| `alignment.py` | `fit_alignment()` | the misalignment angles from the +Z / +X references |
| `regression.py` | `LinearTrend` | a least-squares line per column of values against one variable |
| `compensation.py` | `TemperatureCompensation` | the 12 parameters as lines in temperature |
| `geometry.py` | `roll_pitch_deg()`, `axis_angles_deg()`, `snap_to_axis()` | vector helpers: tilt angles, the angle of each axis to gravity, the nearest axis |
| `statistics.py` | `rms_magnitude_error_mg()` | how far the readings are from 1 g |
| `placement.py` | `is_new_position()` | the checks the board makes before accepting a position |

```python
from src.processing.ellipsoid_fit import fit_ellipsoid
model = fit_ellipsoid(readings_g)          # (n, 3) still readings, in g
corrected_g = model.apply(readings_g)
```
