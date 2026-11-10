# Application

All the code that STM32CubeMX doesn't generate, so a new code generation can't overwrite it.
`App/` is the application; every other folder is a library that the application uses.

| Folder | Contents | Uses |
|---|---|---|
| [`App/`](App/README.md) | the application (library `APP`): main loop, protocol, sensor, COM1 | the libraries, the HAL |
| [`AccelCalib/`](AccelCalib/README.md) | the guided capture and the correction `K (raw - b)` | standard C only |
| [`ADXL355/`](ADXL355/README.md) | ADXL355 driver, and its STM32WL platform in `stm32wl/` | standard C (`stm32wl/`: the HAL) |
| [`ADXL345/`](ADXL345/README.md) | ADXL345 driver, and its STM32WL platform in `stm32wl/` | standard C (`stm32wl/`: the HAL) |

A library never includes `App/`, `Core/` or HAL headers (rules SRC-09, SRC-10): it gets the
hardware through a struct of function pointers that the application fills. Each folder has
`Inc/` (public headers) and `Src/`.
