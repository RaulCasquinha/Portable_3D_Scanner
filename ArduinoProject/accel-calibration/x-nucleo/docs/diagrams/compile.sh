#!/usr/bin/env bash
# Compile every D2 diagram of this folder into out/: <name>.d2 -> out/<name>.svg and .png.
#
#     bash Accel_Calib_stm32/docs/diagrams/compile.sh        (from any folder)
#
# out/ is emptied first, so a deleted or renamed diagram leaves no stale image.
# Needs d2 (https://d2lang.com) on the PATH.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"     # The folder of this script, wherever it is called from

if ! command -v d2 > /dev/null; then
    echo "d2 not found: install it from https://d2lang.com" >&2
    exit 1
fi

shopt -s nullglob
sources=(*.d2)
if [ ${#sources[@]} -eq 0 ]; then
    echo "no .d2 files in $(pwd)"
    exit 0
fi

mkdir -p out
rm -f out/*.svg out/*.png

for source in "${sources[@]}"; do
    name="${source%.d2}"
    for format in svg png; do
        # elk lays out nested containers (the layers) more cleanly than the default
        d2 --layout elk "$source" "out/$name.$format"
    done
done

echo "compiled ${#sources[@]} diagrams into $(pwd)/out"
