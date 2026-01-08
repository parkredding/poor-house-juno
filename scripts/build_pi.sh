#!/bin/bash
# Build script for Raspberry Pi platform

set -e

echo "========================================="
echo "Poor House Juno - Raspberry Pi Build"
echo "========================================="

# Check for ALSA
if ! pkg-config --exists alsa; then
    echo "Warning: ALSA development files not found!"
    echo "On Raspberry Pi, install with:"
    echo "  sudo apt-get install libasound2-dev"
    echo ""
fi

# Create build directory
BUILD_DIR="build-pi"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring..."
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo "Output binary: build-pi/poor-house-juno"
echo ""
echo "To run on Raspberry Pi:"
echo "  ./build-pi/poor-house-juno"
echo ""
echo "Optional arguments:"
echo "  --audio <device>   ALSA audio device (default: 'default')"
echo "  --midi <device>    ALSA MIDI device (default: 'default')"
