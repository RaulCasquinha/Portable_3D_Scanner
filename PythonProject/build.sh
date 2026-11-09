#!/usr/bin/env bash
# Builds a standalone PortableScanner3D executable with PyInstaller.
# Usage: ./build.sh   (run from anywhere; always operates on this script's directory)
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

if [ ! -d .venv ]; then
    echo "Creating venv..."
    python3.12 -m venv .venv
fi
source .venv/bin/activate

pip install -q -r requirements-dev.txt

pyinstaller --onefile --windowed --name PortableScanner3D --collect-all dearpygui main.py

echo "Build complete: $(pwd)/dist/PortableScanner3D"
