"""
Board messages window: the last text messages from the board, newest at the bottom.

Layer: view.
"""

import dearpygui.dearpygui as dpg

from .base import Window

__all__ = ["BoardLogWindow"]


class BoardLogWindow(Window):
    """The board's messages."""

    TAG = "win_board_log"
    TITLE = "Board messages"

    def __init__(self, window_id, board):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board, whose log is shown.
        """
        super().__init__(window_id)
        self._board = board
        with self.window():
            with dpg.child_window(tag="board_log.window", border=False):
                dpg.add_text("", tag="board_log.text", wrap=0)   # 0: wrap at the window edge

    def update(self, live):
        log = self._board.log
        if self._changed("log", tuple(log)):
            dpg.set_value("board_log.text", "\n".join(log))
            dpg.set_y_scroll("board_log.window", -1.0)          # The newest is last
