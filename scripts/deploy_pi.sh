#!/bin/bash
# Deploy to Raspberry Pi via SCP

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <pi-hostname-or-ip>"
    echo "Example: $0 raspberrypi.local"
    echo "         $0 pi@192.168.1.100"
    exit 1
fi

PI_HOST="$1"
BINARY="build-pi/poor-house-juno"

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found at $BINARY"
    echo "Run ./scripts/build_pi.sh first"
    exit 1
fi

echo "Deploying to $PI_HOST..."
scp "$BINARY" "$PI_HOST:~/"

echo ""
echo "Deployment complete!"
echo "To run on Pi:"
echo "  ssh $PI_HOST"
echo "  ./poor-house-juno"
