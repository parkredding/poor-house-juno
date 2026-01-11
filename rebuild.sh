#!/bin/bash
# Rebuild and reinstall poor-house-juno

set -e

echo "======================================="
echo "Rebuilding Poor House Juno"
echo "======================================="

# Clean build directory
echo "Cleaning build directory..."
rm -rf build
mkdir -p build

# Build
echo "Building..."
cd build
cmake ..
make -j$(nproc)

echo ""
echo "======================================="
echo "Installing"
echo "======================================="

# Stop service if running
echo "Stopping service..."
sudo systemctl stop poor-house-juno || true

# Install binary
echo "Installing binary..."
sudo cp poor-house-juno /usr/local/bin/

# Restart service
echo "Starting service..."
sudo systemctl start poor-house-juno

echo ""
echo "======================================="
echo "Build Complete!"
echo "======================================="
echo "The service has been restarted with the latest code."
echo ""
echo "Check status with: sudo systemctl status poor-house-juno"
echo "View logs with: sudo journalctl -u poor-house-juno -f"
