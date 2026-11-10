# calibrations

Saved calibration records: `calibrations/<sensor>/<YYYYmmdd_HHMMSS>.json`. They are written by
**Save record** and **New compensated calibration**, and listed in the **Records** window. The
app only adds files, and never changes or deletes one.

There are two kinds of record:

- **Simple**: one capture at one temperature. It holds:
  - the fitted parameters (`bias_g`, `scale`, `nonortho_deg`, `alignment_deg`, `K`);
  - the rms |g| error before and after the fit;
  - the C `defines`;
  - the `capture`: every position with its std, the references and the temperatures.
- **Temperature compensated** (`method` is `temperature-compensated`): a `temperature_table`
  with one row per simple calibration it was made from, plus `slope_groups` and
  `hold_outside_range`. Its parameters follow the sensor's temperature once it is loaded.

The fields are documented in `src/storage/record_format.py`. New fields may be added; old
records must keep loading.
