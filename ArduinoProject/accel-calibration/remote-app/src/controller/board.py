"""
The Nucleo as the app sees it: the connection, the reading stream, the capture
and the calibration in use.

Layer: controller. No GUI code. Each frame the GUI calls advance(), which drains
the lines the transport received, and then reads the state. Commands (connect,
start a capture, stream...) are methods. What happened is said in status, one
line for the user.
"""

import time
from collections import deque

import numpy as np

from ..processing.ellipsoid_fit import minimum_positions
from ..processing.sfem import APPLIED_TERMS, TERMS, CalibrationModel
from ..protocol.codec import (KEY_ABORT, KEY_CAPTURE, KEY_STOP, KEY_STREAM, MAX_POSITIONS,
                              STREAM_RATE_HZ, format_start, parse_line)
from ..protocol.messages import Reading, Temperature, Text
from ..services.calibrate import calibrate
from ..services.export import c_defines
from ..session.capture import ABORTED, DONE, Capture
from ..session.live_buffer import LiveBuffer
from ..session.temperature_log import TemperatureLog
from ..storage.record_format import record_from_calibration
from ..transport.serial_link import SerialLink

__all__ = ["CAL_SOURCE_PC", "CAL_SOURCE_BOARD", "CAL_SOURCES", "BUFFER_S", "Board"]

# Where the calibrated readings come from
CAL_SOURCE_PC = "PC fit"            # This app's calibration applied to the raw readings
CAL_SOURCE_BOARD = "board (flash)"  # The board's own, with the constants in its firmware
CAL_SOURCES = (CAL_SOURCE_PC, CAL_SOURCE_BOARD)

BUFFER_S = 10.0                     # Seconds of readings kept for the plots

_LOG_LINES = 12                     # Board messages kept for the log
_START_TIMEOUT_S = 1.5              # $START without $BEGIN after this: retry once, then report
_STREAM_PAUSE_S = 0.15              # The board polls its UART: let it stop streaming first


