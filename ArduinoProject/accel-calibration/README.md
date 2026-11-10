# Accel Calib

Calibration of a 3-axis MEMS accelerometer (ADXL355 or ADXL345) on an STM32 NUCLEO-WL55JC1.
The firmware captures the measurements; the PC app computes the fit.

1. The firmware streams the readings and runs the capture. For each orientation it sends the
   mean and standard deviation of the samples to the PC.
2. The PC app fits bias and scale (optionally non-orthogonality and alignment): a 6-point
   calibration: n independent orientations (6 or more), not necessarily aligned with the axes.
3. The correction is external: the parameters are read in the app's **Parameters** window. To
   correct on the board instead, its two C defines can be pasted into the firmware and flashed:

```
corrected = K (raw - bias)        K = M C S
```

## Projects

| Folder | Contents |
|---|---|
| [`remote-app/`](remote-app/README.md) | PC app: Python + DearPyGui, or the compiled `AccelCalib.exe` |
| [`x-nucleo/`](x-nucleo/README.md) | firmware: STM32CubeIDE project for the NUCLEO-WL55JC1 (STM32WL55) |
| `baseboard/` | empty for now |

Each project has its own `docs/` with its architecture diagram.

## Quick start

1. Firmware: import `x-nucleo` in STM32CubeIDE, set the sensor in
   `Application/App/Inc/app_calib_conf.h`, build and flash. Wiring: see its README.
2. PC app: run `remote-app/compiled/AccelCalib.exe`, or:

   ```
   cd remote-app
   python -m venv .venv
   .venv\Scripts\Activate.ps1            # Git Bash: source .venv/Scripts/activate
   pip install -r requirements.txt
   python main.py
   ```
3. Calibrate:
   - connect to the Nucleo's COM port, press **Start**;
   - place the board in n independent orientations (6 or more, each at least 30 deg from the
     others), keep it still, press **d** for each;
   - read the parameters in **Parameters**;
   - save the record;
   - optional, to correct on the board: copy the defines into `app_calib_conf.h`, rebuild and
     flash.


