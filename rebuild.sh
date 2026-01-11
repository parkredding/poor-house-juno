#!/bin/bash
# Rebuild and reinstall poor-house-juno with filter stability fix

set -e

echo "======================================="
echo "Poor House Juno - Rebuild Script"
echo "======================================="
echo ""
echo "This will rebuild with the filter stability fix"
echo "that prevents NaN/static audio output."
echo ""

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Pull latest changes
echo "Pulling latest changes from git..."
git fetch origin
git pull origin claude/debug-audio-output-u5Gzx || {
    echo "Warning: Could not pull from branch, continuing with current code..."
}

# Clean build directory
echo ""
echo "======================================="
echo "Cleaning and Building"
echo "======================================="
echo "Cleaning build directory..."
rm -rf build
mkdir -p build

# Build
echo "Building with $(nproc) parallel jobs..."
cd build

if ! cmake ..; then
    echo ""
    echo "ERROR: CMake configuration failed!"
    echo "You may need to install dependencies:"
    echo "  sudo apt-get install cmake g++ libasound2-dev"
    exit 1
fi

if ! make -j$(nproc); then
    echo ""
    echo "ERROR: Build failed!"
    echo "Check the error messages above."
    exit 1
fi

if [ ! -f poor-house-juno ]; then
    echo ""
    echo "ERROR: Binary was not created!"
    exit 1
fi

echo ""
echo "Build successful! Binary info:"
ls -lh poor-house-juno

echo ""
echo "======================================="
echo "Installing Binary"
echo "======================================="

# Stop service if running
echo "Stopping service..."
sudo systemctl stop poor-house-juno || true

# Install binary
echo "Installing binary to /usr/local/bin/..."
sudo cp poor-house-juno /usr/local/bin/
sudo chmod +x /usr/local/bin/poor-house-juno

# Verify installation
if [ -f /usr/local/bin/poor-house-juno ]; then
    echo "Binary installed successfully!"
    ls -lh /usr/local/bin/poor-house-juno
else
    echo "ERROR: Binary installation failed!"
    exit 1
fi

# Restart service
echo ""
echo "Starting service..."
sudo systemctl start poor-house-juno

# Give it a moment to start
sleep 2

echo ""
echo "======================================="
echo "Rebuild Complete!"
echo "======================================="
echo ""
echo "The service has been rebuilt and restarted with:"
echo "  ✓ Filter stability fix (prevents NaN/static)"
echo "  ✓ Build timestamp output for verification"
echo "  ✓ Additional safety validations"
echo ""
echo "VERIFY THE FIX:"
echo "  1. Check logs for build timestamp (should be TODAY):"
echo "     sudo journalctl -u poor-house-juno -n 30"
echo ""
echo "  2. Watch live output:"
echo "     sudo journalctl -u poor-house-juno -f"
echo ""
echo "  3. Look for 'Build timestamp: Jan 11 2026' in the logs"
echo "     (If you see this, the new binary is running!)"
echo ""
echo "  4. Listen to the test chord - should be CLEAR, not static"
echo ""
echo "If you still hear static and the timestamp is old,"
echo "the new binary may not be running. Check:"
echo "  sudo systemctl status poor-house-juno"
echo ""
