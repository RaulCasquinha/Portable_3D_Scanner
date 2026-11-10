"""
GUI: the DearPyGui windows. The only package that imports dearpygui.

    app        App: builds the windows, routes the keys, updates every frame
    menu_bar   MenuBar: the bar at the top: show/hide windows, workspaces
    windows/   one module per window
    widgets/   reusable pieces: plots, the 3D view, hover help
    style      colours, fonts and themes
    tips       the hover help text of every tagged item

Tags are "<window>.<item>" (connection.connect, stream.plot.x). Windows only read
the controller's state and call its methods.
"""