class Board:
    """
    The Nucleo and what the app knows about it.

    Attributes:
        status: One line for the user: what the board asks for, or what happened.
        log: The last text lines from the board and the app's own notes.
        capture: The capture running or last finished.
        buffer: The last BUFFER_S of readings.
        temperatures: The sensor's temperatures; empty if it has no thermometer.
        calibration: The Calibration in use, or None.
        calibration_version: Goes up whenever the calibration in use changes, so the
            GUI knows when to redraw.
        is_unsaved: True while a new fit has not been saved as a record.
        terms: The terms of the last capture or record, from sfem.TERMS.
        time_s: Seconds since the readings were last cleared.
    """

    def __init__(self, link=None):
        """
        Args:
            link: The transport. None: a new SerialLink. Tests pass one whose queue
                they fill.
        """
        self.link = link if link is not None else SerialLink()
        self.log = deque(maxlen=_LOG_LINES)
        self.status = "not connected"
        self.capture = Capture()
        self.buffer = LiveBuffer(int(BUFFER_S * STREAM_RATE_HZ))
        self.temperatures = TemperatureLog()
        self.calibration = None
        self.calibration_version = 0
        self.is_unsaved = False
        self.terms = TERMS
        self.time_s = 0.0
        self._samples_per_position = None
        self._start_time = time.perf_counter()
        self._pending_start = None      # (sent at, settings, retries left) until $BEGIN

    # -- state ------------------------------------------------------------------

    @property
    def is_connected(self):
        """True while the port is open."""
        return self.link.is_open

    @property
    def is_calibrated(self):
        """True while a calibration is in use."""
        return self.calibration is not None

    @property
    def model(self):
        """The model in use: the calibration's, or the identity."""
        return self.calibration.model if self.calibration else CalibrationModel()

    @property
    def temperature_now_c(self):
        """The sensor's last temperature, degC, or None."""
        return self.temperatures.latest_c

    def recent_raw(self, window_s):
        """Mean raw reading of the last window_s, (3,), g; +Z up before any reading."""
        mean = self.buffer.recent_mean("raw", window_s * STREAM_RATE_HZ)
        return np.array([0.0, 0.0, 1.0]) if mean is None else mean

    def recent_corrected(self, window_s, source, applied=APPLIED_TERMS):
        """
        Mean calibrated reading of the last window_s, (3,), g, from CAL_SOURCES.
        applied: the terms of this app's calibration to apply (sfem.APPLIED_TERMS);
        the board's own readings can't be changed.
        """
        if source == CAL_SOURCE_BOARD and len(self.buffer):
            return self.buffer.recent_mean("board_cal", window_s * STREAM_RATE_HZ)
        return self.model.with_terms(applied).apply(self.recent_raw(window_s))[0]

    def corrected(self, raw_g, board_cal_g, source, applied=APPLIED_TERMS):
        """
        Calibrated readings (n, 3), g: the board's, or this app's applied to raw_g
        with only the applied terms (sfem.APPLIED_TERMS).
        """
        if source == CAL_SOURCE_BOARD:
            return board_cal_g
        return self.model.with_terms(applied).apply(raw_g)

    # -- connection -----------------------------------------------------------------

    def connect(self, port_name):
        """
        Open the port and start the stream. A calibration in use is kept.

        Raises:
            SerialException, ValueError: The port can't be opened.
        """
        self.link.open(port_name)
        self._clear_readings()
        if self.capture.is_capturing:   # Cut off when the port closed
            self.capture = Capture()
            self._set_calibration(None)
        self.status = f"connected to {port_name}: Start"
        self.link.send(KEY_STREAM)

    def disconnect(self):
        """Stop the stream and close the port."""
        self.link.send(KEY_STOP)
        self.link.close()
        self.temperatures = TemperatureLog()
        self.status = "not connected"

    # -- commands -------------------------------------------------------------------

    def start_capture(self, positions, samples, references, terms):
        """
        Ask the board to start a capture.

        Args:
            positions: Positions to capture, enough for the terms, up to MAX_POSITIONS.
            samples: Samples averaged per position.
            references: Reference positions, 0 to 2.
            terms: The terms the fit will estimate, from sfem.TERMS.
        """
        needed = minimum_positions(terms)
        if not needed <= positions <= MAX_POSITIONS:
            self.status = f"use {needed} (terms ticked) to {MAX_POSITIONS} positions, not {positions}"
            return
        if self.capture.is_capturing:
            self.status = "a capture is running: Abort (q) first"
            return
        if not self.is_connected:
            self.status = "not connected"
            return
        try:
            format_start(positions, samples, references)    # Checks the firmware's limits
        except ValueError as error:
            self.status = str(error)
            return
        self.terms = tuple(terms)
        self._samples_per_position = samples
        self.capture.clear()        # The old positions must not show as part of the new capture
        self._send_start((positions, samples, references), retries=1)
        self.status = f"starting: {positions} positions, {samples} samples"

    def capture_position(self):
        """Ask the board to measure the position it is in (the d key)."""
        self.link.send(KEY_CAPTURE)

    def abort(self):
        """Stop the capture (the q key)."""
        self.link.send(KEY_ABORT)
        self._pending_start = None
        self.status = "aborted"

    def start_stream(self):
        """Ask the board to stream its readings (the r key)."""
        if self._is_connected_or_say_so():
            self.link.send(KEY_STREAM)
            self.status = "streaming: Stop (s)"

    def stop_stream(self):
        """Ask the board to stop streaming (the s key)."""
        if self._is_connected_or_say_so():
            self.link.send(KEY_STOP)
            self.status = "stream stopped: Stream (r)"

    def refit(self, terms):
        """Fit the last capture again with these terms."""
        self.terms = tuple(terms)
        self._fit()

    def use_calibration(self, calibration, capture, name):
        """
        Make a loaded calibration the one in use.

        Args:
            calibration: The Calibration; a compensated one moves to the sensor's
                temperature as soon as it is known.
            capture: The Capture it was fitted to.
            name: What to call it in the status line.

        Returns:
            False if a capture is running, which must not be replaced.
        """
        if self.capture.is_capturing:
            self.status = "a capture is running: Abort (q) before loading a record"
            return False
        if calibration.is_compensated and self.temperature_now_c is not None:
            calibration = calibration.at_temperature(self.temperature_now_c)
        self.capture = capture
        self.terms = calibration.terms
        self._samples_per_position = calibration.samples_per_position
        self._set_calibration(calibration)
        self.status = f"loaded {name}"
        return True

    def to_record(self, sensor, saved_at):
        """The record of the calibration in use, to save. None if there is none."""
        if self.calibration is None:
            return None
        return record_from_calibration(self.calibration, self.capture, sensor, saved_at,
                                       self.defines())

    def defines(self):
        """The C defines of the calibration in use, "" if there is none."""
        calibration = self.calibration
        if calibration is None:
            return ""
        return c_defines(calibration.model, calibration.temperature_c,
                         is_compensated=calibration.is_compensated)

    def mark_saved(self):
        """The calibration in use was saved as a record."""
        self.is_unsaved = False

    # -- received lines -------------------------------------------------------------

    def advance(self):
        """Handle every line received since the last call. Call once per frame."""
        self.time_s = time.perf_counter() - self._start_time
        times_s, raw_g, board_cal_g = [], [], []
        while not self.link.lines.empty():
            received_at, line = self.link.lines.get_nowait()
            try:
                message = parse_line(line)
            except ValueError:
                self.log.append(f"# bad line: {line}")
                continue
            if isinstance(message, Reading):    # The most frequent: collected, then added at once
                times_s.append(received_at - self._start_time)
                raw_g.append(message.raw_g)
                board_cal_g.append(message.board_cal_g)
            elif isinstance(message, Temperature):
                self.temperatures.append(received_at - self._start_time, message.temperature_c)
            else:
                self._handle(message)
        self.buffer.append(times_s, raw_g, board_cal_g)
        self._check_start()
        self._follow_temperature()

    # -- private --------------------------------------------------------------------

    def _handle(self, message):
        """A message about the capture, or a text for the user."""
        if self.capture.handle(message):
            if self.capture.is_capturing and self.capture.measured_count == 0:
                # $BEGIN: the board started; the old calibration no longer applies
                self._set_calibration(None)
                self._pending_start = None
            elif self.capture.state == DONE:
                self._fit()
            elif self.capture.state == ABORTED and self._pending_start is None:
                self.status = "capture aborted"     # Not the abort a new Start just caused
        elif isinstance(message, Text) and message.text:
            self.log.append(message.text)
            if self.capture.state != DONE:     # Keep the result of the fit in view
                self.status = message.text.strip()

    def _fit(self):
        """Calibrate the capture; on failure no calibration is in use."""
        try:
            calibration = calibrate(self.capture, self.terms, self._samples_per_position)
        except ValueError as error:
            self._set_calibration(None)
            self.status = f"fit failed: {error}"
            return
        self._set_calibration(calibration)
        self.is_unsaved = True
        self.status = (f"calibrated: |g| error {calibration.rms_before_mg:.3f} mg before, "
                       f"{calibration.rms_after_mg:.3f} mg after")

    def _set_calibration(self, calibration):
        self.calibration = calibration
        self.is_unsaved = False
        self.calibration_version += 1

    def _follow_temperature(self):
        """A compensated calibration moves to each new temperature of the sensor."""
        now_c = self.temperature_now_c
        calibration = self.calibration
        if (calibration is not None and calibration.is_compensated and now_c is not None
                and now_c != calibration.temperature_c):
            self.calibration = calibration.at_temperature(now_c)
            self.calibration_version += 1

    def _send_start(self, settings, retries):
        # q first: a capture still running on the board (after a reconnect, or a lost q)
        # would ignore $START, so it is aborted; with none running the board ignores q.
        # Then s: the board polls its UART and could miss the command while streaming.
        self.link.send_sequence([(KEY_ABORT, _STREAM_PAUSE_S), (KEY_STOP, _STREAM_PAUSE_S),
                                 (format_start(*settings), 0.0)])
        self.log.append("> $START,{},{},{}".format(*settings))
        self._pending_start = (time.perf_counter(), settings, retries)

    def _check_start(self):
        """No $BEGIN after a $START: send it again once, then say so."""
        if self._pending_start is None:
            return
        sent_at, settings, retries = self._pending_start
        if time.perf_counter() - sent_at < _START_TIMEOUT_S:
            return
        if retries > 0:
            self.log.append("# no $BEGIN from the board, sending $START again")
            self._send_start(settings, retries - 1)
        else:
            self._pending_start = None
            self.status = "the board did not start the capture: check its messages"

    def _clear_readings(self):
        self._start_time = time.perf_counter()
        self.time_s = 0.0
        self.buffer.clear()
        self.temperatures = TemperatureLog()
        self.log.clear()
        self._pending_start = None

    def _is_connected_or_say_so(self):
        if not self.is_connected:
            self.status = "not connected"
        return self.is_connected
