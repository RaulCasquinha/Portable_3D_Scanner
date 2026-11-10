# gui/widgets

Reusable DearPyGui pieces that know nothing of this app's data.

| Module | Does |
|---|---|
| `plots.py` | `add_line()`, `set_xy()`, `build_time_plots()`: a grid of plots with linked time axes; `release_time_plots()` / `follow_time_plots()` free the axes for zooming and give them back |
| `orbit3d.py` | `Orbit3D`: a drawlist with a 3D view that rotates when dragged; `resize()` changes its size. Depth is shaded: a translucent sphere dims what is behind (not the arrow, whose colour matters), points are filled in front and rings behind, the arrow head filled or outlined and its hidden part dashed; `set_shading()` sets the tint, the highlight and those marks; `axes()` draws each axis faintly in its own colour |
| `hover_help.py` | `HoverHelp`: tooltips, including for plots and drawlists, which can't hold one |
