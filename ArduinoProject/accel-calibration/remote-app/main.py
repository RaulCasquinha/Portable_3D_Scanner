"""
Accelerometer calibrator for the Nucleo of Accel_Calib_stm32.

Connect, Start, place the board and press d for each position. The fit is the
closed-form ellipsoid fit and runs when the capture ends, then the Parameters
window gives the two defines for app_calib_conf.h.

Each calibration can be saved as a record of its sensor in calibrations/<sensor>/,
and the Records window browses them. The bar at the top shows and hides windows
and saves named workspaces; at start the windows open in the first workspace (by
name), or in the default layout if there is none. See README.md, CLAUDE.md and
docs/diagrams/out/architecture.svg.

    python main.py
    python main.py --smoke 4 --screenshot shot.png   # replays the newest record, no port
"""

import argparse
import time

import dearpygui.dearpygui as dpg
import numpy as np

from src.gui.app import App
from src.processing.sfem import TERMS
from src.protocol.codec import STREAM_RATE_HZ, format_capture
from src.storage.paths import LAYOUT_FILE, RECORD_DIR
from src.storage.record_store import RecordStore

_VIEWPORT_SIZE = (1700, 1000)
_LAYOUT_FRAME = 3             # The viewport has its real size by this frame


def main():
    """Run the app until its window is closed (or for --smoke seconds)."""
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--smoke", type=float, default=0.0,
                        help="replay test lines, no port, and close after this many seconds")
    parser.add_argument("--screenshot", default=None, help="with --smoke: save a PNG")
    args = parser.parse_args()

    # The ini keeps what DearPyGui saves of the windows (docking, table columns); their
    # places are then set by the first workspace. Smoke tests leave the ini alone
    keeps_layout = not args.smoke
    has_layout = keeps_layout and LAYOUT_FILE.exists()
    if keeps_layout:
        LAYOUT_FILE.parent.mkdir(parents=True, exist_ok=True)

    dpg.create_context()
    dpg.configure_app(docking=True, docking_space=True,
                      **({"init_file": str(LAYOUT_FILE), "load_init_file": has_layout}
                         if keeps_layout else {}))
    dpg.create_viewport(title="Accelerometer calibrator", width=_VIEWPORT_SIZE[0],
                        height=_VIEWPORT_SIZE[1])
    app = App()
    app.menu_bar.show_all(False)    # Hidden until the layout of _LAYOUT_FRAME is set
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.maximize_viewport()
    if args.smoke:
        _feed_test_lines(app.board, args.smoke)

    start = time.perf_counter()
    frames = 0
    has_screenshot = False
    while dpg.is_dearpygui_running():
        frames += 1
        if frames == _LAYOUT_FRAME:
            # The first workspace, or the default layout; smoke tests always the default
            if args.smoke:
                app.menu_bar.default_layout()
            else:
                app.menu_bar.open_first_workspace()
        app.frame()
        dpg.render_dearpygui_frame()
        if args.smoke:
            elapsed_s = time.perf_counter() - start
            if args.screenshot and not has_screenshot and elapsed_s > args.smoke - 1:
                dpg.output_frame_buffer(file=args.screenshot)
                has_screenshot = True
            if elapsed_s > args.smoke:
                break

    if args.smoke:
        capture = app.board.capture
        print(f"smoke: {frames} frames, capture {capture.state} "
              f"{capture.measured_count}/{capture.expected_count}, {app.board.status}")
    else:
        dpg.save_init_file(str(LAYOUT_FILE))
    if app.board.is_connected:
        app.board.disconnect()
    dpg.destroy_context()


def _feed_test_lines(board, duration_s):
    """--smoke: the capture of the newest simple record as the board would send it, a
    still board and a warming sensor, put straight on the line queue. No port."""
    now = time.perf_counter()
    store = RecordStore(RECORD_DIR)
    for path in store.list_files():
        record = store.read(path)
        try:
            lines = format_capture(record["capture"])
            terms = tuple(term for term in TERMS if term in record["terms"])
        except (KeyError, TypeError, ValueError):
            continue
        if record.get("method") != "ellipsoid-fit":
            continue        # A compensated record's capture merges several temperatures
        board.terms = terms    # What Start would set from the ticks
        for line in lines:
            board.link.lines.put((now, line))
        break
    for k in range(int(duration_s * STREAM_RATE_HZ)):
        wobble_ug = int(1e3 * np.sin(k / 20))
        board.link.lines.put((now - duration_s + k / STREAM_RATE_HZ,
                              f"$READ,{wobble_ug},20000,1000000,0,0,1000000"))
    for k in range(int(duration_s)):
        board.link.lines.put((now - duration_s + k, f"$TEMP,{24310 + 110 * k}"))


if __name__ == "__main__":
    main()
