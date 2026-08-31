#!/bin/bash
set -e
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "========================================"
echo "  Patika C Library Build (macOS)"
echo "  Type: $BUILD_TYPE"
echo "========================================"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cmake -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_C_COMPILER=gcc -Wall -Wextra

cmake --build "$BUILD_DIR" -j$(sysctl -n hw.logicalcpu)

echo ""
echo "Build complete!"
echo "  Library: $BUILD_DIR/libpatika_core.dylib"
echo ""
echo "To install: sudo cmake --install $BUILD_DIR"
