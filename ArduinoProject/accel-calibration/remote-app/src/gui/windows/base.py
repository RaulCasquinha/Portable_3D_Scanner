"""
What every window has in common.

Layer: view.
"""

import dearpygui.dearpygui as dpg

from ...services.report import placeholder_row

__all__ = ["Window"]


class Window:
    """
    A docked window that redraws only what changed.

    Subclasses set TAG (the alias of the window) and TITLE, build their items
    inside self.window() in __init__, and implement update().
    """

    TAG = ""
    TITLE = ""

    def __init__(self, window_id):
        """
        Args:
            window_id: The id reserved for this window at start (see App), so the
                saved layout finds it.
        """
        self._window_id = window_id
        self._last_values = {}

    def window(self, **options):
        """The DearPyGui window, as a context manager to build in."""
        return dpg.window(label=self.TITLE, tag=self._window_id, no_close=True,
                          no_collapse=True, **options)

    def update(self, live):
        """Redraw. Called every frame with the frame's LiveView."""

    def set_paused(self, is_paused):
        """Freeze (True) or resume (False) the live display. While paused, update() is not
        called. Windows with time plots release their axes so they can be zoomed."""

    def _set_empty_readout(self, tag, header, names, name_width, column_widths):
        """
        A readout table with --- for every value, while there is no reading. Its rows
        are named as the rows of numbers will be, and aligned like them. Set only when
        it changes; when the numbers are shown, call self._changed(tag, None) so the
        empty table is set again next time.
        """
        text = header + "".join("\n" + placeholder_row(name, name_width, column_widths)
                                for name in names)
        if self._changed(tag, text):
            dpg.set_value(tag, text)

    def _changed(self, key, value):
        """True the first time and whenever value differs from the last one for key."""
        if self._last_values.get(key, _UNSET) != value:
            self._last_values[key] = value
            return True
        return False


_UNSET = object()
