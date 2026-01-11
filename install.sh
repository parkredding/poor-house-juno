#!/bin/bash
# Poor House Juno - One-Line Installer for Raspberry Pi
# Usage: curl -sSL https://raw.githubusercontent.com/parkredding/poor-house-juno/main/install.sh | bash

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
INSTALL_DIR="${HOME}/poor-house-juno"
REPO_URL="https://github.com/parkredding/poor-house-juno.git"
BRANCH="${BRANCH:-main}"
TEMP_AUDIO_LIST="/tmp/phj_audio_list.txt"

# --- Helper Functions ---

print_header() { echo -e "${BLUE}=========================================${NC}\n${BLUE}$1${NC}\n${BLUE}=========================================${NC}"; }
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_info() { echo -e "${YELLOW}ℹ${NC} $1"; }
print_step() { echo -e "\n${BLUE}[Step $1/$2]${NC} $3"; }

# Robust Input Function (Safe for curl | bash)
get_input() {
    local prompt="$1"
    local default="$2"
    local var_name="$3"
    local input_src="/dev/stdin"
    
    if [ -c /dev/tty ]; then
        input_src="/dev/tty"
    fi

    if [ ! -t 0 ] && [ "$input_src" = "/dev/stdin" ]; then
        print_info "Non-interactive mode detected. Using default: $default"
        eval "$var_name='$default'"
        return
    fi

    local prompt_text="$prompt"
    if [ -n "$default" ]; then
        prompt_text="$prompt [$default]: "
    else
        prompt_text="$prompt: "
    fi

    echo -n "$prompt_text" >&2
    read -r response < "$input_src"
    
    if [ -z "$response" ]; then
        eval "$var_name='$default'"
    else
        eval "$var_name='$response'"
    fi
}

check_platform() {
    if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null && ! grep -q "BCM" /proc/cpuinfo 2>/dev/null; then
        print_error "This script is designed for Raspberry Pi"
        get_input "Continue anyway? (y/n)" "n" "CONTINUE_CHOICE"
        if [[ ! $CONTINUE_CHOICE =~ ^[Yy]$ ]]; then exit 1; fi
    fi
}

detect_audio_device() {
    local best_device=""
    local best_score=0

    # Dump aplay output to temp file to avoid pipe/subshell issues
    aplay -l > "$TEMP_AUDIO_LIST" 2>/dev/null || true

    while IFS= read -r line; do
        if [[ $line =~ ^card\ ([0-9]+):\ ([^,]+).*device\ ([0-9]+): ]]; then
            local card="${BASH_REMATCH[1]}"
            local card_name="${BASH_REMATCH[2]}"
            local device="${BASH_REMATCH[3]}"
            local hw_id="hw:${card},${device}"
            
            if [[ "$card_name" == "Loopback" ]]; then continue; fi

            print_info "Testing ${hw_id} (${card_name})..." >&2
            local test_passed=false

            # Redirect stdin to /dev/null for safety
            if timeout 5 speaker-test -D "$hw_id" -c 2 -r 48000 -t sine -l 1 </dev/null >/dev/null 2>&1; then
                test_passed=true
            elif timeout 5 speaker-test -D "$hw_id" -c 2 -r 44100 -t sine -l 1 </dev/null >/dev/null 2>&1; then
                test_passed=true
            elif [[ $card_name =~ (bcm2835|Headphones|Analog) ]]; then
                print_info "  ${hw_id} detection uncertain, assuming compatible" >&2
                test_passed=true
            fi

            if [ "$test_passed" = true ]; then
                local score=1
                local description="${card_name}"
                if [[ $card_name =~ (USB|Audio|DAC) ]]; then score=100; description="USB Audio";
                elif [[ $card_name =~ (Headphones|bcm2835|Analog) ]]; then score=50; description="Headphones/Analog";
                elif [[ $card_name =~ (HDMI|hdmi|vc4) ]]; then score=10; description="HDMI"; fi

                print_success "  ${hw_id} works (${description})" >&2
                if [ $score -gt $best_score ]; then
                    best_score=$score
                    best_device=$hw_id
                fi
            else
                print_info "  ${hw_id} not compatible" >&2
            fi
        fi
    done < "$TEMP_AUDIO_LIST"
    
    echo "$best_device"
}

