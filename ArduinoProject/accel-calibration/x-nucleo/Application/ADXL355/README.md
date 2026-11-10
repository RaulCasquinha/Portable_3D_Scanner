# ADXL355 driver

Platform-independent C driver for the Analog Devices
[ADXL355](https://www.analog.com/en/products/adxl355.html) low-noise 3-axis
accelerometer.

The driver uses only standard C headers (C11).

| File | Content |
|---|---|
| `Inc/adxl355.h`, `Src/adxl355.c` | Register-level driver (`ADXL355_*`) |
| `Inc/adxl355_util.h`, `Src/adxl355_util.c` | Utilities built on it (`ADXL355_UTIL_*`) |
| `Doxyfile` | Doxygen configuration of the driver and `stm32wl` (run `doxygen` in this folder; this README is the start page) |


## Terms

The ADXL355 datasheet uses *sample*, *entry* and *single acceleration data* for one axis measurement.

Terms used in this driver:

- **entry** or **sample**: one axis measurement (3 bytes, 20 bits);
- **sample set**: one X, Y, Z measurement (9 bytes).

## Register-level driver (`adxl355.h`)

| Area | Functions |
|---|---|
| Register access | `ReadRegister`, `WriteRegister`, `ReadRegisterMasked`, `WriteRegisterMasked` |
| Identification | `GetDevIdAd`, `GetDevIdMst`, `GetPartId`, `GetRevId`, `GetAllIds` |
| Data | `ReadRawSampleSet`, `ReadRawTemperature` |
| FIFO | `GetFifoSamples`, `SetFifoSamples` (watermark), `ReadNumberFifoEntries`, `ReadFifo`, `ReadFifoSampleSet`, `ReadFifoDMA` (see below) |
| Offsets | `GetAxisOffset`, `SetAxisOffset` |
| Activity detection | `Get/SetActivityThreshold`, `Get/SetActivityCount`, `EnableActivity`, `DisableActivity` |
| Interrupts (INT1, INT2) | `Get/SetInterruptPolarity`, `Get/SetInterruptMap`, `EnableInterrupt`, `DisableInterrupt`, `ClearInterrupts` |
| Configuration | `Get/SetRange`, `Get/SetODR`, `Get/SetHPF`, `Get/SetMode` |
| Reset | `SoftReset` |

## Utilities (`adxl355_util.h`)

| Function | Purpose |
|---|---|
| `ADXL355_UTIL_Config` | Applies a full configuration (`ADXL355_UTIL_DevCfg_t`) in standby, then sets the mode last |
| `ADXL355_UTIL_UnpackSampleSets` | Raw 20-bit sample sets to sign-extended `int32_t` |
| `ADXL355_UTIL_SampleSets2Gs` | Unpacked sample sets to g (`float`) for a range |
| `ADXL355_UTIL_ActivityThreshold2LSB` | Activity threshold in mg to the ACT_THRESH value |
| `ADXL355_UTIL_Offset2LSB` | Axis offset in mg to the OFFSET_x value |
| `ADXL355_UTIL_ClearFifo` | Empties the FIFO by reading 96 entries (testing and debugging) |
| `ADXL355_UTIL_AlignFifo` | Discards FIFO entries until the next entry read is an X entry (one sample set is lost) |
| `ADXL355_UTIL_ReadTemperature`, `ADXL355_UTIL_ReadTemperatureMdegc` | Temperature in degC (`float`) or mdegC (integer) |
| `ADXL355_UTIL_ResetAndVerify` | Soft reset, then waits for NVM_BUSY and checks the shadow registers |


## General notes

Every function takes the `ADXL355_Platform_t` as its first argument and
returns an `ADXL355_Status_t`.

The application initializes `ADXL355_Platform_t` (bus and delay function) first. For the
STM32WL, [`stm32wl`](stm32wl) provides it for SPI and I2C.

Input parameters are checked: NULL pointers and invalid values return
`ADXL355_INVALID_PARAMETER`.

Setters of a register field use read-modify-write and keep the other bits.

**Interrupt sources** (`ADXL355_IntSource_t`) are INT_MAP bit masks and can be
combined: bits 0 to 3 map DATA_READY, FIFO_FULL, FIFO_OVR and ACTIVITY to
**INT1**, bits 4 to 7 map them to **INT2**.

`EnableInterrupt()` and `DisableInterrupt()` change only the given bits. `SetInterruptMap()` writes
the whole register.

## Integration

1. Add `Inc` to the include path and compile `Src/adxl355.c` and
   `Src/adxl355_util.c`.
2. Provide an `ADXL355_Platform_t` with blocking `read` and `write` functions
   and a `delay_ms` function. On an STM32, use
   [`stm32wl`](stm32wl) (add `stm32wl/Inc` and
   `stm32wl/Src/adxl355_stm32wl.c`).
3. Configure the sensor with `ADXL355_UTIL_Config()` and read samples with
   `ADXL355_ReadRawSampleSet()` or the FIFO functions.

## Platform services

```c
typedef struct {
    ADXL355_Status_t (*read)(const uint8_t address, uint8_t *buffer,
                             const uint16_t data_size);
    ADXL355_Status_t (*write)(const uint8_t address,
                              const uint8_t *buffer,
                              const uint16_t data_size);
    void (*delay_ms)(uint32_t delay_ms);
} ADXL355_Platform_t;
```

`address` is the register address and `data_size` counts **data bytes only**.

The platform adds the command byte (SPI: `address << 1` plus the read/write
bit) or the I2C addressing, so an SPI transfer is `data_size + 1` bytes: for
example `ADXL355_WriteRegister()` calls `write(address, &data, 1)`, which is
2 bytes on the SPI bus. Transfers are at most `ADXL355_FIFO_MAX_BYTES` data
bytes (a full FIFO).

`read` must be **blocking**: it returns only when `buffer` holds the
data (or with an error). The driver uses the buffer as soon as the call
returns, often a local variable, so a read that only starts a transfer would
return wrong values, write garbage back in read-modify-write setters and let
the transfer overwrite a stack frame that no longer exists. A DMA
implementation is fine if it waits for the end of the transfer.

## FIFO read with DMA

Every driver function is blocking. The one exception is
`ADXL355_ReadFifoDMA()`, for streaming the FIFO while the CPU does something
else or sleeps. It takes the function that **starts** the transfer
(`ADXL355_StartRead_t`, same arguments as `read`), checks the
arguments, calls it for `3 x num_entries` bytes at FIFO_DATA and returns. The
driver keeps no state: the application detects the end of the transfer and
then uses the data.

```c
static uint8_t fifo_buffer[ADXL355_FIFO_MAX_BYTES]; // static: the DMA
                                                    // writes after return

// FIFO_FULL mapped to INT1 with ADXL355_EnableInterrupt(); on its edge:
status = ADXL355_ReadFifoDMA(ADXL355_STM32WL_ReadSpiDma, fifo_buffer, 96U);

// Later, when the transfer has ended (STM32: !ADXL355_STM32WL_IsSpiDmaBusy()):
status = ADXL355_UTIL_UnpackSampleSets((const ADXL355_RawSampleSet_t *)
                                       fifo_buffer, 32U, samples);
```

The buffer must stay valid until the transfer ends: never pass a local
variable of a function that returns before that. With `num_entries` a
multiple of 3 and an aligned FIFO (`ADXL355_UTIL_AlignFifo()`), the buffer
holds whole X, Y, Z sample sets.

## Example (stm32wl)

```c
#include "adxl355.h"
#include "adxl355_util.h"
#include "adxl355_stm32wl.h" // or another ADXL355_Platform_t

// After ADXL355_STM32WL_ConfigSpi() (see stm32wl/README.md)
const ADXL355_Platform_t *platform = ADXL355_STM32WL_GetSpiPlatform();

ADXL355_UTIL_DevCfg_t config = {
    .platform = platform,
    .odr = ADXL355_ODR_125Hz,
    .hpf = ADXL355_HPF_OFF,
    .range = ADXL355_RANGE_2G,
    .int_pol = ADXL355_INT_POL_HIGH,
    .int_src = ADXL355_DATA_READY_INT1,  // DATA_READY on the INT1 pin
    .act_mode = ADXL355_ACTIVITY_OFF,
    .act_cnt = 1U,
    .act_thr_lsb = 0U,
    .fifo_samples = ADXL355_FIFO_MAX_SAMPLES,
    .x_offset_lsb = 0,
    .y_offset_lsb = 0,
    .z_offset_lsb = 0,
    .meas_mode = ADXL355_MEAS_TEMP_ON_DRDY_ON
};

ADXL355_Status_t status = ADXL355_UTIL_Config(&config);

ADXL355_RawSampleSet_t raw;
ADXL355_UTIL_UnpackedSampleSet_t sample;

if (status == ADXL355_OK) {
    status = ADXL355_ReadRawSampleSet(platform, &raw);
}
if (status == ADXL355_OK) {
    status = ADXL355_UTIL_UnpackSampleSets(&raw, 1U, &sample);
}
```

