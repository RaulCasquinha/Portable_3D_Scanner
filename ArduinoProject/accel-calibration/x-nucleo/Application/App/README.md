# Application/App

The application: library `APP`. `main.c` calls `APP_Init()` and `APP_Run()`; everything else
starts here.

| Module | Layer | Does |
|---|---|---|
| `app.c` / `app.h` | application | `APP_Run()`: the main loop. Takes the PC's commands, streams `$READ` (50 Hz) and `$TEMP` (1 Hz), runs a capture of `AccelCalib` and gives it its platform services |
| `app_protocol.c` / `.h` | protocol | builds every line sent to the PC and decodes the keys and `$START`; the list of lines is in `app_protocol.h` |
| `app_accel.c` / `.h` | service | the sensor chosen by `APP_SENSOR`: set up, read in g, read the temperature in mdegC |
| `app_link.c` / `.h` | transport | COM1 (ST-LINK VCOM, 115200 8N1): send bytes, receive one, wait for one, with timeouts |
| `app_log.c` / `.h` | debug | `LOG()`: debug text in Debug builds only, removed from Release |
| `app_calib_conf.h` | settings | the sensor, the capture settings of the `c` key, and the calibration constants to flash |

The protocol lines are sent by `app_protocol` through `app_link`, never through `LOG()`, so
they are also in Release builds. Their PC side is `remote-app/src/protocol/codec.py`,
with the same names and units.
