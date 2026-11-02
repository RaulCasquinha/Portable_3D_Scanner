import os

import dearpygui.dearpygui as dpg

from src.app.scan_session import ScanSession
from src.sim.sim_session import SimSession
from src.gui.main_window import MainWindow

_LAYOUT_FILE = "layout.ini"


def main() -> None:
    dpg.create_context()

    # Enable docking and restore the saved layout if one exists
    dpg.configure_app(docking=True, docking_space=True)
    if os.path.exists(_LAYOUT_FILE):
        dpg.configure_app(init_file=_LAYOUT_FILE)

    session     = ScanSession()
    sim_session = SimSession()
    window      = MainWindow(session, sim_session)
    window.build()

    dpg.create_viewport(title="Portable 3D Scanner", width=1200, height=760)
    dpg.setup_dearpygui()
    dpg.show_viewport()

    while dpg.is_dearpygui_running():
        session.process_pending()
        sim_session.process_pending()
        window.tick()
        dpg.render_dearpygui_frame()

    # Persist the current panel layout for next launch
    dpg.save_init_file(_LAYOUT_FILE)

    session.stop()
    sim_session.stop()
    dpg.destroy_context()


if __name__ == "__main__":
    main()
