# Accel Calib App

Desktop app that calibrates a 3-axis accelerometer (ADXL355 or ADXL345) on an STM32 Nucleo
running [`x-nucleo`](../x-nucleo/README.md). The board is placed in n independent
orientations (6 or more); the app fits bias and scale and shows the parameters, plus two C defines for
the firmware if the correction is to run on the board.

## Model

```
corrected = K (raw - bias)        K = M C S
```

| Term | Meaning | Estimated |
|---|---|---|
| `bias` | offset per axis | always |
| `S` | scale per axis | always |
| `C` | non-orthogonality between axes | optional: tick alpha, 9+ positions |
| `M` | misalignment sensor to board | optional: measure the references |

Model name: SFEM. Method: 6-point calibration. The fit uses only the magnitude of gravity
(|g| = 1 g in every orientation), so the positions only need to be independent: any n
orientations (6 or more for bias and scale), each at least 30 deg from the others (`APP_CALIB_MIN_SEPARATION_DEG`). They do
not need to be aligned with the axes. The **Status sphere** window shows the measured positions
on a sphere.

## Run the executable

Copy [`compiled/`](compiled/README.md) to a Windows PC and run `AccelCalib.exe`. Python is not
required.

## Setup after git clone

Requires Python 3.10+. The app uses its own virtual environment, `.venv`.

Windows (PowerShell):

```
git clone <repository url>
cd "<repository>\applications\accel-calibration\remote-app"
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r requirements.txt        # numpy, dearpygui, pyserial (pinned)
python main.py
```

- Git Bash: `source .venv/Scripts/activate`. Linux / macOS: `source .venv/bin/activate`.
- Activate the environment in every new terminal.

## Develop and build

```
pip install -r requirements-dev.txt    # pyinstaller, pyflakes
python -m pyflakes main.py src         # must print nothing
python build_exe.py                    # writes compiled/AccelCalib.exe
```

PyInstaller packs Python, the libraries and `src/` into one file. `build/` holds temporary
files and can be deleted.

## Calibration procedure

Default: 6 positions, bias and scale. At start the app loads the preset **Default**; save over
it in **Calibration settings** to change the start settings.

1. Connect the Nucleo by USB. In **Connection**, select the COM port, press **Connect**.
2. In **Calibration settings** check:
   - positions n (6 or more; 6 is the minimum for b and S), b and S ticked, references none;
   - samples k per position (more samples: less noise, longer capture).
3. Press **Start** (Controls). For each of the n positions: place the board in a new orientation, keep
   it still, press **d**; the board averages k samples. A position is kept if the board stayed still and it is at least 30 deg from the
   ones already kept; otherwise measure it again. The arrow in the **Status sphere** is
   green when the position will be accepted. **Calibration procedure** lists the mean and
   std of each measurement, and the temperature at the start and end.
4. The fit runs when the capture ends. **Parameters** shows the result:
   - external correction: read or copy the 6 parameters (bias and scale);
   - on-board correction: copy the defines into `app_calib_conf.h` and flash.
5. In **Records**, enter the sensor name, press **Save record**. Saved in `calibrations/<name>/`.

With 6 positions and 6 unknowns the fit is exact, so the |g| error after the fit is 0; with
more positions it is a least-squares fit and that error is the residual. To
verify a calibration, repeat it and compare the parameters, or check the live **Errors** (the
reading minus the nearest axis, so with the board resting near an axis).

Optional terms (Calibration settings):
- Non-orthogonality: tick alpha, 9+ positions including tilted ones.
- Alignment (references), measured with **d** after the positions:
  - "z up": corrects tilt. +Z reference: board flat on a level surface.
  - "z up + x up": also corrects heading. +X reference: board on its side.

| Key | Action (while connected) |
|---|---|
| d | measure this position |
| q | abort the capture |
| r | start the stream |
| s | stop the stream |

## Windows

| Window | Contents |
|---|---|
| Connection | serial port, Connect; **Stream** / **Stop** the board's readings |
| Controls | status, Start / Capture / Abort |
| Calibration settings | capture settings, estimated terms, presets (Default loaded at start; a preset also holds the display settings) |
| Calibration procedure | temperature now and at the start and end of the capture; a progress bar; mean and std of every position and reference (--- for those still to come) |
| Display settings | **Pause display** freezes Live stream, Tilt and Errors for zooming (the board keeps measuring); cal source, terms applied to the shown values, smoothing (optional: the mean of the last so many seconds), nearest axis reference |
| Live stream | x, y, z, \|g\|: raw, calibrated, nearest axis |
| Tilt | two modes: **roll / pitch**, or **axis angles**: the angle of each axis to gravity (calibrated: red/green/blue; raw: cyan/magenta/yellow) |
| Errors | reading minus nearest axis, before and after calibration |
| Status sphere | measured positions on a sphere; where the board points; whether d would accept. **Shading** (at the top): tint, highlight, and whether what is behind is drawn as rings, outlines and dashes |
| Parameters | the 12 fitted parameters, the C defines |
| Records | sensor ID, **Save record**; saved calibrations; Load |
| Temperature analysis | a parameter against temperature; merge calibrations at several temperatures into a temperature-compensated one |
| Board messages | the last text messages from the board |

At start the windows open in the first saved workspace (in order of name), or in the default
layout if there is none. **Workspaces > Default layout** restores the default.

The bar at the top of the app stays there:

- **Windows**: tick or untick a window to show or hide it; **Show all**, **Hide all**.
- **Workspaces**: **Save current as...** saves the place and size of every window, and which
  are hidden, under a name; **Delete**; **Default layout** shows every window in its default
  place.
- Then one item per saved workspace: click it and the windows move there at once. Windows that
  were docked become floating windows in the same places (DearPyGui reads the docking only at
  start). A workspace is scaled to the size of the screen it is applied on.

Workspaces are files in [`workspaces/`](workspaces/README.md), in the app's folder, so they ship with it; the executable has its own `compiled/workspaces/`. What DearPyGui keeps of the windows (docking, table columns) is in `app_layout.ini` there too, which is not in git.

## Folders

| Folder | Contents |
|---|---|
| [`src/`](src/README.md) | code, in layers |
| [`compiled/`](compiled/README.md) | executable, with its own records, presets and workspaces |
| [`docs/`](docs/README.md) | architecture diagram |
| [`calibrations/`](calibrations/README.md) | calibration records, one folder per sensor |
| [`presets/`](presets/README.md) | saved settings |
| [`workspaces/`](workspaces/README.md) | saved window arrangements (the Workspaces menu) |

Architecture: [`docs/diagrams/out/architecture.svg`](docs/diagrams/out/architecture.svg).
