"""
Named presets of settings: presets/<name>.json, a dict of setting name to value.
The same store keeps the workspaces (window arrangements) in their own folder.

Layer: storage. It doesn't know what the dicts hold; the GUI chooses.
"""

import json
from pathlib import Path

from .file_names import clean_name

__all__ = ["PresetStore"]


class PresetStore:
    """The preset files in one folder."""

    def __init__(self, root):
        """
        Args:
            root: The folder of the presets. Created on the first save.
        """
        self.root = Path(root)

    def list_names(self):
        """The names of the saved presets, sorted."""
        return sorted(path.stem for path in self.root.glob("*.json"))

    def read(self, name):
        """
        The settings of a preset.

        Raises:
            OSError, ValueError: The file can't be read or is not a dict.
        """
        values = json.loads(self._path(name).read_text(encoding="utf-8"))
        if not isinstance(values, dict):
            raise ValueError("not a preset")
        return values

    def write(self, name, values):
        """
        Save settings under a name, replacing a preset of that name.

        Raises:
            ValueError: The name has no usable characters.
            OSError: The file could not be written.
        """
        name = clean_name(name)
        if not name:
            raise ValueError("enter a name (letters, digits, - _ space)")
        self.root.mkdir(parents=True, exist_ok=True)
        self._path(name).write_text(json.dumps(values, indent=2), encoding="utf-8")
        return name

    def delete(self, name):
        """Delete a preset. Raises OSError if it can't."""
        self._path(name).unlink()

    def _path(self, name):
        return self.root / f"{clean_name(name)}.json"
