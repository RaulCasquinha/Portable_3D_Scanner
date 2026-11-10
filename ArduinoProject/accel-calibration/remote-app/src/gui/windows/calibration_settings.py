"""
Calibration settings window: how to capture and fit, and named presets. A preset
also holds the Display settings, the Live stream time scale, the Tilt mode and the
Status sphere shading.

The normal calibration is 6 positions estimating bias and scale; the
non-orthogonality (alpha, 9 positions or more) and the alignment (references)
are extras. The preset named DEFAULT_PRESET is loaded at start; without it the
built-in defaults below are the normal calibration.

Layer: view. The other windows read the settings through this window's methods.
"""

import dearpygui.dearpygui as dpg

from ...processing.sfem import TERMS
from ...protocol.codec import MAX_POSITIONS, MAX_SAMPLES
from ..style import GREY
from .base import Window
from .display_settings import MAX_SMOOTHING_S, MIN_SMOOTHING_S, TERM_LABELS
from .stream import TIME_SCALE_TAG
from .tilt import MODE_TAG as TILT_MODE_TAG

__all__ = ["DEFAULT_PRESET", "CalibrationSettingsWindow"]

DEFAULT_PRESET = "Default"      # Loaded at start, if it exists

_REFERENCE_CHOICES = ["none (magnitude only)", "z up", "z up + x up"]   # Index = references
# The normal calibration: 6 positions for the 6 unknowns of bias and scale, no references
_DEFAULT_POSITIONS = 6
_DEFAULT_TERMS = ("bias", "scale")
_DEFAULT_SAMPLES = 200

# Preset key: tag. The keys are those of presets saved by older versions, kept so
# they still load
_PRESET_FIELDS = {"positions": "settings.positions",
                  "samples": "settings.samples",
                  "references": "settings.references",
                  "term.bias": "settings.term.bias",
                  "term.scale": "settings.term.scale",
                  "term.nonortho": "settings.term.nonortho",
                  "smoothing": "display.smoothing",
                  "smoothing_s": "display.smoothing_s",
                  "stream_s": TIME_SCALE_TAG,
                  "tilt_mode": TILT_MODE_TAG,
                  "sphere_tint": "sphere.tint",
                  "sphere_highlight": "sphere.highlight",
                  "sphere_marks_behind": "sphere.marks_behind",
                  "show_reference": "display.show_reference",
                  "cal_source": "display.cal_source",
                  "apply.bias": "display.apply.bias",
                  "apply.scale": "display.apply.scale",
                  "apply.nonortho": "display.apply.nonortho",
                  "apply.misalignment": "display.apply.misalignment"}


