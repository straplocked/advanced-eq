#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

VST3_DIR="$HOME/.vst3"
BIN_DIR="$HOME/.local/bin"

echo "Installing Advanced EQ..."

# VST3
if [ -d "$SCRIPT_DIR/Advanced EQ.vst3" ]; then
    mkdir -p "$VST3_DIR"
    cp -r "$SCRIPT_DIR/Advanced EQ.vst3" "$VST3_DIR/"
    echo "  VST3 -> $VST3_DIR/Advanced EQ.vst3"
fi

# Standalone
if [ -f "$SCRIPT_DIR/AdvancedEQ" ]; then
    mkdir -p "$BIN_DIR"
    cp "$SCRIPT_DIR/AdvancedEQ" "$BIN_DIR/"
    chmod +x "$BIN_DIR/AdvancedEQ"
    echo "  Standalone -> $BIN_DIR/AdvancedEQ"
fi

echo "Done! Restart your DAW and rescan plugins."
