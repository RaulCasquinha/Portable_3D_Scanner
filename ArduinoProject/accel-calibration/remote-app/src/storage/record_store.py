"""
The record files: calibrations/<sensor>/<YYYYmmdd_HHMMSS>.json.

Layer: storage. Only adds files: a saved record is never changed or deleted
by the app.
"""

import json
import time
from pathlib import Path

__all__ = ["RecordStore"]

_FILE_TIME_FORMAT = "%Y%m%d_%H%M%S"


class RecordStore:
    """The record files under one folder, one subfolder per sensor."""

    def __init__(self, root):
        """
        Args:
            root: The folder of the records. Created on the first save.
        """
        self.root = Path(root)

    def list_sensors(self):
        """Names of the sensors that have a folder, sorted."""
        return sorted(path.name for path in self.root.glob("*") if path.is_dir())

    def list_files(self, sensor=None):
        """
        Record files, newest first (the names are their save times).

        Args:
            sensor: Only this sensor's files; None for all.
        """
        files = [path for path in self.root.glob("*/*.json")
                 if sensor is None or path.parent.name == sensor]
        return sorted(files, key=lambda path: (path.stem, path.parent.name), reverse=True)

    @staticmethod
    def read(path):
        """The record in a file, or None if it can't be read or isn't a dict."""
        try:
            record = json.loads(Path(path).read_text(encoding="utf-8"))
        except (OSError, ValueError):
            return None
        return record if isinstance(record, dict) else None

    def write(self, record):
        """
        Save a record of record["sensor"], named after the current time.

        Returns:
            The path of the new file.

        Raises:
            FileExistsError: A record of this sensor was saved this second.
            OSError: The file could not be written.
        """
        path = self.root / record["sensor"] / time.strftime(f"{_FILE_TIME_FORMAT}.json")
        if path.exists():
            raise FileExistsError("a record was saved this second: try again")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(record, indent=1), encoding="utf-8")
        return path

    def relative(self, path):
        """A record path as the user knows it: calibrations/<sensor>/<file>."""
        return Path(path).relative_to(self.root.parent).as_posix()