class CalibrationSettingsWindow(Window):
    """The capture settings, and the presets."""

    TAG = "win_settings"
    TITLE = "Calibration settings"
    TEXT_INPUTS = ("settings.positions", "settings.samples", "settings.preset_name")

    def __init__(self, window_id, board, preset_store):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board: Refit, and the status line for preset messages.
            preset_store: The PresetStore of the presets.
        """
        super().__init__(window_id)
        self._board = board
        self._presets = preset_store
        with self.window(pos=(40, 520), width=350, height=440):
            dpg.add_text("capture", color=GREY)
            dpg.add_input_int(label="positions", tag="settings.positions",
                              default_value=_DEFAULT_POSITIONS, min_value=1,
                              max_value=MAX_POSITIONS, min_clamped=True, max_clamped=True,
                              width=120)
            dpg.add_input_int(label="samples", tag="settings.samples",
                              default_value=_DEFAULT_SAMPLES, min_value=1, max_value=MAX_SAMPLES,
                              min_clamped=True, max_clamped=True, width=120)
            dpg.add_combo(_REFERENCE_CHOICES, default_value=_REFERENCE_CHOICES[0],
                          tag="settings.references", label="references", width=170)
            with dpg.group(horizontal=True):
                dpg.add_text("estimate:")
                for term in TERMS:
                    dpg.add_checkbox(label=TERM_LABELS[term], tag=f"settings.term.{term}",
                                     default_value=term in _DEFAULT_TERMS)
                dpg.add_button(label="Refit", tag="settings.refit",
                               callback=lambda: self._board.refit(self.terms()))
            dpg.add_separator()

            with dpg.collapsing_header(label="Presets", default_open=True):
                with dpg.group(horizontal=True):
                    dpg.add_combo(self._presets.list_names(), tag="settings.preset_list",
                                  width=170)
                    dpg.add_button(label="Load", tag="settings.preset_load",
                                   callback=self._on_preset_load)
                    dpg.add_button(label="Delete", tag="settings.preset_delete",
                                   callback=self._on_preset_delete)
                with dpg.group(horizontal=True):
                    dpg.add_input_text(tag="settings.preset_name", hint="name", width=170)
                    dpg.add_button(label="Save", tag="settings.preset_save",
                                   callback=self._on_preset_save)

    # -- the settings, for the other windows ------------------------------------------

    def positions(self):
        """Positions to capture."""
        return dpg.get_value("settings.positions")

    def samples(self):
        """Samples averaged per position."""
        return dpg.get_value("settings.samples")

    def references(self):
        """How many reference positions: 0, 1 (z up) or 2 (z up and x up)."""
        return _REFERENCE_CHOICES.index(dpg.get_value("settings.references"))

    def terms(self):
        """The ticked terms, in the order of sfem.TERMS."""
        return tuple(term for term in TERMS if dpg.get_value(f"settings.term.{term}"))

    def set_terms(self, terms):
        """Tick these terms only, e.g. those of a loaded record."""
        for term in TERMS:
            dpg.set_value(f"settings.term.{term}", term in terms)

    # -- presets --------------------------------------------------------------------

    def load_preset(self, name, reports_missing=True):
        """
        Set the settings of a preset, and select it in the list.

        Args:
            name: The preset's name.
            reports_missing: False to stay silent if there is no such preset, as at
                start when the Default preset was deleted.

        Returns:
            True if it was loaded.
        """
        if name not in self._presets.list_names() and not reports_missing:
            return False
        try:
            values = self._presets.read(name)
        except (OSError, ValueError) as error:
            self._board.status = f"preset '{name}' could not be read: {error}"
            return False
        for key, value in values.items():
            # Unknown keys are from other versions; "10 s" time scales are from an
            # old version that used buttons
            if key in _PRESET_FIELDS and not (key == "stream_s" and isinstance(value, str)):
                dpg.set_value(_PRESET_FIELDS[key], value)
        # Older presets kept the smoothing as a frequency: f Hz is the mean of 1/f s
        hz = values.get("vector_hz")
        if "smoothing_s" not in values and isinstance(hz, (int, float)) and hz > 0:
            dpg.set_value("display.smoothing", True)
            dpg.set_value("display.smoothing_s",
                          min(max(1.0 / hz, MIN_SMOOTHING_S), MAX_SMOOTHING_S))
        dpg.set_value("settings.preset_list", name)
        return True

    def _on_preset_save(self):
        values = {key: dpg.get_value(tag) for key, tag in _PRESET_FIELDS.items()}
        try:
            name = self._presets.write(dpg.get_value("settings.preset_name"), values)
        except (ValueError, OSError) as error:
            self._board.status = f"preset: {error}"
            return
        self._refresh_list(selected=name)
        self._board.status = f"preset '{name}' saved"

    def _on_preset_delete(self):
        name = dpg.get_value("settings.preset_list")
        if not name:
            self._board.status = "preset: select one first"
            return
        try:
            self._presets.delete(name)
        except OSError as error:
            self._board.status = f"preset '{name}' could not be deleted: {error}"
            return
        self._refresh_list(selected="")
        self._board.status = f"preset '{name}' deleted"

    def _on_preset_load(self):
        name = dpg.get_value("settings.preset_list")
        if not name:
            self._board.status = "preset: select one first"
            return
        if self.load_preset(name):
            self._board.status = f"preset '{name}' loaded"

    def _refresh_list(self, selected):
        dpg.configure_item("settings.preset_list", items=self._presets.list_names())
        dpg.set_value("settings.preset_list", selected)
