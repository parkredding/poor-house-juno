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

# Detect and test audio devices to find the best one
detect_audio_device() {
    local best_device=""
    local best_score=0

    # Parse aplay -l output to extract all available devices
    while IFS= read -r line; do
        # Match lines like: "card 0: vc4hdmi0 [vc4-hdmi-0], device 0:"
        if [[ $line =~ ^card\ ([0-9]+):\ ([^,]+).*device\ ([0-9]+): ]]; then
            local card="${BASH_REMATCH[1]}"
            local card_name="${BASH_REMATCH[2]}"
            local device="${BASH_REMATCH[3]}"
            local hw_id="hw:${card},${device}"

            # Test if device works with stereo playback
            print_info "Testing ${hw_id} (${card_name})..."

            # Quick test: try to open device with speaker-test
            # Use timeout to limit test duration, and -l 1 for just one iteration
            # Increased timeout to 3 seconds for devices that need longer initialization (like bcm2835)
            local test_passed=false

            # First attempt: Standard test with 3 second timeout
            if timeout 3 speaker-test -D "$hw_id" -c 2 -r 48000 -t sine -l 1 >/dev/null 2>&1; then
                test_passed=true
            # Second attempt for bcm2835 devices: Try with different parameters
            elif [[ $card_name =~ (bcm2835|Headphones) ]] && timeout 3 speaker-test -D "$hw_id" -c 2 -r 44100 -t sine -l 1 >/dev/null 2>&1; then
                test_passed=true
            # Third attempt: bcm2835/Headphones devices often work even if speaker-test is finicky
            # We'll trust them and let the actual app test fail if there's a real issue
            elif [[ $card_name =~ (bcm2835|Headphones|Analog) ]]; then
                print_info "  ${hw_id} detection uncertain, but bcm2835/analog devices usually work"
                test_passed=true
            fi

            if [ "$test_passed" = true ]; then
                # Device works! Now score it based on type
                local score=1
                local description="${card_name}"

                # Scoring system (higher is better):
                # USB audio interfaces: 100
                # Analog/headphones: 50
                # HDMI: 10

                if [[ $card_name =~ (USB|Audio) ]]; then
                    score=100
                    description="USB Audio"
                elif [[ $card_name =~ (Headphones|bcm2835|Analog) ]]; then
                    score=50
                    description="Headphones/Analog"
                elif [[ $card_name =~ (HDMI|hdmi|vc4) ]]; then
                    score=10
                    description="HDMI"
                fi

                print_success "  ${hw_id} works (${description})"

                # Keep track of best device
                if [ $score -gt $best_score ]; then
                    best_score=$score
                    best_device=$hw_id
                fi
            else
                print_info "  ${hw_id} not compatible"
            fi
        fi
    done < <(aplay -l 2>/dev/null)

    echo "$best_device"
}

