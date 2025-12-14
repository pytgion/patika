#!/bin/bash

set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "========================================"
echo "  Patika C Library Build"
echo "  Type: $BUILD_TYPE"
echo "========================================"

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
cmake --build "$BUILD_DIR" -j$(nproc 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "  Library: $BUILD_DIR/libpatika_core.so"
echo ""
echo "To install: sudo cmake --install $BUILD_DIR"
