# ADXL345 STM32WL bus interface

`adxl345_stm32wl` connects the [ADXL345 driver](..) to the STM32 HAL. It
provides the `ADXL345_Platform_t` platform services the driver needs: register
read and write over SPI or I2C, and a blocking delay.

| File | Content |
|---|---|
| `Inc/adxl345_stm32wl.h` | Public interface (Doxygen documented) |
| `Src/adxl345_stm32wl.c` | Implementation |

This is a platform module: unlike the driver, it includes the HAL header
(`stm32wlxx_hal.h`). For another STM32 family, change that include only.

## Features

- **SPI** (built when `HAL_SPI_MODULE_ENABLED` is defined): blocking read and
  write. CS is a GPIO driven by the module. The command byte is
  `[R/W][MB][A5..A0]`, with MB set for transfers of more than one byte.
- **I2C** (built when `HAL_I2C_MODULE_ENABLED` is defined): blocking read and
  write with `HAL_I2C_Mem_Read/Write`.
- Every wait has a timeout: 10 ms per SPI transfer and 10 ms plus the
  transfer time at 100 kHz per I2C transfer. A HAL timeout is returned as
  `ADXL345_TIMEOUT`.
- Transfers are 1 to 64 bytes (`ADXL345_STM32WL_MAX_TRANSFER_BYTES`, the
  6-bit address space).

## Usage (SPI)

The ADXL345 SPI is CPOL = 1, CPHA = 1, at most 5 MHz (CubeMX: CPOL High,
CPHA 2 Edge, software NSS).

```c
#include "adxl345.h"
#include "adxl345_stm32wl.h"

// After MX_SPI1_Init() and MX_GPIO_Init() (CS as output, high)
if (ADXL345_STM32WL_ConfigSpi(&hspi1, ADXL_NCS_GPIO_Port,
                              ADXL_NCS_Pin) != ADXL345_STM32WL_OK) {
    // Handle the error
}
const ADXL345_Platform_t *platform = ADXL345_STM32WL_GetSpiPlatform();

ADXL345_Status_t status = ADXL345_SetRange(platform, ADXL345_RANGE_16G);
```

## Usage (I2C)

```c
// ALT ADDRESS low: 0x53; ALT ADDRESS high: 0x1D
(void)ADXL345_STM32WL_ConfigI2c(&hi2c1, ADXL345_I2C_ADDRESS_ALT_LOW);
const ADXL345_Platform_t *platform = ADXL345_STM32WL_GetI2cPlatform();
```

## Limitations

- **No DMA read.** Unlike the ADXL355, the ADXL345 FIFO is read one sample
  set (6 bytes) per transaction, so a DMA FIFO read would not help.
- **One sensor per bus type at a time.** The driver callbacks carry no
  context, so the module keeps one SPI and one I2C configuration. With more
  than one sensor, call `ADXL345_STM32WL_ConfigSpi()` (or `...ConfigI2c()`)
  before switching to another sensor.
- The I2C errors are returned as `ADXL345_SPI_RECEIVE_ERROR` and
  `ADXL345_SPI_SEND_ERROR`: the driver has no bus-neutral or I2C status (same
  as the ADXL355 driver).
- The I2C part is not compiled in this project (I2C is not enabled in the
  .ioc), so it is untested.
- Not reentrant; the transfer functions are not for interrupt context.
