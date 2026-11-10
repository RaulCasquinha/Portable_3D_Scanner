# presets

Named settings: `presets/<name>.json`. They are saved and loaded in the **Calibration settings**
window. The preset named **Default** is loaded when the app starts: save over it to change
what the app starts with. Without it, the app starts with the normal calibration: 6 positions,
bias and scale, no references.

A preset holds the capture settings (positions, samples, references), the estimated terms, and
the display settings (cal source, applied terms, smoothing on/off and its time in s, reference, time scale), the Tilt mode and the Status sphere shading. Its keys are listed in
`_PRESET_FIELDS` in `src/gui/windows/calibration_settings.py`; unknown keys are ignored.