# Test if a specific audio device works with Poor House Juno requirements
test_audio_device() {
    local device="$1"

    # Quick compatibility test using speaker-test
    # We test with the same parameters Poor House Juno needs:
    # - Stereo (2 channels)
    # - 48kHz sample rate
    # Increased timeout to 3 seconds for devices that need longer initialization
    if timeout 3 speaker-test -D "$device" -c 2 -r 48000 -t sine -l 1 >/dev/null 2>&1; then
        return 0
    # Try alternative sample rate for bcm2835 devices
    elif timeout 3 speaker-test -D "$device" -c 2 -r 44100 -t sine -l 1 >/dev/null 2>&1; then
        return 0
    else
        return 1
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
    print_step 5 6 "Verifying installation"

    if [ -f "${INSTALL_DIR}/build-pi/poor-house-juno" ]; then
        print_success "Binary created successfully"
    else
        print_error "Build failed - binary not found"
        exit 1
    fi

    # Step 6: Detect and test audio devices
    print_step 6 6 "Detecting audio devices"

    # Collect all available audio devices
    declare -a AUDIO_DEVICES
    declare -a AUDIO_DESCRIPTIONS
    local device_count=0

    while IFS= read -r line; do
        if [[ $line =~ ^card\ ([0-9]+):\ ([^,]+).*device\ ([0-9]+): ]]; then
            local card="${BASH_REMATCH[1]}"
            local card_name="${BASH_REMATCH[2]}"
            local device="${BASH_REMATCH[3]}"
            local hw_id="hw:${card},${device}"

            AUDIO_DEVICES[$device_count]="$hw_id"
            AUDIO_DESCRIPTIONS[$device_count]="$card_name"
            ((device_count++))
        fi
    done < <(aplay -l 2>/dev/null)

    # Auto-detect recommended device
    RECOMMENDED_AUDIO=$(detect_audio_device)

    if [ -n "$RECOMMENDED_AUDIO" ]; then
        print_success "Recommended audio device: ${RECOMMENDED_AUDIO}"
    else
        print_info "Using default ALSA device"
        RECOMMENDED_AUDIO="default"
    fi

    # Prompt user to select or confirm audio device
    SELECTED_AUDIO="$RECOMMENDED_AUDIO"
    if [ $device_count -gt 0 ]; then
        echo ""
        echo "Available audio devices:"
        for i in "${!AUDIO_DEVICES[@]}"; do
            local marker=""
            if [ "${AUDIO_DEVICES[$i]}" = "$RECOMMENDED_AUDIO" ]; then
                marker=" [recommended]"
            fi
            echo "  $((i+1)). ${AUDIO_DEVICES[$i]} - ${AUDIO_DESCRIPTIONS[$i]}${marker}"
        done
        echo "  $((device_count+1)). default (ALSA default)"
        echo ""

        # Interactive selection with validation
        while true; do
            read -r -p "Select audio device (1-$((device_count+1))) or press Enter for recommended [${RECOMMENDED_AUDIO}]: " AUDIO_CHOICE

            if [ -z "$AUDIO_CHOICE" ]; then
                SELECTED_AUDIO="$RECOMMENDED_AUDIO"
                break
            elif [[ "$AUDIO_CHOICE" =~ ^[0-9]+$ ]] && [ "$AUDIO_CHOICE" -ge 1 ] && [ "$AUDIO_CHOICE" -le $((device_count+1)) ]; then
                if [ "$AUDIO_CHOICE" -eq $((device_count+1)) ]; then
                    SELECTED_AUDIO="default"
                else
                    SELECTED_AUDIO="${AUDIO_DEVICES[$((AUDIO_CHOICE-1))]}"
                fi
                break
            else
                echo "Invalid selection. Please enter a number between 1 and $((device_count+1))."
            fi
        done

        print_success "Selected audio device: ${SELECTED_AUDIO}"
    fi

    # Save configuration
    CONFIG_DIR="${HOME}/.config/poor-house-juno"
    CONFIG_FILE="${CONFIG_DIR}/config"
    mkdir -p "$CONFIG_DIR"
    cat > "$CONFIG_FILE" << EOF
# Poor House Juno Configuration
# This file is auto-generated by the installer
# You can edit it manually or re-run the installer

# Audio device (e.g., hw:2,0 or default)
AUDIO_DEVICE=${SELECTED_AUDIO}

# MIDI device (e.g., hw:1,0,0 or default)
# Leave empty for auto-detection
MIDI_DEVICE=
EOF
    print_success "Configuration saved to ${CONFIG_FILE}"

    # Summary
    echo ""
    print_header "Installation Complete!"
    echo ""
    print_success "Poor House Juno is ready to use"
    echo ""
    echo -e "${GREEN}Quick Start:${NC}"
    echo "  cd ${INSTALL_DIR}"
    if [ "$SELECTED_AUDIO" != "default" ]; then
        echo "  ./build-pi/poor-house-juno --audio ${SELECTED_AUDIO}"
    else
        echo "  ./build-pi/poor-house-juno"
    fi
    echo ""
    echo -e "${GREEN}Useful Commands:${NC}"
    echo "  List MIDI devices:  amidi -l"
    echo "  List audio devices: aplay -l"
    if [ "$SELECTED_AUDIO" != "default" ]; then
        echo "  Test audio:         speaker-test -D ${SELECTED_AUDIO} -c 2"
    fi
    echo "  Adjust volume:      alsamixer"
    echo "  Edit config:        nano ${CONFIG_FILE}"
    echo ""
    echo -e "${GREEN}Documentation:${NC}"
    echo "  Quick start:  ${INSTALL_DIR}/DEPLOY.md"
    echo "  Full guide:   ${INSTALL_DIR}/README.md"
    echo ""

    # Optional: Auto-start setup
    echo -e "${YELLOW}Optional:${NC} Set up auto-start on boot?"

    # Robust prompt: accept y/yes/n/no and keep asking until answered.
    # Tries /dev/tty first (works when piped), falls back to stdin if needed.
    # If neither is available, we skip but tell the user explicitly.
    while true; do
        if [ -r /dev/tty ]; then
            read_target="/dev/tty"
        else
            read_target="/dev/stdin"
        fi

        if ! read -r -p "Configure Poor House Juno to start automatically? (y/n) " REPLY < "$read_target"; then
            echo "No interactive input available; skipping auto-start setup."
            break
        fi

        case "$REPLY" in
            [Yy]|[Yy][Ee][Ss])
                setup_autostart "$SELECTED_AUDIO"
                break
                ;;
            [Nn]|[Nn][Oo])
                break
                ;;
            *)
                echo "Please answer y or n."
                ;;
        esac
    done

    echo ""
    print_header "Ready to Rock! 🎹"
}

