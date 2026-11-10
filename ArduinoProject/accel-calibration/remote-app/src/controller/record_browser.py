"""
The saved records as the Records and Temperature analysis windows see them:
which are listed, which one is selected, which are ticked.

Layer: controller. No GUI code. Rows are identified by their text (one line of
the list), so the selection and the ticks survive a refresh.
"""

import time

from ..processing.compensation import DEFAULT_SLOPE_GROUPS
from ..services.merge import compensated_record, usable_records
from ..services.report import record_row
from ..storage.file_names import clean_name
from ..storage.record_format import calibration_from_record, record_parameters, \
    record_temperature_c

__all__ = ["RecordBrowser"]

_SAVE_TIME_FORMAT = "%Y-%m-%dT%H:%M:%S"


class RecordBrowser:
    """
    The listed records, the selected one and the ticked ones.

    Attributes:
        rows: Row text: its record, or None if the file can't be read; newest first.
        selected_row: The row shown in detail, "" for none.
        checked_rows: The ticked rows, kept across refreshes and sensor filters.
        sensor: The sensor whose records are listed; None for all.
        version: Goes up whenever the rows, the selection or the ticks change, so the
            GUI knows when to redraw.
    """

    def __init__(self, store, board):
        """
        Args:
            store: The RecordStore of the records.
            board: The Board that loaded records go to, and that holds the status.
        """
        self._store = store
        self._board = board
        self._row_paths = {}
        # (row, calibration version) whose Load was pressed once over an unsaved calibration
        self._replace_unsaved_row = None
        self.rows = {}
        self.selected_row = ""
        self.checked_rows = set()
        self.sensor = None
        self.version = 0

    # -- listing --------------------------------------------------------------------

    def list_sensors(self):
        """Names of the sensors with records."""
        return self._store.list_sensors()

    def refresh(self, sensor=None):
        """
        Read the record files again.

        Args:
            sensor: List only this sensor's records; None for all. An unknown sensor
                lists all.
        """
        self.sensor = sensor if sensor in self._store.list_sensors() else None
        self.rows, self._row_paths = {}, {}
        for path in self._store.list_files(self.sensor):
            record = self._store.read(path)
            row = record_row(f"{path.parent.name}/{path.name}", record)
            self.rows[row] = record
            self._row_paths[row] = path
        if self.selected_row not in self.rows:
            self.selected_row = next(iter(self.rows), "")
        self.version += 1

    def select(self, row):
        """Show this row in detail."""
        self.selected_row = row
        self.version += 1

    def set_checked(self, row, is_checked):
        """Tick or untick a row for Temperature analysis."""
        if is_checked:
            self.checked_rows.add(row)
        else:
            self.checked_rows.discard(row)
        self.version += 1

    @property
    def selected_record(self):
        """The selected record, or None."""
        return self.rows.get(self.selected_row)

    def ticked_records(self):
        """The ticked records listed now (records of other sensors are hidden)."""
        return [record for row, record in self.rows.items() if row in self.checked_rows]

    def ticked_points(self):
        """
        The ticked simple records that have a temperature, as points for a plot.

        Returns:
            ([(temperature_c, parameters (12,))...], how many ticked records were left
            out: compensated, without a temperature, or unreadable).
        """
        ticked = self.ticked_records()
        points = [(record_temperature_c(record), record_parameters(record))
                  for record in usable_records(ticked)]
        return points, len(ticked) - len(points)

    # -- actions --------------------------------------------------------------------

    def load_selected(self):
        """
        Make the selected record the calibration in use. A calibration not saved yet
        is only replaced on a second press.

        Returns:
            The loaded record, or None if nothing was loaded.
        """
        board, row = self._board, self.selected_row
        record = self.rows.get(row)
        if record is None:
            board.status = "select a readable record first"
            return None
        warning = (row, board.calibration_version)   # Valid for this calibration only
        if board.is_unsaved and not board.capture.is_capturing \
                and self._replace_unsaved_row != warning:
            self._replace_unsaved_row = warning
            board.status = ("this calibration is not saved: Save record, or press Load again "
                            "to replace it")
            return None
        try:
            calibration, capture = calibration_from_record(record)
            name = f"{record['sensor']} {record['saved_at'].replace('T', ' ')}"
        except (KeyError, AttributeError) as error:
            board.status = f"this record cannot be loaded: {error!r}"
            return None
        except ValueError as error:
            board.status = str(error)
            return None
        if not board.use_calibration(calibration, capture, name):
            return None
        self._replace_unsaved_row = None
        return record

    def save_calibration(self, sensor):
        """
        Save the calibration in use as a record of this sensor.

        Args:
            sensor: The name typed by the user; unsafe characters are removed.

        Returns:
            True if it was saved.
        """
        board = self._board
        sensor = clean_name(sensor)
        if not sensor:
            board.status = "enter the sensor ID to save (letters, digits, - _ space)"
            return False
        record = board.to_record(sensor, time.strftime(_SAVE_TIME_FORMAT))
        if record is None:
            board.status = "nothing to save: calibrate first"
            return False
        path = self._write(record, "the record")
        if path is None:
            return False
        board.mark_saved()
        board.status = f"saved {self._store.relative(path)}"
        self.refresh(self.sensor)
        return True

    def create_compensated(self, slope_groups=DEFAULT_SLOPE_GROUPS, hold=False):
        """
        Merge the ticked records into a new temperature compensated record, saved
        and selected. See services/merge.py.

        Returns:
            True if it was saved.
        """
        board = self._board
        try:
            record = compensated_record(self.ticked_records(), slope_groups, hold)
        except ValueError as error:
            board.status = f"compensated calibration: {error}"
            return False
        path = self._write(record, "the compensated calibration")
        if path is None:
            return False
        self.refresh(self.sensor)
        new_row = next((row for row, p in self._row_paths.items() if p == path), None)
        if new_row is not None:
            self.select(new_row)
        board.status = (f"saved {self._store.relative(path)}: regression of "
                        f"{len(record['temperature_table'])} calibrations. Load it to use it")
        return True

    def _write(self, record, what):
        """Save a record; on failure say why in the status. The new path, or None."""
        try:
            return self._store.write(record)
        except FileExistsError as error:
            self._board.status = str(error)
        except OSError as error:
            self._board.status = f"could not save {what}: {error}"
        return None
