# ADXL355 STM32WL bus interface

`adxl355_stm32wl` connects the [ADXL355 driver](..) to the STM32 HAL. It
provides the `ADXL355_Platform_t` platform services the driver needs: register
read and write over SPI or I2C, an SPI read with DMA, and a blocking delay.

| File | Content |
|---|---|
| `Inc/adxl355_stm32wl.h` | Public interface (Doxygen documented) |
| `Src/adxl355_stm32wl.c` | Implementation |

This is a platform module: unlike the driver, it includes the HAL header
(`stm32wlxx_hal.h`). For another STM32 family, change that include only.

## Features

- **SPI** (built when `HAL_SPI_MODULE_ENABLED` is defined): blocking read and
  write, and a DMA read. nCS is a GPIO driven by the module.
- **I2C** (built when `HAL_I2C_MODULE_ENABLED` is defined): blocking read and
  write with `HAL_I2C_Mem_Read/Write`.
- Every wait has a timeout: 10 ms per SPI transfer, 10 ms plus the transfer
  time at 100 kHz per I2C transfer, and 2 ms for a running DMA read to end. A
  HAL timeout is returned as `ADXL355_TIMEOUT`.
- Transfers are 1 to `ADXL355_FIFO_MAX_BYTES` bytes (a full FIFO read).

## Usage (SPI)

```c
#include "adxl355.h"
#include "adxl355_stm32wl.h"

// After MX_SPI1_Init() and MX_GPIO_Init() (nCS as output, high)
if (ADXL355_STM32WL_ConfigSpi(&hspi1, ADXL355_nCS_GPIO_Port,
                              ADXL355_nCS_Pin) != ADXL355_STM32WL_OK) {
    // Handle the error
}
const ADXL355_Platform_t *platform = ADXL355_STM32WL_GetSpiPlatform();

ADXL355_Status_t status = ADXL355_SetRange(platform, ADXL355_RANGE_2G);
```

## Usage (I2C)

```c
// ASEL low: 0x1D; ASEL high: 0x53
(void)ADXL355_STM32WL_ConfigI2c(&hi2c1, 0x1DU);
const ADXL355_Platform_t *platform = ADXL355_STM32WL_GetI2cPlatform();
```

## DMA read

`ADXL355_STM32WL_ReadSpiDma()` sends the command byte, starts the DMA reception
and returns at once. nCS stays low until the transfer ends. It is not
blocking, so it must **not** be used as `ADXL355_Platform_t.read`. It
is the start function for the driver's FIFO read with DMA:

```c
static uint8_t fifo_buffer[ADXL355_FIFO_MAX_BYTES];

status = ADXL355_ReadFifoDMA(ADXL355_STM32WL_ReadSpiDma, fifo_buffer, 96U);
// ... when ADXL355_STM32WL_IsSpiDmaBusy() returns false, use fifo_buffer
```

The module does not define the HAL callbacks, so other SPI devices can still
use them. The application calls the end handler from its own callbacks:

```c
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    ADXL355_STM32WL_SpiRxCpltHandler(hspi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    ADXL355_STM32WL_SpiRxCpltHandler(hspi);
}
```

`ADXL355_STM32WL_IsSpiDmaBusy()` returns `false` once the data is in the
buffer. A blocking transfer requested while a DMA read runs waits for it
for up to 2 ms (a full FIFO takes 0.3 ms at 8 MHz).

In full-duplex master mode the HAL sends the dummy bytes with DMA too, so the
SPI handle needs **both an RX and a TX DMA channel** (STM32CubeMX:
SPI > DMA Settings). Without the TX channel `HAL_SPI_Receive_DMA()` fails and
the function returns `ADXL355_SPI_RECEIVE_ERROR`.

## Limitations

- **One sensor per bus type at a time.** The driver callbacks carry no
  context, so the module keeps one SPI and one I2C configuration. With more
  than one sensor, call `ADXL355_STM32WL_ConfigSpi()` (or `...ConfigI2c()`)
  before switching to another sensor, while no DMA read runs.
- The I2C errors are returned as `ADXL355_SPI_RECEIVE_ERROR` and
  `ADXL355_SPI_SEND_ERROR`: the driver has no bus-neutral or I2C status.
- Not reentrant; the transfer functions are not for interrupt context.