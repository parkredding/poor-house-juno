#!/bin/bash
# Build script for Web (Emscripten) platform

set -e

echo "========================================="
echo "Poor House Juno - Web Build"
echo "========================================="

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found!"
    echo "Please install Emscripten SDK and source emsdk_env.sh"
    echo ""
    echo "Example:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "Emscripten version: $(emcc --version | head -n 1)"
echo ""

# Create build directory
BUILD_DIR="build-web"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring..."
emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
emmake make -j$(nproc)

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo "Output files:"
echo "  - web/synth-processor.js"
echo "  - web/synth-processor.wasm"
echo ""
echo "To test, serve the web/ directory with a local HTTP server:"
echo "  cd web && python3 -m http.server 8000"
echo "Then open http://localhost:8000"
