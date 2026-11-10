# gui/windows

One module per window. Each window is a `Window` subclass (`base.py`):
- `__init__` builds the window;
- `update(live)` redraws it every frame;
- `_changed()` skips redraws when nothing changed;
- `set_paused()` freezes or resumes a live window (Pause display in Display settings).

| Module | Window | Shows |
|---|---|---|
| `connection.py` | Connection | port, Refresh, Connect; Stream / Stop |
| `controls.py` | Controls | status, Start / Capture / Abort |
| `calibration_settings.py` | Calibration settings | positions, samples, references; terms and Refit; presets |
| `procedure.py` | Calibration procedure | temperatures; progress bar; mean and std of every position and reference (--- until measured) |
| `display_settings.py` | Display settings | Pause display (freezes the three live windows); cal source, applied terms, smoothing (optional, in s), reference |
| `stream.py` | Live stream | x, y, z, \|g\| readout and plots; the time scale |
| `tilt.py` | Tilt | two modes: roll / pitch, or the angle of each axis to gravity, raw and calibrated: readout and plots |
| `errors.py` | Errors | reading minus the nearest axis |
| `status_sphere.py` | Status sphere | the positions on a sphere that fills the window; whether d would accept now; Shading options |
| `parameters.py` | Parameters | the 12 parameters, rms, the C defines |
| `records.py` | Records | sensor ID, Save record; saved records with tick boxes; the selected one; Load |
| `temperature.py` | Temperature analysis | a parameter of the ticked records against temperature; New compensated calibration |
| `board_log.py` | Board messages | the board's last messages |
