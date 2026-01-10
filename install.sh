#!/bin/bash
# Poor House Juno - One-Line Installer for Raspberry Pi
# Usage: curl -sSL https://raw.githubusercontent.com/parkredding/poor-house-juno/main/install.sh | bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
INSTALL_DIR="${HOME}/poor-house-juno"
REPO_URL="https://github.com/parkredding/poor-house-juno.git"
BRANCH="${BRANCH:-main}"

# Helper functions
print_header() {
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}=========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

print_step() {
    echo -e "\n${BLUE}[Step $1/$2]${NC} $3"
}

# Check if running on Raspberry Pi
check_platform() {
    if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null && ! grep -q "BCM" /proc/cpuinfo 2>/dev/null; then
        print_error "This script is designed for Raspberry Pi"
        print_info "Detected platform: $(uname -m)"
        read -r -p "Continue anyway? (y/n) " REPLY
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# Main installation
main() {
    print_header "Poor House Juno - Raspberry Pi Installer"
    echo ""
    echo "This will install Poor House Juno synthesizer on your Raspberry Pi"
    echo "Installation directory: ${INSTALL_DIR}"
    echo ""

    check_platform

    # Step 1: Update system
    print_step 1 5 "Updating package list"
    sudo apt-get update -qq

    # Step 2: Install dependencies
    print_step 2 5 "Installing dependencies"
    print_info "Installing: build-essential, cmake, git, libasound2-dev, pkg-config"

    sudo apt-get install -y -qq \
        build-essential \
        cmake \
        git \
        libasound2-dev \
        pkg-config \
        alsa-utils 2>&1 | grep -v "Setting up\|Processing\|Preparing" || true

    print_success "Dependencies installed"

    # Step 3: Clone or update repository
    print_step 3 5 "Getting source code"

    if [ -d "$INSTALL_DIR" ]; then
        print_info "Directory exists, updating..."
        cd "$INSTALL_DIR"
        git fetch origin
        git checkout "$BRANCH"
        git pull origin "$BRANCH"
        print_success "Repository updated"
    else
        print_info "Cloning repository..."
        git clone -b "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
        cd "$INSTALL_DIR"
        print_success "Repository cloned"
    fi

    # Step 4: Build the project
    print_step 4 5 "Building Poor House Juno"
    print_info "This may take 5-10 minutes on Raspberry Pi..."

    # Clean previous build
    rm -rf build-pi
    mkdir -p build-pi
    cd build-pi

    # Build with progress
    cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)

    print_success "Build completed"

    # Step 5: Test installation
    print_step 5 5 "Verifying installation"

    if [ -f "${INSTALL_DIR}/build-pi/poor-house-juno" ]; then
        print_success "Binary created successfully"
    else
        print_error "Build failed - binary not found"
        exit 1
    fi

    # Summary
    echo ""
    print_header "Installation Complete!"
    echo ""
    print_success "Poor House Juno is ready to use"
    echo ""
    echo -e "${GREEN}Quick Start:${NC}"
    echo "  cd ${INSTALL_DIR}"
    echo "  ./build-pi/poor-house-juno --audio hw:0,0 --midi hw:1,0,0"
    echo ""
    echo -e "${GREEN}Useful Commands:${NC}"
    echo "  List MIDI devices:  amidi -l"
    echo "  List audio devices: aplay -l"
    echo "  Test audio:         speaker-test -D hw:0,0 -c 2"
    echo "  Adjust volume:      alsamixer"
    echo ""
    echo -e "${GREEN}Documentation:${NC}"
    echo "  Quick start:  ${INSTALL_DIR}/DEPLOY.md"
    echo "  Full guide:   ${INSTALL_DIR}/README.md"
    echo ""

    # Optional: Auto-start setup
    echo -e "${YELLOW}Optional:${NC} Set up auto-start on boot?"
    read -r -p "Configure Poor House Juno to start automatically? (y/n) " REPLY
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        setup_autostart
    fi

    echo ""
    print_header "Ready to Rock! 🎹"
}

# Setup systemd service for auto-start
setup_autostart() {
    print_info "Setting up auto-start..."

    # Prompt for MIDI device
    echo ""
    echo "Available MIDI devices:"
    amidi -l || true
    echo ""
    read -r -p "Enter MIDI device (e.g., hw:1,0,0) or press Enter for auto-detect: " MIDI_DEV
    MIDI_DEV=${MIDI_DEV:-hw:1,0,0}

    # Create systemd service
    SERVICE_FILE="/tmp/poor-house-juno.service"
    cat > "$SERVICE_FILE" << EOF
[Unit]
Description=Poor House Juno Synthesizer
After=sound.target

[Service]
Type=simple
User=${USER}
WorkingDirectory=${INSTALL_DIR}
ExecStart=${INSTALL_DIR}/build-pi/poor-house-juno --audio hw:0,0 --midi ${MIDI_DEV}
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

    sudo mv "$SERVICE_FILE" /etc/systemd/system/poor-house-juno.service
    sudo systemctl daemon-reload
    sudo systemctl enable poor-house-juno
    sudo systemctl start poor-house-juno

    print_success "Auto-start configured"
    echo ""
    echo "Service commands:"
    echo "  Status:  sudo systemctl status poor-house-juno"
    echo "  Stop:    sudo systemctl stop poor-house-juno"
    echo "  Disable: sudo systemctl disable poor-house-juno"
    echo "  Logs:    journalctl -u poor-house-juno -f"
}

# Run main installation
main "$@"
