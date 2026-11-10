"""
The bar fixed at the top of the viewport:
- Windows: a tick per window, to show or hide it;
- Workspaces: save the current arrangement under a name, delete one, default layout;
- then one item per saved workspace: clicking it moves the windows there at once.

A workspace is the place and size of every window and which ones are hidden:
workspaces/<name>.json in the app's folder (WORKSPACE_DIR).

Layer: view. DearPyGui reads the docking only at start, so windows that were docked
become floating windows in the same places when a workspace is applied. Positions are
kept relative to the area under the bar and scaled to the viewport's size, so a
workspace also fits a different screen. Hidden windows are not in the layout ini:
they are shown again at the next start.
"""

import dearpygui.dearpygui as dpg

from .style import frame_height

__all__ = ["MenuBar"]

_BAR = "menu.bar"
_DIALOG = "workspaces.dialog"
_NAME_WIDTH = 240


class MenuBar:
    """The top bar: windows and workspaces."""

    TEXT_INPUTS = ("workspaces.name",)

    def __init__(self, store, windows, board, on_default_layout):
        """
        Args:
            store: A PresetStore of the workspace files.
            windows: (tag, title) of each window the bar shows, hides and places.
            board: The Board, for the status line.
            on_default_layout: Puts the windows in their default places.
        """
        self._store = store
        self._window_tags = tuple(tag for tag, _ in windows)
        self._board = board
        self._on_default_layout = on_default_layout
        self._items = []          # The menu items of the saved workspaces
        with dpg.viewport_menu_bar(tag=_BAR):
            with dpg.menu(label="Windows", tag="windows.menu"):
                for tag, title in windows:
                    dpg.add_menu_item(label=title, tag=_tick(tag), check=True, default_value=True,
                                      callback=lambda s, shown, t: self._show(t, shown),
                                      user_data=tag)
                dpg.add_separator()
                dpg.add_menu_item(label="Show all", tag="windows.show_all",
                                  callback=lambda: self.show_all(True))
                dpg.add_menu_item(label="Hide all", tag="windows.hide_all",
                                  callback=lambda: self.show_all(False))
            with dpg.menu(label="Workspaces", tag="workspaces.menu"):
                dpg.add_menu_item(label="Save current as...", tag="workspaces.save_as",
                                  callback=self._open_dialog)
                dpg.add_menu(label="Delete", tag="workspaces.delete")
                dpg.add_separator()
                dpg.add_menu_item(label="Default layout", tag="workspaces.default",
                                  callback=self.default_layout)
        with dpg.window(label="Save workspace", tag=_DIALOG, modal=True, show=False,
                        autosize=True, no_resize=True, no_saved_settings=True):
            dpg.add_input_text(tag="workspaces.name", hint="name", width=_NAME_WIDTH,
                               on_enter=True, callback=self._on_save)
            with dpg.group(horizontal=True):
                dpg.add_button(label="Save", tag="workspaces.save", callback=self._on_save)
                dpg.add_button(label="Cancel", tag="workspaces.cancel",
                               callback=lambda: dpg.hide_item(_DIALOG))
        self._refresh()

    def work_area(self):
        """
        The part of the viewport under the bar, where the windows go.

        Returns:
            (top, width, height), pixels.
        """
        top = frame_height()        # DearPyGui doesn't report the bar's size
        return (top, dpg.get_viewport_client_width(),
                dpg.get_viewport_client_height() - top)

    def default_layout(self):
        """Show every window and put them in their default places."""
        self.show_all()
        self._on_default_layout()

    def open_first_workspace(self):
        """
        At start: apply the first saved workspace (by name), or the default layout if
        there is none or it can't be read. The windows start hidden until then.
        """
        names = self._store.list_names()
        if not (names and self.apply(names[0])):
            self.default_layout()

    def apply(self, name):
        """
        Show, hide, move and resize the windows as in the workspace saved under name.

        Returns:
            False if the workspace could not be read (the windows are left as they are).
        """
        try:
            values = self._store.read(name)
            saved_width, saved_height = values["area"]
            places = {tag: values["windows"][tag] for tag in self._window_tags
                      if tag in values["windows"]}
            hidden = set(values.get("hidden", ()))
            scale_x, scale_y = 1 / saved_width, 1 / saved_height
        except (OSError, KeyError, TypeError, ValueError, ZeroDivisionError) as error:
            self._board.status = f"workspace '{name}' could not be read: {error}"
            return False
        top, width, height = self.work_area()
        scale_x, scale_y = scale_x * width, scale_y * height
        for tag, (x, y, w, h) in places.items():
            dpg.configure_item(tag, pos=(round(x * scale_x), top + round(y * scale_y)),
                               width=round(w * scale_x), height=round(h * scale_y))
        for tag in self._window_tags:      # A window the workspace doesn't know is hidden
            self._show(tag, tag in places and tag not in hidden)
        self._board.status = f"workspace '{name}'"
        return True

    # -- windows ----------------------------------------------------------------------

    def _show(self, tag, shown):
        """Show (True) or hide (False) a window, and tick its menu item to match."""
        dpg.configure_item(tag, show=shown)
        dpg.set_value(_tick(tag), shown)

    def show_all(self, shown=True):
        """Show (True) or hide (False) every window. The bar stays, to show them again."""
        for tag in self._window_tags:
            self._show(tag, shown)

    # -- workspaces -------------------------------------------------------------------

    def _current(self):
        """The workspace of the windows as they are now, as saved in its file."""
        top, width, height = self.work_area()
        windows = {}
        for tag in self._window_tags:
            x, y = dpg.get_item_pos(tag)
            w, h = dpg.get_item_rect_size(tag)
            windows[tag] = [x, y - top, w, h]
        hidden = [tag for tag in self._window_tags if not dpg.is_item_shown(tag)]
        return {"area": [width, height], "windows": windows, "hidden": hidden}

    def _refresh(self):
        """Rebuild the workspace items and the Delete menu from the files."""
        for item in self._items:
            dpg.delete_item(item)
        dpg.delete_item("workspaces.delete", children_only=True)
        names = self._store.list_names()
        self._items = [dpg.add_menu_item(label=name, parent=_BAR,
                                         callback=lambda s, a, n: self.apply(n),
                                         user_data=name)
                       for name in names]
        for name in names:
            dpg.add_menu_item(label=name, parent="workspaces.delete",
                              callback=lambda s, a, n: self._on_delete(n), user_data=name)
        if not names:
            dpg.add_menu_item(label="(none saved)", parent="workspaces.delete", enabled=False)

    def _open_dialog(self):
        """Ask for the name of the new workspace, near the middle of the viewport."""
        dpg.set_value("workspaces.name", "")
        dpg.set_item_pos(_DIALOG, (dpg.get_viewport_client_width() // 2 - _NAME_WIDTH // 2,
                                   dpg.get_viewport_client_height() // 3))
        dpg.show_item(_DIALOG)
        dpg.focus_item("workspaces.name")

    def _on_save(self):
        try:
            name = self._store.write(dpg.get_value("workspaces.name"), self._current())
        except (OSError, ValueError) as error:
            self._board.status = f"workspace: {error}"
            return
        dpg.hide_item(_DIALOG)
        self._refresh()
        self._board.status = f"workspace '{name}' saved"

    def _on_delete(self, name):
        try:
            self._store.delete(name)
        except OSError as error:
            self._board.status = f"workspace '{name}' could not be deleted: {error}"
            return
        self._refresh()
        self._board.status = f"workspace '{name}' deleted"


def _tick(window_tag):
    """The tag of a window's item in the Windows menu."""
    return f"windows.{window_tag}"
