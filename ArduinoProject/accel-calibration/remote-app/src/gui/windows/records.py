"""
Records window: save the calibration in use as a record of a sensor; the saved
calibrations, newest first, with a tick box each for Temperature analysis, and the
selected one in full.

Layer: view. The list, selection and ticks live in the RecordBrowser; this
window only draws them and passes the clicks on.
"""

import dearpygui.dearpygui as dpg

from ...services.report import RECORD_HEADER, record_text
from ..style import selected_row_theme, use_mono_font
from .base import Window

__all__ = ["RecordsWindow"]

_ALL_SENSORS = "all sensors"
_LIST_HEIGHT = 150
_TICK_COLUMN_WIDTH = 22


class RecordsWindow(Window):
    """Save, browse, tick and load calibrations."""

    TAG = "win_records"
    TITLE = "Records"
    TEXT_INPUTS = ("records.sensor_id",)

    def __init__(self, window_id, browser, on_loaded):
        """
        Args:
            window_id: The id reserved for this window.
            browser: The RecordBrowser.
            on_loaded: Called with the record after Load made it the calibration in use.
        """
        super().__init__(window_id)
        self._browser = browser
        self._on_loaded = on_loaded
        self._row_items = {}            # Row text: its selectable
        with self.window(pos=(380, 60), width=640, height=640):
            with dpg.group(horizontal=True):
                dpg.add_input_text(tag="records.sensor_id", hint="sensor ID", width=170)
                dpg.add_button(label="Save record", tag="records.save",
                               callback=lambda: self._browser.save_calibration(
                                   dpg.get_value("records.sensor_id")))
            dpg.add_separator()
            with dpg.group(horizontal=True):
                dpg.add_combo([_ALL_SENSORS], default_value=_ALL_SENSORS, tag="records.sensor",
                              width=170, callback=lambda: self._refresh())
                dpg.add_button(label="Refresh", tag="records.refresh",
                               callback=lambda: self._refresh())
                dpg.add_button(label="Load", tag="records.load", callback=self._on_load_click)
            # A tick box and a clickable row per record, under a frozen header
            with dpg.table(tag="records.list", header_row=True, scrollY=True, freeze_rows=1,
                           height=_LIST_HEIGHT, row_background=True,
                           policy=dpg.mvTable_SizingFixedFit):
                # Fixed width: the rows are added later, so the column can't fit them
                dpg.add_table_column(label="", width_fixed=True,
                                     init_width_or_weight=_TICK_COLUMN_WIDTH)
                dpg.add_table_column(label=RECORD_HEADER, width_stretch=True)
            dpg.bind_item_theme("records.list", selected_row_theme())
            # A read-only input, not a text, so it can be selected and copied
            dpg.add_input_text(tag="records.text", multiline=True, readonly=True, width=-1,
                               height=-1)
        use_mono_font("records.list")
        use_mono_font("records.text")
        self._refresh()

    def set_sensor(self, sensor):
        """Put a sensor name in the sensor ID field, e.g. of a loaded record."""
        dpg.set_value("records.sensor_id", sensor)

    def update(self, live):
        browser = self._browser
        # Rebuild the rows only when the list changes: a click only moves the selection
        rows_changed = self._changed("rows", (tuple(browser.rows), browser.sensor))
        if rows_changed:
            self._rebuild_rows()
        if self._changed("selected", browser.selected_row) or rows_changed:
            self._show_selection()

    def _refresh(self):
        """Read the files again, for the sensor chosen in the combo."""
        sensor = dpg.get_value("records.sensor")
        self._browser.refresh(None if sensor == _ALL_SENSORS else sensor)

    def _rebuild_rows(self):
        """One row per record, with its tick, and the sensors in the combo."""
        browser = self._browser
        dpg.configure_item("records.sensor", items=[_ALL_SENSORS, *browser.list_sensors()])
        dpg.set_value("records.sensor", browser.sensor or _ALL_SENSORS)
        dpg.delete_item("records.list", children_only=True, slot=1)     # Slot 1: the rows
        self._row_items = {}
        for row in browser.rows:
            with dpg.table_row(parent="records.list"):
                dpg.add_checkbox(default_value=row in browser.checked_rows, user_data=row,
                                 callback=self._on_tick)
                self._row_items[row] = dpg.add_selectable(
                    label=row, span_columns=True, user_data=row, callback=self._on_row_click)

    def _show_selection(self):
        """Highlight the selected row only, and show its record below the list."""
        selected = self._browser.selected_row
        for row, item in self._row_items.items():
            dpg.set_value(item, row == selected)
        dpg.set_value("records.text", self._selected_text())

    def _selected_text(self):
        browser = self._browser
        if not browser.rows:
            return "no records yet: use Save record"
        if browser.selected_record is None:
            return "this file could not be read"
        return record_text(browser.selected_record)

    def _on_row_click(self, sender, value, row):
        # A click on the selected row unselects it; select it again so one always is
        dpg.set_value(sender, True)
        self._browser.select(row)

    def _on_tick(self, sender, value, row):
        self._browser.set_checked(row, value)

    def _on_load_click(self):
        record = self._browser.load_selected()
        if record is not None:
            self._on_loaded(record)
