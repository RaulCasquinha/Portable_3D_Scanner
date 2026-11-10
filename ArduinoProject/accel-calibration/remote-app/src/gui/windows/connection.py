"""
Connection window: choose the serial port of the board and open or close it, and
start or stop the board's stream of readings.

Layer: view.
"""

import dearpygui.dearpygui as dpg

from ...transport.serial_link import SerialException, SerialLink
from .base import Window

__all__ = ["ConnectionWindow"]

_PORT_WIDTH = 110


class ConnectionWindow(Window):
    """The serial port of the board, and its stream."""

    TAG = "win_connection"
    TITLE = "Connection"

    def __init__(self, window_id, board):
        """
        Args:
            window_id: The id reserved for this window.
            board: The Board to connect.
        """
        super().__init__(window_id)
        self._board = board
        with self.window():
            with dpg.group(horizontal=True):
                dpg.add_combo(SerialLink.list_ports(), tag="connection.port",
                              width=_PORT_WIDTH)
                dpg.add_button(label="Refresh", tag="connection.refresh_ports",
                               callback=lambda: dpg.configure_item(
                                   "connection.port", items=SerialLink.list_ports()))
                dpg.add_button(label="Connect", tag="connection.connect",
                               callback=self._on_connect_click)
            with dpg.group(horizontal=True):
                dpg.add_button(label="Stream (r)", tag="connection.stream",
                               callback=lambda: self._board.start_stream())
                dpg.add_button(label="Stop (s)", tag="connection.stop",
                               callback=lambda: self._board.stop_stream())

    def update(self, live):
        if not self._board.is_connected and dpg.get_item_label("connection.connect") != "Connect":
            dpg.set_item_label("connection.connect", "Connect")     # The port closed by itself

    def _on_connect_click(self):
        board = self._board
        if board.is_connected:
            board.disconnect()
            dpg.set_item_label("connection.connect", "Connect")
            return
        port = dpg.get_value("connection.port")
        try:
            board.connect(port)
        except (SerialException, ValueError) as error:
            board.status = f"cannot open {port}: {error}"
            return
        dpg.set_item_label("connection.connect", "Disconnect")
