# ADXL345 driver

Platform-independent C driver for the Analog Devices
[ADXL345](https://www.analog.com/en/products/adxl345.html) 3-axis
accelerometer, built after the ADXL355 driver (same platform struct, status
values, naming and documentation style).

The driver uses only standard C headers (C11).

| File | Content |
|---|---|
| `Inc/adxl345.h`, `Src/adxl345.c` | Register-level driver (`ADXL345_*`) |
| `Inc/adxl345_util.h`, `Src/adxl345_util.c` | Utilities built on it (`ADXL345_UTIL_*`) |
| `stm32wl/` | STM32 HAL platform services, SPI and I2C (`ADXL345_STM32WL_*`) |

## Terms

- **sample**: one axis measurement (2 bytes, LSB first);
- **sample set**: one X, Y, Z measurement (6 bytes, DATAX0 to DATAZ1).

## Register-level driver (`adxl345.h`)

| Area | Functions |
|---|---|
| Register access | `ReadRegister`, `WriteRegister`, `ReadRegisterMasked`, `WriteRegisterMasked` |
| Identification | `GetDevId`, `CheckDevId` |
| Data | `ReadRawSampleSet` |
| FIFO | `Get/SetFifoMode`, `Get/SetFifoSamples` (watermark), `SetFifoTrigger`, `ReadNumberFifoEntries`, `GetFifoTriggered`, `ReadFifoSampleSet` (see below) |
| Offsets | `GetAxisOffset`, `SetAxisOffset` |
| Activity detection | `Get/SetActivityThreshold`, `EnableActivity`, `DisableActivity`, `Get/SetInactivityThreshold`, `Get/SetInactivityTime`, `EnableInactivity`, `DisableInactivity` |
| Tap detection | `Get/SetTapThreshold`, `SetTapDuration`, `SetTapLatency`, `SetTapWindow`, `SetTapAxes`, `GetActTapStatus` |
| Free-fall detection | `SetFreeFallThreshold`, `SetFreeFallTime` |
| Interrupts (INT1, INT2) | `Get/SetInterruptPolarity`, `Get/SetInterruptMap`, `MapInterrupt`, `Get/SetInterruptEnable`, `ClearInterrupts`, `EnableInterrupt`, `DisableInterrupt`, `GetInterruptSource` |
| Configuration | `Get/SetRange`, `Get/SetODR`, `SetLowPower`, `Get/SetFullResolution`, `SetSelfTest`, `Get/SetMode`, `SetLinkAutoSleep`, `SetSleep` |

## Utilities (`adxl345_util.h`)

| Function | Purpose |
|---|---|
| `ADXL345_UTIL_Config` | Checks DEVID and applies a full configuration (`ADXL345_UTIL_DevCfg_t`) in standby, then sets the mode last |
| `ADXL345_UTIL_UnpackSampleSets` | Raw sample sets to sign-extended `int16_t` |
| `ADXL345_UTIL_SampleSets2Gs` | Unpacked sample sets to g (`float`) for a range |
| `ADXL345_UTIL_Threshold2LSB` | Threshold in mg to the THRESH_TAP/ACT/INACT/FF value |
| `ADXL345_UTIL_Offset2LSB` | Axis offset in mg to the OFSX/OFSY/OFSZ value |
| `ADXL345_UTIL_ClearFifo` | Empties the FIFO by reading the stored sample sets (testing and debugging) |

## General notes

Every function takes the `ADXL345_Platform_t` as its first argument and
returns an `ADXL345_Status_t`.

Input parameters are checked (`ADXL345_INVALID_PARAMETER` for NULL pointers,
missing callbacks, reserved registers 0x01 to 0x1C and invalid values).
Getters only write their output on success.

Setters of a register field use a read-modify-write and keep the other bits
(`WriteRegisterMasked()` rejects data bits outside the mask).

**Interrupt sources** (`ADXL345_INT_*`) and **axes** (`ADXL345_AXIS_*_EN`)
are unsigned bit-mask defines and can be OR-ed. INT_ENABLE, INT_MAP and
INT_SOURCE share the same bit layout; in INT_MAP a 1 routes the source to
**INT2**, a 0 to **INT1**.

Every datasheet value in `adxl345.h` has a `Ref:` line with its page and
table (ADXL345 datasheet Rev. G).

## Integration

1. Add `Inc` to the include path and compile `Src/adxl345.c` and
   `Src/adxl345_util.c`.
2. Provide an `ADXL345_Platform_t` with blocking `read` and `write`
   functions. On an STM32, use [`stm32wl`](stm32wl) (add `stm32wl/Inc` and
   `stm32wl/Src/adxl345_stm32wl.c`).
3. Configure the sensor with `ADXL345_UTIL_Config()` and read samples with
   `ADXL345_ReadRawSampleSet()` or `ADXL345_ReadFifoSampleSet()`.

## Platform services

```c
typedef struct {
    ADXL345_Status_t (*read)(const uint8_t address, uint8_t *buffer,
                             const uint16_t data_size);
    ADXL345_Status_t (*write)(const uint8_t address, const uint8_t *buffer,
                              const uint16_t data_size);
    void (*delay_ms)(uint32_t delay_ms);
} ADXL345_Platform_t;
```

`address` is the register address and `data_size` counts **data bytes only**.
The platform adds the command byte (SPI: `[R/W][MB][A5..A0]`, with MB set
when `data_size > 1` so the address auto increments) or the I2C addressing.

`read` must be **blocking**: the driver uses the buffer, often a local
variable, as soon as the call returns.

`delay_ms` keeps the struct identical to the ADXL355 one. No ADXL345 function
uses it (the ADXL345 has no reset to wait for), so it may be NULL.

## FIFO

The FIFO is read through DATAX0..DATAZ1 and each read pops one sample set, so
`ADXL345_ReadFifoSampleSet()` does one 6-byte transaction per set. There is
no DMA FIFO read as in the ADXL355 driver. The datasheet needs at least 5 us
between reads (CS high); above 1.6 MHz SPI the time between transactions must
cover it.

## Example (stm32wl)

```c
#include "adxl345.h"
#include "adxl345_util.h"
#include "adxl345_stm32wl.h" // or another ADXL345_Platform_t

// After ADXL345_STM32WL_ConfigSpi() (see stm32wl/README.md)
const ADXL345_Platform_t *platform = ADXL345_STM32WL_GetSpiPlatform();

ADXL345_UTIL_DevCfg_t config = {
    .platform = platform,
    .odr = ADXL345_ODR_100Hz,
    .range = ADXL345_RANGE_16G,
    .full_res = true,
    .act_coupling = ADXL345_DC_COUPLED,
    .inact_coupling = ADXL345_DC_COUPLED,
    .int_pol = ADXL345_INT_POL_HIGH,
    .int_enable = ADXL345_INT_DATA_READY, // on INT1 (int_map = 0)
    .fifo_mode = ADXL345_FIFO_BYPASS,
    .fifo_trigger = ADXL345_INT1,
    .meas_mode = ADXL345_MEASURE
};

ADXL345_Status_t status = ADXL345_UTIL_Config(&config);

ADXL345_RawSampleSet_t raw;
ADXL345_UTIL_UnpackedSampleSet_t sample;
ADXL345_UTIL_SampleSetGs_t sample_g;

if (status == ADXL345_OK) {
    status = ADXL345_ReadRawSampleSet(platform, &raw);
}
if (status == ADXL345_OK) {
    status = ADXL345_UTIL_UnpackSampleSets(&raw, 1U, &sample);
}
if (status == ADXL345_OK) {
    status = ADXL345_UTIL_SampleSets2Gs(&sample, ADXL345_RANGE_16G, true, 1U,
                                        &sample_g);
}
```
