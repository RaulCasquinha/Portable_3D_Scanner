"""
Where the app keeps its files. Every path the app uses is defined here.

Layer: storage. When the app runs as an executable built by PyInstaller, its
code is unpacked to a temporary folder, so the data folders are taken next to
the executable instead: compiled/calibrations, compiled/presets, compiled/workspaces.
Everything the app keeps is in its own folder, so it ships with it.
"""

import sys
from pathlib import Path

__all__ = ["APP_DIR", "RECORD_DIR", "PRESET_DIR", "LAYOUT_FILE", "WORKSPACE_DIR"]

if getattr(sys, "frozen", False):
    APP_DIR = Path(sys.executable).resolve().parent      # The folder of the .exe
else:
    APP_DIR = Path(__file__).resolve().parents[2]       # The folder of main.py

RECORD_DIR = APP_DIR / "calibrations"             # Saved calibrations, one folder per sensor
PRESET_DIR = APP_DIR / "presets"                  # Named settings
WORKSPACE_DIR = APP_DIR / "workspaces"            # Named window arrangements (Workspaces menu)
# What DearPyGui keeps of the windows (docking, table columns). It reads it only at start
# and writes it on a normal close. Per machine: not in git, not shipped
LAYOUT_FILE = APP_DIR / "app_layout.ini"
