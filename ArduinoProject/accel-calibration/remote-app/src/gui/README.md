# gui

The DearPyGui interface: the only code that imports dearpygui. Windows read the controller's
state and call its methods; they hold no logic of their own.

| Item | Does |
|---|---|
| `app.py` | `App`: builds every window, routes the keys d q r s, sets the default layout, and updates the windows every frame |
| `menu_bar.py` | `MenuBar`: the bar at the top of the viewport: show and hide windows, save, delete and apply workspaces |
| [`windows/`](windows/README.md) | one module per window |
| [`widgets/`](widgets/README.md) | reusable pieces: plots, the 3D view, hover help |
| `style.py` | colours, fonts and plot themes |
| `tips.py` | the hover help of every item, by tag |

Every item that changes or has help gets a tag `"<window>.<item>"` (for example
`controls.connect` or `stream.plot.x`) and an entry in `tips.py`.

The window ids are reserved first, in the order of `WINDOW_TAGS`, because the saved layout
(`app_layout.ini` in the app's folder) finds windows by id. New windows go at the end of that
list.