main() {
    print_header "Poor House Juno - Raspberry Pi Installer"
    echo "Installation directory: ${INSTALL_DIR}"
    
    check_platform

    # Step 1: Update & Dep
    print_step 1 6 "Updating dependencies"
    sudo apt-get update -qq < /dev/null
    sudo apt-get install -y -qq build-essential cmake git libasound2-dev pkg-config alsa-utils < /dev/null 2>&1 | grep -v "Processing\|Preparing" || true

    # Step 2: Clone/Update
    print_step 2 6 "Fetching source"
    if [ -d "$INSTALL_DIR" ]; then
        cd "$INSTALL_DIR"
        git fetch origin < /dev/null
        git checkout "$BRANCH" < /dev/null
        git pull origin "$BRANCH" < /dev/null
        print_success "Repository updated"
    else
        git clone -b "$BRANCH" "$REPO_URL" "$INSTALL_DIR" < /dev/null
        cd "$INSTALL_DIR"
        print_success "Repository cloned"
    fi

    # Step 3: Build
    print_step 3 6 "Building Poor House Juno"
    print_info "This may take 5-10 minutes..."
    
    mkdir -p build-pi
    cd build-pi
    cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release < /dev/null
    make -j$(nproc) < /dev/null

    if [ ! -f "${INSTALL_DIR}/build-pi/poor-house-juno" ]; then
        print_error "Build failed"
        exit 1
    fi
    print_success "Build completed"

    # Step 4: Audio Selection
    print_step 4 6 "Audio Configuration"
    
    # 1. Generate temp file safely
    aplay -l > "$TEMP_AUDIO_LIST" 2>/dev/null || true
    
    declare -a AUDIO_DEVICES
    declare -a AUDIO_NAMES
    count=0

    # 2. Read from temp file (Safe, no process substitution)
    while IFS= read -r line; do
        if [[ $line =~ ^card\ ([0-9]+):\ ([^,]+).*device\ ([0-9]+): ]]; then
            AUDIO_DEVICES[$count]="hw:${BASH_REMATCH[1]},${BASH_REMATCH[3]}"
            AUDIO_NAMES[$count]="${BASH_REMATCH[2]}"
            ((count++))
        fi
    done < "$TEMP_AUDIO_LIST"

    # 3. Detect recommended
    RECOMMENDED_AUDIO=$(detect_audio_device)
    [ -z "$RECOMMENDED_AUDIO" ] && RECOMMENDED_AUDIO="default"

    echo -e "\nAvailable devices:"
    for i in "${!AUDIO_DEVICES[@]}"; do
        marker=""
        [ "${AUDIO_DEVICES[$i]}" = "$RECOMMENDED_AUDIO" ] && marker=" [recommended]"
        echo "  $((i+1)). ${AUDIO_DEVICES[$i]} - ${AUDIO_NAMES[$i]}${marker}"
    done
    echo "  $((count+1)). default (ALSA default)"

    # 4. Clean up
    rm -f "$TEMP_AUDIO_LIST"

    get_input "Select audio device (1-$((count+1)))" "" "AUDIO_CHOICE"

    SELECTED_AUDIO="$RECOMMENDED_AUDIO"
    SELECTED_AUDIO_NAME="Auto-detected"

    if [[ "$AUDIO_CHOICE" =~ ^[0-9]+$ ]]; then
        if [ "$AUDIO_CHOICE" -le "$count" ] && [ "$AUDIO_CHOICE" -gt 0 ]; then
            idx=$((AUDIO_CHOICE-1))
            SELECTED_AUDIO="${AUDIO_DEVICES[$idx]}"
            SELECTED_AUDIO_NAME="${AUDIO_NAMES[$idx]}"
        elif [ "$AUDIO_CHOICE" -eq $((count+1)) ]; then
            SELECTED_AUDIO="default"
            SELECTED_AUDIO_NAME="ALSA Default"
        fi
    fi

    # Step 5: Save Config
    print_step 5 6 "Saving Configuration"
    CONFIG_DIR="${HOME}/.config/poor-house-juno"
    mkdir -p "$CONFIG_DIR"
    cat > "${CONFIG_DIR}/config" << EOF
AUDIO_DEVICE=${SELECTED_AUDIO}
AUDIO_DEVICE_NAME=${SELECTED_AUDIO_NAME}
MIDI_DEVICE=
EOF
    print_success "Saved to ${CONFIG_DIR}/config"

    # Step 6: Autostart
    print_step 6 6 "Service Setup"
    get_input "Configure auto-start on boot? (y/n)" "n" "AUTOSTART"
    if [[ "$AUTOSTART" =~ ^[Yy] ]]; then
        setup_autostart "$SELECTED_AUDIO"
    fi

    echo -e "\n${GREEN}Installation Complete!${NC}"
    echo "Run manually: ${INSTALL_DIR}/build-pi/poor-house-juno --audio ${SELECTED_AUDIO}"
}

setup_autostart() {
    local audio_device="$1"
    
    echo -e "\nAvailable MIDI Devices:"
    amidi -l 2>/dev/null || echo "  (None found or permission denied)"
    
    get_input "Enter MIDI device (or press Enter for auto)" "hw:1,0,0" "MIDI_DEV"
    
    sed -i "s|^MIDI_DEVICE=.*|MIDI_DEVICE=${MIDI_DEV}|" "${HOME}/.config/poor-house-juno/config"

    local exec_cmd="${INSTALL_DIR}/build-pi/poor-house-juno"
    [ "$audio_device" != "default" ] && exec_cmd="${exec_cmd} --audio ${audio_device}"
    exec_cmd="${exec_cmd} --midi ${MIDI_DEV}"

    # Use tee to write file with sudo permissions
    cat << EOF | sudo tee /etc/systemd/system/poor-house-juno.service > /dev/null
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

    sudo systemctl daemon-reload
    sudo systemctl enable poor-house-juno
    sudo systemctl start poor-house-juno
    
    if systemctl is-active --quiet poor-house-juno; then
        print_success "Service is running!"
    else
        print_error "Service failed to start. Check: journalctl -u poor-house-juno"
    fi
}

main "$@"
