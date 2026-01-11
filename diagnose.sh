#!/bin/bash
# Diagnostic script to check current installation state

echo "========================================="
echo "Poor House Juno - Diagnostic Check"
echo "========================================="
echo ""

# Check if binary exists
echo "1. Checking installed binary..."
if [ -f /usr/local/bin/poor-house-juno ]; then
    ls -lh /usr/local/bin/poor-house-juno
    echo ""
else
    echo "   ERROR: /usr/local/bin/poor-house-juno not found!"
    echo ""
fi

# Check running process
echo "2. Checking running process..."
ps aux | grep poor-house-juno | grep -v grep || echo "   No process running"
echo ""

# Check service status
echo "3. Checking service status..."
systemctl status poor-house-juno --no-pager | head -20
echo ""

# Check recent logs for sample rate
echo "4. Checking sample rate in logs..."
journalctl -u poor-house-juno --no-pager | grep -E "Sample rate:|Audio initialized:" | tail -3
echo ""

# Check for build timestamp
echo "5. Checking for build timestamp in logs..."
if journalctl -u poor-house-juno --no-pager | grep -q "Build timestamp"; then
    journalctl -u poor-house-juno --no-pager | grep "Build timestamp" | tail -1
    echo "   ✓ New binary is running!"
else
    echo "   ✗ Old binary (no build timestamp found)"
fi
echo ""

# Check audio devices
echo "6. Available audio devices..."
aplay -l | grep -E "^card|^  Subdevices"
echo ""

# Check current git branch
echo "7. Current git branch and status..."
cd ~/poor-house-juno 2>/dev/null && git branch --show-current && git status --short
echo ""

echo "========================================="
echo "Summary"
echo "========================================="
echo "If you see 'Sample rate: 0 Hz' above, there's an ALSA configuration issue."
echo "If you see 'Old binary', you need to rebuild."
echo ""
echo "To fix both issues, run: ./rebuild.sh"
echo ""
