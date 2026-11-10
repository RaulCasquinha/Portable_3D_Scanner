# storage

The files the app reads and writes.

| Module | Does |
|---|---|
| `paths.py` | every folder and file the app uses, all in the app's folder (next to the exe when compiled) |
| `record_format.py` | a record dict <-> `Calibration` and `Capture`; documents every record field |
| `record_store.py` | `RecordStore`: list, read and add `calibrations/<sensor>/<time>.json` |
| `preset_store.py` | `PresetStore`: list, read, write and delete `<name>.json` in one folder: the presets, and the workspaces |
| `file_names.py` | `clean_name()`: a name typed by the user, made safe for a file |
