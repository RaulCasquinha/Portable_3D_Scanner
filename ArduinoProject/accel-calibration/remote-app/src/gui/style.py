"""
Colours, fonts and themes shared by the windows.

Layer: view. Themes are created once and reused: DearPyGui items are cheap to
bind but not to create every frame.
"""

import dearpygui.dearpygui as dpg

__all__ = ["AXIS_COLORS", "RAW_AXIS_COLORS", "RAW_COLOR", "CAL_COLOR", "REFERENCE_COLOR", "GREY", "GREEN", "RED",
           "YELLOW", "ORANGE", "CAPTURED", "STATUS_COLOR", "load_fonts", "use_mono_font",
           "apply_global_theme", "frame_height", "line_theme", "marker_theme", "fit_padding_theme",
           "selected_row_theme"]

AXIS_COLORS = [(235, 90, 90), (95, 205, 105), (95, 150, 255)]   # x, y, z: red, green, blue
# Raw x, y, z where they share a plot with the calibrated ones: the subtractive primaries,
# each the complement of its axis colour (cyan, magenta, yellow)
RAW_AXIS_COLORS = [(60, 220, 230), (230, 80, 230), (240, 225, 60)]
RAW_COLOR = (240, 150, 60)
CAL_COLOR = (80, 200, 230)
REFERENCE_COLOR = (235, 235, 235)
GREY = (130, 130, 140)
GREEN = (120, 255, 140)
RED = (255, 90, 90)
YELLOW = (255, 225, 80)
ORANGE = (240, 160, 60)
CAPTURED = (90, 210, 110)
STATUS_COLOR = (255, 220, 120)

_TEXT_FONTS = (r"C:\Windows\Fonts\segoeui.ttf", r"C:\Windows\Fonts\arial.ttf")
_MONO_FONTS = (r"C:\Windows\Fonts\consola.ttf", r"C:\Windows\Fonts\cour.ttf")
_TEXT_SIZE = 17
_MONO_SIZE = 14
_FRAME_PADDING = (6, 4)       # Inside buttons, inputs and the menu bar, pixels

_themes = {}         # Key: theme item, so each theme is made once
_mono_font = None


def load_fonts():
    """Load the text font (bound to everything) and the monospace font for tables.
    Missing fonts fall back to DearPyGui's own. Call once, before building windows."""
    global _mono_font
    with dpg.font_registry():
        text_font = _first_font(_TEXT_FONTS, _TEXT_SIZE)
        if text_font is not None:
            dpg.bind_font(text_font)
        _mono_font = _first_font(_MONO_FONTS, _MONO_SIZE)


def use_mono_font(item):
    """Draw an item in the monospace font, so its columns stay aligned."""
    if _mono_font is not None:
        dpg.bind_item_font(item, _mono_font)


def apply_global_theme():
    """Rounded corners and a little more spacing than the default."""
    with dpg.theme() as theme:
        with dpg.theme_component(dpg.mvAll):
            dpg.add_theme_style(dpg.mvStyleVar_WindowRounding, 6)
            dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 4)
            dpg.add_theme_style(dpg.mvStyleVar_FramePadding, *_FRAME_PADDING)
            dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 6)
    dpg.bind_theme(theme)


def frame_height():
    """Height of a one-line frame (a button, the menu bar), pixels. Valid once the
    first frame is drawn."""
    return dpg.get_text_size("Ag")[1] + 2 * _FRAME_PADDING[1]


def line_theme(color):
    """A plot line of this colour."""
    key = ("line", color)
    if not _has_theme(key):
        with dpg.theme() as theme:
            with dpg.theme_component(dpg.mvAll):
                dpg.add_theme_color(dpg.mvPlotCol_Line, color, category=dpg.mvThemeCat_Plots)
                dpg.add_theme_style(dpg.mvPlotStyleVar_LineWeight, 1.5,
                                    category=dpg.mvThemeCat_Plots)
        _themes[key] = theme
    return _themes[key]


def marker_theme(color, size=5):
    """Scatter points: filled circles of this colour and size."""
    key = ("marker", color, size)
    if not _has_theme(key):
        with dpg.theme() as theme:
            with dpg.theme_component(dpg.mvScatterSeries):
                for item in (dpg.mvPlotCol_MarkerFill, dpg.mvPlotCol_MarkerOutline):
                    dpg.add_theme_color(item, color, category=dpg.mvThemeCat_Plots)
                dpg.add_theme_style(dpg.mvPlotStyleVar_Marker, dpg.mvPlotMarker_Circle,
                                    category=dpg.mvThemeCat_Plots)
                dpg.add_theme_style(dpg.mvPlotStyleVar_MarkerSize, size,
                                    category=dpg.mvThemeCat_Plots)
        _themes[key] = theme
    return _themes[key]


def fit_padding_theme(x=0.0):
    """Auto-fit with a 10% margin in y (and x on the x axis), so constant lines and
    the outer points stay off the frame."""
    key = ("fit_padding", x)
    if not _has_theme(key):
        with dpg.theme() as theme:
            with dpg.theme_component(dpg.mvAll):
                dpg.add_theme_style(dpg.mvPlotStyleVar_FitPadding, x, 0.1,
                                    category=dpg.mvThemeCat_Plots)
        _themes[key] = theme
    return _themes[key]


def selected_row_theme():
    """A clearly visible selected row; in a table the default highlight is too faint."""
    key = ("selected_row",)
    if not _has_theme(key):
        with dpg.theme() as theme:
            with dpg.theme_component(dpg.mvSelectable):
                for item, color in ((dpg.mvThemeCol_Header, (30, 90, 150)),
                                    (dpg.mvThemeCol_HeaderHovered, (45, 105, 165)),
                                    (dpg.mvThemeCol_HeaderActive, (55, 120, 185))):
                    dpg.add_theme_color(item, color)
        _themes[key] = theme
    return _themes[key]


def _first_font(paths, size):
    """The first of these font files that loads, or None."""
    for path in paths:
        try:
            return dpg.add_font(path, size)
        except Exception:      # DearPyGui raises a plain Exception for a missing file
            continue
    return None


def _has_theme(key):
    """True if the theme for key was made in the current DearPyGui context (a new
    context, as in tests, deletes the old themes)."""
    return key in _themes and dpg.does_item_exist(_themes[key])
