#!/bin/bash
# Debug why the service is running old code

echo "========================================="
echo "Service Debug - Finding the Problem"
echo "========================================="
echo ""

echo "1. Check which binary the service is actually running:"
echo "---"
ps aux | grep poor-house-juno | grep -v grep | head -5
echo ""

echo "2. Check what systemd service file says:"
echo "---"
systemctl cat poor-house-juno 2>/dev/null || echo "Service file not found!"
echo ""

echo "3. Compare binary timestamps:"
echo "---"
echo "Build directory binary:"
ls -lh ~/poor-house-juno/build/poor-house-juno 2>/dev/null || echo "Not found"
echo ""
echo "Installed binary:"
ls -lh /usr/local/bin/poor-house-juno 2>/dev/null || echo "Not found"
echo ""
echo "Other locations:"
find /usr -name "poor-house-juno" -type f -executable 2>/dev/null | while read f; do
    ls -lh "$f"
done
echo ""

echo "4. Check running process executable:"
PID=$(pgrep -f poor-house-juno | head -1)
if [ -n "$PID" ]; then
    echo "Process $PID is running from:"
    ls -lh /proc/$PID/exe 2>/dev/null || readlink /proc/$PID/exe
else
    echo "No process found"
fi
echo ""

echo "========================================="
echo "Diagnosis"
echo "========================================="
echo "If the service is running a binary from a DIFFERENT location"
echo "than /usr/local/bin/poor-house-juno, that's the problem!"
echo ""
echo "Solution: Update the systemd service file to point to the right location."
echo ""
