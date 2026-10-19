#!/bin/sh
# Installs PitchShifter for the current user (or system-wide when run with sudo).
set -e
cd "$(dirname "$0")"

if [ "$(id -u)" -eq 0 ]; then
    VST3_DIR=/usr/local/lib/vst3
    BIN_DIR=/usr/local/bin
    DATA_DIR=/usr/local/share/PitchShifter
else
    VST3_DIR="$HOME/.vst3"
    BIN_DIR="$HOME/.local/bin"
    DATA_DIR="$HOME/.local/share/PitchShifter"
fi

mkdir -p "$VST3_DIR" "$BIN_DIR" "$DATA_DIR"
rm -rf "$VST3_DIR/PitchShifter.vst3"
cp -R PitchShifter.vst3 "$VST3_DIR/"
cp PitchShifter "$BIN_DIR/"
cp -R Presets PitchShifter-Manual.pdf "$DATA_DIR/"

echo "VST3:       $VST3_DIR/PitchShifter.vst3"
echo "Standalone: $BIN_DIR/PitchShifter"
echo "Presets:    $DATA_DIR/Presets"
