"""
The application window: builds every window and the bar at the top, routes the keys
and updates the windows each frame.

Layer: view. It owns the controller objects (Board, RecordBrowser) and the stores,
and passes each window only what it needs.
"""

import time

import dearpygui.dearpygui as dpg

from ..controller.board import Board
from ..controller.live_view import live_view
from ..controller.record_browser import RecordBrowser
from ..storage.paths import PRESET_DIR, RECORD_DIR, WORKSPACE_DIR
from ..storage.preset_store import PresetStore
from ..storage.record_store import RecordStore
from .menu_bar import MenuBar
from .style import apply_global_theme, load_fonts
from .tips import TIPS
from .widgets.hover_help import HoverHelp
from .windows.board_log import BoardLogWindow
from .windows.calibration_settings import DEFAULT_PRESET, CalibrationSettingsWindow
from .windows.connection import ConnectionWindow
from .windows.controls import ControlsWindow
from .windows.display_settings import DisplaySettingsWindow
from .windows.errors import ErrorsWindow
from .windows.parameters import ParametersWindow
from .windows.procedure import ProcedureWindow
from .windows.records import RecordsWindow
from .windows.status_sphere import StatusSphereWindow
from .windows.stream import StreamWindow
from .windows.temperature import TemperatureWindow
from .windows.tilt import TiltWindow

__all__ = ["WINDOW_TAGS", "App"]

# The layout file names windows by id, and ids are given in order from the start.
# These are reserved first, in this order, so a saved layout still finds its
# windows when widgets are added. New windows go at the END only.
WINDOW_TAGS = ("win_controls", "win_stream", "win_calib", "win_params", "win_errors",
               "win_records", "win_temp", "win_settings", "win_tilt",
               "win_connection", "win_procedure", "win_board_log", "win_display")

_SLOW_UPDATE_S = 0.1          # The sphere and tables: 10 times a second is enough
_KEYS = ((dpg.mvKey_D, "d"), (dpg.mvKey_Q, "q"), (dpg.mvKey_R, "r"), (dpg.mvKey_S, "s"))

# Default layout, in pixels: column widths, heights of the small windows, and the
# share of the height of the stacked live windows
_LEFT_WIDTH = 560              # Fits the measurement table
_SPHERE_WIDTH = 310           # Also the Display settings, under the sphere
_CONNECTION_HEIGHT = 100      # Left column, top to bottom: Connection, Controls,
_CONTROLS_HEIGHT = 135        # Calibration procedure (the rest), Calibration settings,
_SETTINGS_HEIGHT = 285        # Board messages
_BOARD_LOG_HEIGHT = 110
_DISPLAY_HEIGHT = 200
_STREAM_SHARE = 0.40          # Middle column: Live stream, then Tilt, then Errors
_TILT_SHARE = 0.22


