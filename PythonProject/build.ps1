# Builds a standalone PortableScanner3D.exe with PyInstaller.
# Usage: .\build.ps1   (run from anywhere; always operates on this script's directory)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (-not (Test-Path .venv)) {
    Write-Host "Creating venv..."
    py -3.12 -m venv .venv
}
. .\.venv\Scripts\Activate.ps1

pip install -q -r requirements-dev.txt

pyinstaller --onefile --windowed --name PortableScanner3D --collect-all dearpygui main.py

Write-Host "Build complete: $PWD\dist\PortableScanner3D.exe"
