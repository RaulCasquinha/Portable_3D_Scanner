# services

The app's use cases, as functions without state.

| Module | Does |
|---|---|
| `calibrate.py` | `calibrate()`: a finished capture -> a `Calibration` (ellipsoid fit, then the alignment) |
| `merge.py` | `compensated_record()`: simple records of one sensor at several temperatures -> a temperature compensated record |
| `export.py` | `c_defines()`: a model -> `ACCEL_CAL_BIAS_G` and `ACCEL_CAL_K` for `app_calib_conf.h` |
| `report.py` | the monospace texts: the measurement table, record rows and record details; `PLACEHOLDER` (`---`) where a value is not there yet |
