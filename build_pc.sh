#!/bin/bash
# build_pc.sh - Build the Animal Crossing PC port
# Run from MSYS2 MINGW32 shell
#
# Prerequisites:
#   pacman -S mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-SDL2
#
# Usage:
#   1. ./build_pc.sh
#   2. Place your disc image (.ciso/.iso/.gcm) in pc/build32/bin/rom/
#   3. pc/build32/bin/AnimalCrossing.exe

set -e

if [ "$MSYSTEM" != "MINGW32" ]; then
    echo "Error: Must run from MSYS2 MINGW32 shell (not MINGW64 or MSYS)"
    echo "Open 'MSYS2 MINGW32' from your Start menu, then run this script again."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/pc/build32"
BIN_DIR="$BUILD_DIR/bin"

# --- CMake configure ---
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f Makefile ]; then
    echo "=== Configuring CMake ==="
    cmake .. -G "MinGW Makefiles"
fi

# --- Build ---
echo "=== Building PC port ==="
mingw32-make -j$(nproc)

# --- Create runtime directories ---
mkdir -p "$BIN_DIR/rom"
mkdir -p "$BIN_DIR/texture_pack"
mkdir -p "$BIN_DIR/save"

echo ""
echo "=== Build complete! ==="
echo ""
echo "Place your Animal Crossing disc image (.ciso/.iso/.gcm) in:"
echo "  pc/build32/bin/rom/"
echo ""
echo "Run: pc/build32/bin/AnimalCrossing.exe"
