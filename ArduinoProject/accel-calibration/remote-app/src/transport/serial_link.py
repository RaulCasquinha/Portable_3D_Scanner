"""
Serial port with a reader thread.

Layer: transport. The only module that imports pyserial. Received lines go to a
queue as (time, line) pairs, timed with time.perf_counter(), so the GUI thread
can drain them each frame without ever blocking on the port. The reader thread
is the only other thread in the app, and it touches nothing but the queue.
"""

import queue
import threading
import time

import serial
import serial.tools.list_ports

__all__ = ["BAUD_RATE", "PORT_CLOSED_LINE", "SerialLink", "SerialException"]

BAUD_RATE = 115200           # ST-LINK VCOM of the Nucleo, 8N1
PORT_CLOSED_LINE = "# port closed"   # Put on the queue when the port fails

SerialException = serial.SerialException

_READ_TIMEOUT_S = 0.1        # readline() returns at least this often, to see a close
_JOIN_TIMEOUT_S = 1.0        # Wait for the reader thread when closing


class SerialLink:
    """
    A serial port and the lines received on it.

    Attributes:
        lines: queue.Queue of (time_s, line) received, line without its line end.
            Tests may put lines on it to stand in for a board.
    """

    def __init__(self):
        self.lines = queue.Queue()
        self._port = None
        self._reader = None

    @staticmethod
    def list_ports():
        """Names of the serial ports on this computer, sorted."""
        return sorted(port.device for port in serial.tools.list_ports.comports())

    @property
    def is_open(self):
        """True while the port is open."""
        return self._port is not None and self._port.is_open

    def open(self, port_name):
        """
        Open a port and start reading it. An open port is closed first.

        Raises:
            SerialException: The port can't be opened.
        """
        self.close()
        self._port = serial.Serial(port_name, BAUD_RATE, timeout=_READ_TIMEOUT_S)
        self._reader = threading.Thread(target=self._read_lines, daemon=True)
        self._reader.start()

    def close(self):
        """Close the port and stop the reader. Does nothing if it is closed."""
        if self._port is not None:
            self._port.close()
        if self._reader is not None:
            self._reader.join(timeout=_JOIN_TIMEOUT_S)
        self._port, self._reader = None, None

    def send(self, text):
        """Send text as ASCII. Ignored while the port is closed."""
        port = self._port            # close() may clear it from another thread
        if port is not None and port.is_open:
            try:
                port.write(text.encode("ascii"))
            except (serial.SerialException, OSError):
                pass    # IGNORED: the port closed while sending; the reader reports it

    def send_sequence(self, steps):
        """
        Send several texts with pauses between them, without blocking the caller.

        Args:
            steps: (text, pause_s) pairs: each text is sent, then the pause is
                waited before the next one.
        """
        def send_all():
            for text, pause_s in steps:
                self.send(text)
                time.sleep(pause_s)

        threading.Thread(target=send_all, daemon=True).start()

    def _read_lines(self):
        """Reader thread: every received line goes on the queue until the port closes."""
        port = self._port            # close() may clear it from another thread
        while port.is_open:
            try:
                line = port.readline()
            except (serial.SerialException, TypeError, OSError):
                # The port was unplugged or closed under us: say so, in order with the lines
                self.lines.put((time.perf_counter(), PORT_CLOSED_LINE))
                break
            if line:
                self.lines.put((time.perf_counter(),
                                line.decode("ascii", errors="replace").strip()))
