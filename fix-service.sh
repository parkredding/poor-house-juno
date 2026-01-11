#!/bin/bash
# Fix the systemd service to use the correct binary location

set -e

echo "========================================="
echo "Fixing Systemd Service Configuration"
echo "========================================="
echo ""

SERVICE_FILE="/etc/systemd/system/poor-house-juno.service"

echo "Creating updated service file..."
echo ""

# Create the corrected service file
sudo tee "$SERVICE_FILE" > /dev/null << 'EOF'
[Unit]
Description=Poor House Juno Synthesizer
After=sound.target

[Service]
Type=simple
User=user
WorkingDirectory=/home/user/poor-house-juno
ExecStart=/usr/local/bin/poor-house-juno --audio hw:2,0 --midi hw:1,0,0
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

echo "✓ Service file updated"
echo ""
echo "Reloading systemd daemon..."
sudo systemctl daemon-reload
echo "✓ Systemd reloaded"
echo ""

echo "Stopping old service..."
sudo systemctl stop poor-house-juno
echo "✓ Service stopped"
echo ""

echo "Starting service with new binary..."
sudo systemctl start poor-house-juno
echo "✓ Service started"
echo ""

sleep 2

echo "========================================="
echo "Service Fixed!"
echo "========================================="
echo ""
echo "The service now uses: /usr/local/bin/poor-house-juno"
echo ""
echo "VERIFY IT WORKED:"
echo "  1. Check that it's running the right binary:"
echo "     ps aux | grep poor-house-juno | grep -v grep"
echo ""
echo "  2. Check logs for build timestamp:"
echo "     sudo journalctl -u poor-house-juno -n 30"
echo ""
echo "  3. You should now see:"
echo "     - 'Build timestamp: Jan 11 2026'"
echo "     - 'Audio initialized: 48000 Hz' (NOT 0 Hz!)"
echo ""
echo "  4. Listen - should be CLEAR audio, not static!"
echo ""
