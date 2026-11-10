"""
Hover help for any item.

Layer: view. Most items get a native tooltip. Plots and drawlists can't hold one,
so they get a small window shown at the mouse once it rests on them.
"""

import time

import dearpygui.dearpygui as dpg

__all__ = ["HoverHelp"]

_DELAY_S = 0.6           # Rest this long before the help appears
_WIDTH = 420             # Wrap width of the help text, pixels
_OFFSET = 16             # The window opens this far below and right of the mouse
_NO_TOOLTIP = ("mvAppItemType::mvPlot", "mvAppItemType::mvDrawlist")


class HoverHelp:
    """Tooltips for items. Call update() every frame."""

    def __init__(self):
        self._texts = {}         # Item without tooltip: its text
        self._item, self._mouse, self._since = None, None, 0.0
        with dpg.window(no_title_bar=True, no_resize=True, no_move=True, autosize=True,
                        no_focus_on_appearing=True, no_saved_settings=True, no_collapse=True,
                        no_docking=True, show=False) as self._window:
            self._text = dpg.add_text("", wrap=_WIDTH)

    def add(self, item, text):
        """Show text when the mouse rests on item."""
        if dpg.get_item_type(item) in _NO_TOOLTIP:
            self._texts[item] = text
        else:
            with dpg.tooltip(item, delay=_DELAY_S, hide_on_activity=True):
                dpg.add_text(text, wrap=_WIDTH)

    def update(self):
        """Show or hide the help window of plots and drawlists."""
        mouse = tuple(dpg.get_mouse_pos(local=False))
        item = next((i for i in self._texts if dpg.is_item_hovered(i)), None)
        now = time.perf_counter()
        if item != self._item or mouse != self._mouse:
            # The mouse moved: start waiting again
            self._item, self._mouse, self._since = item, mouse, now
            dpg.hide_item(self._window)
        elif item is not None and now - self._since >= _DELAY_S \
                and not dpg.is_item_shown(self._window):
            dpg.set_value(self._text, self._texts[item])
            dpg.set_item_pos(self._window, (mouse[0] + _OFFSET, mouse[1] + _OFFSET))
            dpg.show_item(self._window)
            dpg.focus_item(self._window)
