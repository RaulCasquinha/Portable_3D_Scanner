# session

The state the app builds from the board's messages. No files, no port and no GUI.

| Module | Main item | Holds |
|---|---|---|
| `capture.py` | `Capture` | the positions and references of one calibration, their std and temperatures; `check_placement()` says whether d would be accepted now |
| `live_buffer.py` | `LiveBuffer` | the last seconds of readings, raw and calibrated by the board |
| `temperature_log.py` | `TemperatureLog` | the temperatures the sensor sent (empty without a thermometer) |
| `calibration.py` | `Calibration` | the calibration in use: model, terms, rms, and for a compensated one its temperature lines |
