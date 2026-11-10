"""
Build the app into one Windows executable: compiled/AccelCalib.exe.

    pip install -r requirements-dev.txt
    python build_exe.py

PyInstaller packs Python, the libraries and src/ into a single file, so the PC
that runs it needs no Python. The executable keeps its records, presets and workspaces
next to itself (compiled/calibrations, compiled/presets, compiled/workspaces; see
src/storage/paths.py); the presets and workspaces of this folder are copied there so
it starts with them. The build's
temporary files go in build/, which can be deleted.
"""

import shutil
from pathlib import Path

import PyInstaller.__main__

APP_DIR = Path(__file__).resolve().parent
COMPILED_DIR = APP_DIR / "compiled"
BUILD_DIR = APP_DIR / "build"
EXE_NAME = "AccelCalib"


def main():
    """Run PyInstaller, then copy the presets and workspaces next to the executable."""
    PyInstaller.__main__.run([
        str(APP_DIR / "main.py"),
        "--name", EXE_NAME,
        "--onefile",                     # Single .exe, no install needed
        "--windowed",                    # No console window behind the GUI
        "--noconfirm",                   # Replace the previous build without asking
        "--distpath", str(COMPILED_DIR),
        "--workpath", str(BUILD_DIR),
        "--specpath", str(BUILD_DIR),    # The generated .spec is a build file too
        "--paths", str(APP_DIR),         # So "import src..." is found
    ])
    for folder in ("presets", "workspaces"):
        _copy_new_files(folder)
    print(f"built {COMPILED_DIR / (EXE_NAME + '.exe')}")


def _copy_new_files(folder):
    """Copy the .json files of a folder that the executable doesn't have yet; never
    overwrite one."""
    target = COMPILED_DIR / folder
    target.mkdir(parents=True, exist_ok=True)
    for path in (APP_DIR / folder).glob("*.json"):
        if not (target / path.name).exists():
            shutil.copy2(path, target / path.name)


if __name__ == "__main__":
    main()
