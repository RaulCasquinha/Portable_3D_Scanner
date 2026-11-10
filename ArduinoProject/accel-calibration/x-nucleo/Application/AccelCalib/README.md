# Application/AccelCalib

A library that runs the guided capture of an accelerometer calibration, and applies the result.
Standard C only: no hardware, no text and no protocol. It can be used with any accelerometer
and any link.

| File | Contents |
|---|---|
| `Inc/accel_calib.h` | the interface: `ACCEL_CALIB_Run()`, `ACCEL_CALIB_Apply()`, the settings, the events |
| `Src/accel_calib.c` | the procedure |

**The capture** (`ACCEL_CALIB_Run()`), for each position:
1. It waits for a confirmation, at most `input_timeout_s`.
2. It counts down `settle_s`, then averages `samples` readings.
3. It keeps the position if no axis std is above `still_g` and the reading is at least
   `min_separation_deg` from every kept position.

It then measures the references (+Z up, then +X up), each within 25 deg of its axis.

The application gives it **platform services** (`ACCEL_CALIB_Io_t`):
- `read_g`: read one sample;
- `get_input`: the user's CONFIRM or QUIT;
- `get_ms`: the time;
- `report`: receives the events;
- `idle`: optional work while waiting.

It reports what happens as **events** (`ACCEL_CALIB_Event_t`), in this order:
- `BEGIN`;
- for each position: `POSITION_PROMPT`, `COUNTDOWN`, `MEASURING`, then `POSITION`, or a
  rejection (`MOVED`, `TOO_CLOSE`) and the prompt again;
- the reference events (`REFERENCE_PROMPT` ... `REFERENCE` or `WRONG_ORIENTATION`);
- `END`, with the status.

The application turns the events into text and protocol lines.

**The correction** (`ACCEL_CALIB_Apply()`): `corrected = K (raw - bias)`, with `bias` and `K`
fitted by the PC.