# Setup systemd service for auto-start
setup_autostart() {
    local audio_device="$1"
    print_info "Setting up auto-start..."

    # Prompt for MIDI device
    echo ""
    echo "Available MIDI devices:"
    # Avoid noisy stderr when no devices/permissions; still give a helpful hint.
    if ! amidi -l 2>/dev/null; then
        echo "  (Could not list MIDI devices. If one is connected, ensure you have raw MIDI permissions, typically by being in the 'audio' group.)"
    fi
    echo ""
    read -r -p "Enter MIDI device (e.g., hw:1,0,0) or press Enter for auto-detect: " MIDI_DEV
    MIDI_DEV=${MIDI_DEV:-hw:1,0,0}

    # Update config file with MIDI device if specified
    if [ -n "$MIDI_DEV" ] && [ "$MIDI_DEV" != "hw:1,0,0" ]; then
        sed -i "s|^MIDI_DEVICE=.*|MIDI_DEVICE=${MIDI_DEV}|" "${CONFIG_FILE}"
    fi

    # Build the command line
    local exec_cmd="${INSTALL_DIR}/build-pi/poor-house-juno"
    if [ "$audio_device" != "default" ]; then
        exec_cmd="${exec_cmd} --audio ${audio_device}"
    fi
    exec_cmd="${exec_cmd} --midi ${MIDI_DEV}"

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
ExecStart=${exec_cmd}
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

    sudo mv "$SERVICE_FILE" /etc/systemd/system/poor-house-juno.service
    if [ $? -ne 0 ]; then
        print_error "Failed to install systemd service file"
        return 1
    fi

    sudo systemctl daemon-reload
    if [ $? -ne 0 ]; then
        print_error "Failed to reload systemd daemon"
        return 1
    fi

    sudo systemctl enable poor-house-juno
    if [ $? -ne 0 ]; then
        print_error "Failed to enable service"
        return 1
    fi

    sudo systemctl start poor-house-juno
    if [ $? -ne 0 ]; then
        print_error "Failed to start service"
        echo "Check logs with: journalctl -u poor-house-juno -n 50"
        return 1
    fi

    # Verify service is running
    sleep 2
    if sudo systemctl is-active --quiet poor-house-juno; then
        print_success "Auto-start configured and service is running"
    else
        print_error "Service enabled but not running. Check status with: sudo systemctl status poor-house-juno"
        return 1
    fi

    echo ""
    echo "Service commands:"
    echo "  Status:  sudo systemctl status poor-house-juno"
    echo "  Stop:    sudo systemctl stop poor-house-juno"
    echo "  Restart: sudo systemctl restart poor-house-juno"
    echo "  Disable: sudo systemctl disable poor-house-juno"
    echo "  Logs:    journalctl -u poor-house-juno -f"
}

# Run main installation
main "$@"
