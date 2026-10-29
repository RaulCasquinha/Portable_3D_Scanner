#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v doxygen &> /dev/null; then
    echo "Error: doxygen is not installed. Install it with:"
    echo "  nix-env -iA nixpkgs.doxygen   (NixOS)"
    echo "  sudo apt install doxygen       (Debian/Ubuntu)"
    exit 1
fi

cd "$SCRIPT_DIR"

echo "Generating documentation..."
doxygen Doxyfile

INDEX="$SCRIPT_DIR/doxy_docs/html/index.html"
echo "Done. Open: $INDEX"

# Open in browser if xdg-open is available
if command -v xdg-open &> /dev/null; then
    xdg-open "$INDEX"
fi