class App:
    """All the windows of the app."""

    def __init__(self, board=None):
        """
        Args:
            board: The Board; None for a new one on a serial port. Tests pass one.
        """
        window_ids = {tag: dpg.generate_uuid() for tag in WINDOW_TAGS}   # Must be first
        self.board = board if board is not None else Board()
        self.records = RecordBrowser(RecordStore(RECORD_DIR), self.board)
        self._last_slow_update = 0.0

        load_fonts()
        apply_global_theme()
        # Display settings first: the live windows read it as they are built
        self.display = DisplaySettingsWindow(window_ids["win_display"])
        self.settings = CalibrationSettingsWindow(window_ids["win_settings"], self.board,
                                                  PresetStore(PRESET_DIR))
        self.connection = ConnectionWindow(window_ids["win_connection"], self.board)
        self.controls = ControlsWindow(window_ids["win_controls"], self.board, self.settings)
        self.procedure = ProcedureWindow(window_ids["win_procedure"], self.board,
                                         self.settings)
        self.board_log = BoardLogWindow(window_ids["win_board_log"], self.board)
        self.stream = StreamWindow(window_ids["win_stream"], self.display)
        self.tilt = TiltWindow(window_ids["win_tilt"], self.display)
        self.errors = ErrorsWindow(window_ids["win_errors"], self.display)
        self.sphere = StatusSphereWindow(window_ids["win_calib"], self.board, self.display)
        self.parameters = ParametersWindow(window_ids["win_params"], self.board)
        self.record_window = RecordsWindow(window_ids["win_records"], self.records,
                                           on_loaded=self._on_record_loaded)
        self.temperature = TemperatureWindow(window_ids["win_temp"], self.board, self.records)
        # After every window: the preset also sets the Display settings and the Live
        # stream time scale
        self.settings.load_preset(DEFAULT_PRESET, reports_missing=False)
        # In the order of the Windows menu
        self._windows = (self.connection, self.controls, self.settings, self.procedure,
                         self.display, self.stream, self.tilt, self.errors, self.sphere,
                         self.parameters, self.record_window, self.temperature,
                         self.board_log)
        # The windows Pause display (in Display settings) freezes, and whether they were paused last frame
        self._live_windows = (self.stream, self.tilt, self.errors)
        self._was_paused = False
        for tag, window_id in window_ids.items():
            dpg.add_alias(tag, window_id)
        # The bar at the top: show and hide windows, workspaces
        self.menu_bar = MenuBar(PresetStore(WORKSPACE_DIR),
                                [(window.TAG, window.TITLE) for window in self._windows],
                                self.board, on_default_layout=self.default_layout)

        with dpg.handler_registry():
            for key, char in _KEYS:
                dpg.add_key_press_handler(key, callback=self._on_key, user_data=char)
        self.hover_help = HoverHelp()
        for tag, text in TIPS.items():
            self.hover_help.add(tag, text)

    def frame(self):
        """Handle what the board sent and redraw. Call once per frame."""
        self.board.advance()     # Data keeps coming in while the display is paused
        is_paused = self.display.is_paused
        if is_paused != self._was_paused:
            self._was_paused = is_paused
            for window in self._live_windows:
                window.set_paused(is_paused)
        display = self.display
        live = live_view(self.board, self.stream.time_scale_s(), display.smoothing_s(),
                         display.cal_source(), display.applied_terms())
        for window in self._windows:
            if not (is_paused and window in self._live_windows):   # Paused: keep what is drawn
                window.update(live)
        self.hover_help.update()
        now = time.perf_counter()
        if now - self._last_slow_update > _SLOW_UPDATE_S:
            self._last_slow_update = now
            self.procedure.update_slow()
            self.sphere.update_slow()

    def default_layout(self):
        """Put the windows in their default places, filling the viewport under the bar."""
        top, width, height = self.menu_bar.work_area()
        # Left column: the board and the capture
        procedure_y = _CONNECTION_HEIGHT + _CONTROLS_HEIGHT
        settings_y = height - _BOARD_LOG_HEIGHT - _SETTINGS_HEIGHT
        # Middle column: the live readings
        middle_width = (width - _LEFT_WIDTH) // 2
        stream_height = int(height * _STREAM_SHARE)
        tilt_height = int(height * _TILT_SHARE)
        errors_y = stream_height + tilt_height
        # Right column: the sphere over the display settings, the records, the result
        right_x = _LEFT_WIDTH + middle_width
        right_width = width - right_x
        half = height // 2
        params_width = right_width // 2
        places = {
            "win_connection": (0, 0, _LEFT_WIDTH, _CONNECTION_HEIGHT),
            "win_controls": (0, _CONNECTION_HEIGHT, _LEFT_WIDTH, _CONTROLS_HEIGHT),
            "win_procedure": (0, procedure_y, _LEFT_WIDTH, settings_y - procedure_y),
            "win_settings": (0, settings_y, _LEFT_WIDTH, _SETTINGS_HEIGHT),
            "win_board_log": (0, height - _BOARD_LOG_HEIGHT, _LEFT_WIDTH, _BOARD_LOG_HEIGHT),
            "win_stream": (_LEFT_WIDTH, 0, middle_width, stream_height),
            "win_tilt": (_LEFT_WIDTH, stream_height, middle_width, tilt_height),
            "win_errors": (_LEFT_WIDTH, errors_y, middle_width, height - errors_y),
            "win_calib": (right_x, 0, _SPHERE_WIDTH, half - _DISPLAY_HEIGHT),
            "win_display": (right_x, half - _DISPLAY_HEIGHT, _SPHERE_WIDTH, _DISPLAY_HEIGHT),
            "win_records": (right_x + _SPHERE_WIDTH, 0, right_width - _SPHERE_WIDTH, half),
            "win_params": (right_x, half, params_width, height - half),
            "win_temp": (right_x + params_width, half, right_width - params_width,
                         height - half),
        }
        for tag, (x, y, w, h) in places.items():
            dpg.configure_item(tag, pos=(x, top + y), width=w, height=h)

    def _on_key(self, sender, app_data, key):
        """d, q, r, s go to the board while connected, unless a text field is being typed in."""
        text_inputs = (self.settings.TEXT_INPUTS + self.display.TEXT_INPUTS
                       + self.record_window.TEXT_INPUTS + self.menu_bar.TEXT_INPUTS)
        if not self.board.is_connected or any(dpg.is_item_active(tag) for tag in text_inputs):
            return
        board = self.board
        {"d": board.capture_position, "q": board.abort,
         "r": board.start_stream, "s": board.stop_stream}[key]()

    def _on_record_loaded(self, record):
        """A loaded record also sets the sensor name and the ticked terms."""
        self.record_window.set_sensor(record.get("sensor", ""))
        self.settings.set_terms(self.board.terms)
