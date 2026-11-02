"""Scrolling log panel — colour-codes messages by severity prefix."""

from __future__ import annotations

import dearpygui.dearpygui as dpg

_MAX_LINES = 500

_COLOURS: dict[str, tuple[int, int, int]] = {
    "[INFO]":   (180, 180, 180),
    "[DEVICE]": (100, 190, 220),
    "[WARN]":   (230, 180, 50),
    "[ERROR]":  (230, 80,  80),
}
_DEFAULT = (180, 180, 180)


class LogView:
    """Colour-coded log panel that fills its parent window."""

    def __init__(self) -> None:
        self._lines: list[str] = []

    def build(self) -> None:
        """Create DearPyGUI items. Must be called inside a dpg.window() context."""
        dpg.add_separator()
        # height=-1 fills all remaining vertical space in the parent window
        with dpg.child_window(tag="_lv_scroll", height=-1, width=-1,
                               border=False, horizontal_scrollbar=False):
            pass

    def append(self, msg: str) -> None:
        """Append *msg* with automatic colour coding and auto-scroll."""
        self._lines.append(msg)
        if len(self._lines) > _MAX_LINES:
            self._lines.pop(0)
            children = dpg.get_item_children("_lv_scroll", slot=1)
            if children:
                dpg.delete_item(children[0])

        colour = _DEFAULT
        for prefix, col in _COLOURS.items():
            if msg.startswith(prefix):
                colour = col
                break

        dpg.add_text(msg, color=colour, parent="_lv_scroll", wrap=0)
        dpg.set_y_scroll("_lv_scroll", dpg.get_y_scroll_max("_lv_scroll"))
