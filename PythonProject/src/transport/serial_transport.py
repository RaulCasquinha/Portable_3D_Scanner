"""Serial transport — reads raw bytes from USB-Serial in a background thread
and makes them available via a thread-safe queue."""

from __future__ import annotations

import queue
import threading
from typing import Callable

import serial
import serial.tools.list_ports


class SerialTransport:
    """Manages the serial port and exposes incoming bytes for polling.

    The worker thread is a daemon so it exits automatically when the
    main process ends. Call :meth:`read_pending` from the render loop
    to drain buffered bytes without blocking.

    Usage::

        t = SerialTransport()
        t.on_error = lambda msg: print(msg)
        t.open("/dev/ttyUSB0")
        ...
        data = t.read_pending()
        t.close()
    """

    def __init__(self) -> None:
        self._q: queue.Queue[bytes] = queue.Queue()
        self._thread: threading.Thread | None = None
        self._running = False
        self.on_error: Callable[[str], None] | None = None

    def open(self, port: str, baud: int = 115200) -> None:
        """Open *port* and start the background reader thread."""
        self.close()
        self._running = True
        self._thread = threading.Thread(
            target=self._run, args=(port, baud), daemon=True
        )
        self._thread.start()

    def close(self) -> None:
        """Stop the reader thread and close the port."""
        self._running = False
        if self._thread:
            self._thread.join(timeout=1.0)
            self._thread = None

    def read_pending(self) -> bytes:
        """Non-blocking drain — returns all bytes buffered since last call."""
        buf = bytearray()
        while not self._q.empty():
            try:
                buf += self._q.get_nowait()
            except queue.Empty:
                break
        return bytes(buf)

    @staticmethod
    def list_ports() -> list[str]:
        """Return available serial port names."""
        return [p.device for p in serial.tools.list_ports.comports()]

    # ------------------------------------------------------------------

    def _run(self, port: str, baud: int) -> None:
        try:
            ser = serial.Serial(port, baud, timeout=0.1)
        except serial.SerialException as exc:
            if self.on_error:
                self.on_error(str(exc))
            return
        try:
            while self._running:
                chunk = ser.read(64)
                if chunk:
                    self._q.put(chunk)
        except serial.SerialException as exc:
            if self.on_error:
                self.on_error(str(exc))
        finally:
            ser.close()
