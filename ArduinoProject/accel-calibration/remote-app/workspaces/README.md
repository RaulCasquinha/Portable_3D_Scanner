# workspaces

Saved window arrangements: `workspaces/<name>.json`. They are saved, deleted and applied from
the bar at the top of the app (**Workspaces** menu, then one item per workspace). At start the
app opens the first one by name; without any, the default layout.

A workspace holds:

| Key | Meaning |
|---|---|
| `area` | width and height of the area under the bar when it was saved, pixels |
| `windows` | per window tag (`win_controls`, ...): x, y, width, height, with y measured from the bar |
| `hidden` | the tags of the windows that were hidden |

When applied, positions and sizes are scaled from `area` to the current screen. Windows not in
the file are hidden (show them from the Windows menu, then save the workspace again). These files ship with the app; `python build_exe.py` copies new
ones to `compiled/workspaces/`.
