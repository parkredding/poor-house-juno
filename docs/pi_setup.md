# Raspberry Pi Setup Guide

**Poor House Juno - Complete Setup and Configuration for Raspberry Pi 4**

---

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Operating System Setup](#operating-system-setup)
3. [Installing Dependencies](#installing-dependencies)
4. [ALSA Audio Configuration](#alsa-audio-configuration)
5. [MIDI Configuration](#midi-configuration)
6. [Building Poor House Juno](#building-poor-house-juno)
7. [Running the Synthesizer](#running-the-synthesizer)
8. [Performance Tuning](#performance-tuning)
9. [Systemd Service Setup](#systemd-service-setup)
10. [Troubleshooting](#troubleshooting)

---

## Hardware Requirements

### Minimum Requirements

| Component | Specification |
|-----------|--------------|
| **Model** | Raspberry Pi 4 Model B |
| **RAM** | 2 GB (4 GB recommended) |
| **Storage** | 8 GB microSD (16 GB+ recommended) |
| **Audio Interface** | USB audio interface or Pi HAT |
| **MIDI Interface** | USB MIDI interface or DIN-to-USB adapter |
| **Power Supply** | Official 5V/3A USB-C power supply |

### Recommended Hardware

**Audio Interfaces:**
- Behringer UCA202 (budget, stereo)
- Focusrite Scarlett 2i2 (pro quality)
- HifiBerry DAC+ (HAT, no USB)
- AudioInjector Stereo (HAT, low latency)

**MIDI Interfaces:**
- Generic USB-MIDI cable (most common)
- M-Audio Uno (reliable)
- Roland UM-ONE (high quality)
- Kenton USB MIDI Host (converts USB MIDI to DIN)

**Optional:**
- MIDI keyboard (25-61 keys recommended)
- Audio monitor (speakers or headphones)
- Enclosure/case with fan (for cooling)

---

## Operating System Setup

### Raspberry Pi OS Installation

**1. Download Raspberry Pi OS:**
```bash
# Recommended: Raspberry Pi OS Lite (64-bit)
# No desktop environment needed for headless synth
```

Download from: https://www.raspberrypi.com/software/operating-systems/

**2. Flash to microSD:**
```bash
# Using Raspberry Pi Imager (recommended)
# Or dd command:
sudo dd if=raspios-lite-arm64.img of=/dev/sdX bs=4M status=progress
sync
```

**3. Enable SSH (headless setup):**
```bash
# Mount boot partition
cd /media/boot

# Create empty ssh file
touch ssh

# Optional: Configure WiFi
nano wpa_supplicant.conf
```

**wpa_supplicant.conf:**
```
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="YourNetworkName"
    psk="YourPassword"
}
```

**4. Boot Raspberry Pi:**
- Insert microSD card
- Connect power
- Wait ~30 seconds for boot
- Find IP address (check router or use `nmap`)

**5. SSH into Pi:**
```bash
ssh pi@raspberrypi.local
# Default password: raspberry
```

**6. Initial Configuration:**
```bash
sudo raspi-config

# Configure:
# - Change password (essential!)
# - Expand filesystem (use full SD card)
# - Set locale/timezone
# - Optional: Enable I2C/SPI if using HATs
```

**7. Update System:**
```bash
sudo apt update
sudo apt upgrade -y
sudo reboot
```

---

## Installing Dependencies

### Automated Setup

**Run setup script:**
```bash
git clone https://github.com/parkredding/poor-house-juno.git
cd poor-house-juno
chmod +x scripts/setup_pi.sh
./scripts/setup_pi.sh
```

### Manual Installation

**Build Tools:**
```bash
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config
```

**ALSA Development Libraries:**
```bash
sudo apt install -y \
    libasound2-dev \
    alsa-utils
```

**Optional: JACK Audio (for pro users):**
```bash
sudo apt install -y \
    jackd2 \
    qjackctl \
    libjack-jackd2-dev
```

**Verify Installation:**
```bash
cmake --version     # Should be 3.20+
alsa-info --version
aplay --version
```

---

## ALSA Audio Configuration

### List Audio Devices

**Check available playback devices:**
```bash
aplay -l
```

**Example output:**
```
**** List of PLAYBACK Hardware Devices ****
card 0: Headphones [bcm2835 Headphones], device 0: bcm2835 Headphones [bcm2835 Headphones]
  Subdevices: 8/8
card 1: Device [USB Audio Device], device 0: USB Audio [USB Audio]
  Subdevices: 1/1
```

**Card 0:** Built-in 3.5mm headphone jack (basic quality, works for testing)
**Card 1:** USB audio interface (recommended for best quality)

### Using the Built-in 3.5mm Audio Jack

**For Raspberry Pi's internal 3.5mm headphone jack:**

The built-in audio jack (card 0) provides basic stereo output suitable for practice and small setups.

**Configure for 3.5mm output:**
```bash
# Test the built-in jack
speaker-test -D hw:0,0 -c 2 -r 48000

# Run Poor House Juno with 3.5mm output
./build-pi/poor-house-juno --audio hw:0,0
```

**Set as default audio device:**
Edit `~/.asoundrc`:
```bash
pcm.!default {
    type hw
    card 0
    device 0
}

ctl.!default {
    type hw
    card 0
}
```

**Volume Control:**
```bash
# Adjust headphone volume
alsamixer
# Use arrow keys to adjust PCM volume
# Press F6 to select sound card if multiple cards
```

**Notes:**
- The 3.5mm jack provides lower quality than USB audio interfaces
- Suitable for headphones, small speakers, or line-level input to mixers
- Works perfectly for live performance and jamming

### Test Audio Output

**Test with speaker-test:**
```bash
speaker-test -D hw:1,0 -c 2 -r 48000
```

**Test with aplay:**
```bash
aplay -D hw:1,0 test.wav
```

### Set Default Device

**Edit `~/.asoundrc`:**
```bash
nano ~/.asoundrc
```

**Contents:**
```
pcm.!default {
    type hw
    card 1
    device 0
}

ctl.!default {
    type hw
    card 1
}
```

**Or system-wide (`/etc/asound.conf`):**
```bash
sudo nano /etc/asound.conf
# (same contents as above)
```

### Configure Sample Rate

**Check supported sample rates:**
```bash
cat /proc/asound/card1/stream0
```

**Set sample rate (some devices):**
```bash
# Edit /etc/modprobe.d/alsa-base.conf
sudo nano /etc/modprobe.d/alsa-base.conf

# Add:
options snd-usb-audio nrpacks=1
```

### Reduce Audio Latency

**Edit `/etc/security/limits.conf`:**
```bash
sudo nano /etc/security/limits.conf

# Add:
@audio - rtprio 95
@audio - memlock unlimited
```

**Add user to audio group:**
```bash
sudo usermod -a -G audio $USER
# Log out and back in
```

---

## MIDI Configuration

### List MIDI Devices

**Check available MIDI devices:**
```bash
amidi -l
```

**Example output:**
```
Dir Device    Name
IO  hw:1,0,0  USB MIDI MIDI 1
```

### Test MIDI Input

**Monitor MIDI messages:**
```bash
amidi -p hw:1,0,0 -d
# Play notes on MIDI keyboard - should see hex data
```

**Or using `aseqdump`:**
```bash
aseqdump -p 20:0
# Replace 20:0 with your device port
```

### MIDI Permissions

**Add user to dialout group:**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

**Or create udev rule:**
```bash
sudo nano /etc/udev/rules.d/99-midi.rules

# Add:
SUBSYSTEM=="sound", GROUP="audio", MODE="0660"
SUBSYSTEM=="usb", ATTRS{idVendor}=="xxxx", ATTRS{idProduct}=="yyyy", GROUP="audio"
# Replace xxxx:yyyy with your device's USB IDs (from lsusb)

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Expose the Pi as a USB MIDI device (OTG to computer)

This lets your DAW (e.g., Ableton) see the Pi as a USB MIDI output port and send notes/CC back to the synth without a separate MIDI interface.

**Requirements**
- Raspberry Pi 4 or Zero 2 W (USB-C/OTG capable)
- USB-C cable from Pi to your computer (data-capable, not charge-only)
- Raspberry Pi OS Bookworm/Bullseye (paths differ slightly below)

**1) Enable USB gadget mode**
```bash
# Backup boot config (Bookworm uses /boot/firmware; Bullseye uses /boot)
sudo cp /boot/firmware/config.txt /boot/firmware/config.txt.bak 2>/dev/null || true
sudo cp /boot/config.txt /boot/config.txt.bak 2>/dev/null || true

# Add the dwc2 overlay in config.txt (Bookworm: /boot/firmware/config.txt)
sudo sed -i '/^dtoverlay=dwc2/!s|^\\(\\[all\\]\\)|\\1\\ndtoverlay=dwc2,dr_mode=peripheral|' /boot/firmware/config.txt 2>/dev/null || \
sudo sed -i '/^dtoverlay=dwc2/!s|^\\(\\[all\\]\\)|\\1\\ndtoverlay=dwc2,dr_mode=peripheral|' /boot/config.txt
```

**2) Load the USB MIDI gadget at boot**
Edit the kernel cmdline and append gadget modules after `rootwait`:
```bash
sudo sed -i 's/rootwait/rootwait modules-load=dwc2,g_midi g_midi.id=PoorHouseJuno g_midi.iManufacturer=PoorHouse g_midi.iProduct=Juno/' /boot/firmware/cmdline.txt 2>/dev/null || \
sudo sed -i 's/rootwait/rootwait modules-load=dwc2,g_midi g_midi.id=PoorHouseJuno g_midi.iManufacturer=PoorHouse g_midi.iProduct=Juno/' /boot/cmdline.txt
```

**3) Reboot**
```bash
sudo reboot
```

**4) Verify on the Pi**
After reboot, the Pi exposes a USB MIDI device to the host and also shows up in ALSA:
```bash
amidi -l
# You should see something like:
# IO  hw:2,0,0  PoorHouseJuno MIDI 1
```

**5) Use it in the synth**
Run Poor House Juno and point MIDI to the gadget device:
```bash
./build-pi/poor-house-juno --audio hw:0,0 --midi hw:2,0,0
```
Adjust the card/device numbers if they differ (`amidi -l` shows the exact hw:x,y,z).

**6) Wire it in your DAW**
- Plug the Pi’s USB-C into your computer.
- In Ableton (or any DAW), select the new MIDI output port named `PoorHouseJuno` (or `Linux USB MIDI Gadget` if the name didn’t stick).
- Send notes/CC to that port; they arrive at the Pi synth on `--midi hw:2,0,0`.

**Notes**
- Keep the Pi powered via the same USB-C port; avoid unpowered hubs.
- If you later need to undo this, restore the backups and remove the `modules-load=dwc2,g_midi` entry.

---

## Building Poor House Juno

### Clone Repository

```bash
cd ~
git clone https://github.com/parkredding/poor-house-juno.git
cd poor-house-juno
```

### Build Native Binary

**Using Makefile:**
```bash
make pi
```

**Or manual CMake:**
```bash
mkdir build-pi
cd build-pi
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
make -j4  # Use all 4 cores
```

**Verify build:**
```bash
ls -lh build-pi/poor-house-juno
# Should see ~500 KB executable
```

### Build Time

- **Debug build:** ~2-3 minutes
- **Release build:** ~4-5 minutes (with optimization)

---

## Running the Synthesizer

### Basic Usage

**Default devices (auto-detect):**
```bash
./build-pi/poor-house-juno
```

**Specify audio device:**
```bash
./build-pi/poor-house-juno --audio hw:1,0
```

**Specify MIDI device:**
```bash
./build-pi/poor-house-juno --midi hw:1,0,0
```

**Both:**
```bash
./build-pi/poor-house-juno --audio hw:1,0 --midi hw:1,0,0
```

### Runtime Controls

**While running:**
- Press MIDI keys to play notes
- Ctrl+C to exit gracefully

**Output:**
```
Poor House Juno - Raspberry Pi
Sample Rate: 48000 Hz
Buffer Size: 128 samples
Audio Device: hw:1,0
MIDI Device: hw:1,0,0

Starting audio thread...
Starting MIDI thread...
Synth running. Press Ctrl+C to exit.

[CPU: 35%] Active voices: 3/6
[CPU: 42%] Active voices: 6/6
```

### Performance Monitoring

**CPU usage displayed every 5 seconds:**
- Target: < 50%
- Typical: 30-40% with 6 voices + chorus
- Warning: > 60% may cause audio dropouts

**Check system load:**
```bash
htop
# Look for 'poor-house-juno' process
# Should use ~35-40% of one core
```

---

## Performance Tuning

### CPU Governor

**Set to performance mode:**
```bash
# Check current governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set to performance (persistent until reboot)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Make permanent (edit /etc/rc.local)
sudo nano /etc/rc.local
# Add before 'exit 0':
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Disable WiFi/Bluetooth (optional)

**For lowest latency, disable unused services:**
```bash
# Disable WiFi
sudo rfkill block wifi

# Disable Bluetooth
sudo rfkill block bluetooth

# Make permanent
sudo nano /boot/config.txt
# Add:
dtoverlay=disable-wifi
dtoverlay=disable-bt
```

### Real-Time Priority

**Audio thread priority:**
- Poor House Juno sets `SCHED_FIFO` priority 80 automatically
- Requires audio group membership (see ALSA section)

**Verify:**
```bash
ps -eLo pid,cls,rtprio,comm | grep poor-house
# Should see CLS=FF (FIFO) and RTPRIO=80
```

### Disable Desktop Environment

**If running Raspberry Pi OS Desktop:**
```bash
# Switch to console
sudo systemctl set-default multi-user.target
sudo reboot

# Switch back to desktop (if needed)
sudo systemctl set-default graphical.target
sudo reboot
```

### Cooling and Throttling

**Monitor temperature:**
```bash
vcgencmd measure_temp
```

**Avoid throttling:**
- Keep Pi below 80°C
- Use heatsinks or fan
- Ensure good airflow

**Check for throttling:**
```bash
vcgencmd get_throttled
# 0x0 = no throttling (good)
# Other values = throttled (add cooling!)
```

---

## Systemd Service Setup

### Create Service File

**Create `/etc/systemd/system/poor-house-juno.service`:**
```bash
sudo nano /etc/systemd/system/poor-house-juno.service
```

**Contents:**
```ini
[Unit]
Description=Poor House Juno Synthesizer
After=sound.target network.target
Wants=sound.target

[Service]
Type=simple
User=pi
Group=audio
WorkingDirectory=/home/pi/poor-house-juno
ExecStart=/home/pi/poor-house-juno/build-pi/poor-house-juno --audio hw:0,0 --midi hw:1,0,0
Restart=on-failure
RestartSec=5s

# Real-time priority
LimitRTPRIO=95
LimitMEMLOCK=infinity

# Logging
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### Enable and Start Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service (start on boot)
sudo systemctl enable poor-house-juno

# Start service now
sudo systemctl start poor-house-juno

# Check status
sudo systemctl status poor-house-juno

# View logs
journalctl -u poor-house-juno -f
```

### Service Management

```bash
# Stop service
sudo systemctl stop poor-house-juno

# Restart service
sudo systemctl restart poor-house-juno

# Disable service (don't start on boot)
sudo systemctl disable poor-house-juno
```

---

## Troubleshooting

### Audio Issues

**Problem: No audio output**

**Solutions:**
```bash
# Check ALSA devices
aplay -l

# Test speaker-test
speaker-test -D hw:1,0 -c 2

# Check volume (might be muted)
alsamixer

# Verify device name
cat /proc/asound/cards
```

**Problem: Audio dropouts / glitches**

**Solutions:**
```bash
# Increase buffer size (trade-off: higher latency)
# Edit src/platform/pi/audio_driver.cpp
# Change BUFFER_SIZE from 128 to 256

# Check CPU usage
htop

# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Check for throttling
vcgencmd get_throttled
```

**Problem: High latency (delayed response)**

**Solutions:**
```bash
# Reduce buffer size (requires more CPU)
# Default: 128 samples = 2.67 ms latency

# Check real-time priority
ps -eLo pid,cls,rtprio,comm | grep poor-house

# Disable unnecessary services
sudo systemctl disable bluetooth
sudo systemctl disable cups
```

### MIDI Issues

**Problem: No MIDI input detected**

**Solutions:**
```bash
# List MIDI devices
amidi -l

# Test MIDI input
amidi -p hw:1,0,0 -d
# Play keys - should see hex data

# Check permissions
groups
# Should include 'audio' and 'dialout'

# Add to groups if missing
sudo usermod -a -G audio,dialout $USER
# Log out and back in
```

**Problem: MIDI messages received but no sound**

**Solutions:**
```bash
# Check audio output (see Audio Issues above)

# Check synth is receiving MIDI
# (debug output should show note-on messages)

# Try test chord (automatic if no MIDI)
# Should hear A-C-E chord on startup if no MIDI device
```

### Build Issues

**Problem: CMake not found**

```bash
sudo apt install cmake
cmake --version  # Verify 3.20+
```

**Problem: ALSA headers not found**

```bash
sudo apt install libasound2-dev
```

**Problem: Linking errors**

```bash
# Ensure all dependencies installed
./scripts/setup_pi.sh

# Clean and rebuild
rm -rf build-pi
make pi
```

### Performance Issues

**Problem: CPU usage > 50%**

**Solutions:**
- Set CPU governor to performance
- Disable desktop environment
- Close unnecessary processes
- Consider overclocking (advanced)

**Overclocking (not recommended for beginners):**
```bash
sudo nano /boot/config.txt

# Add:
over_voltage=4
arm_freq=1750

# Requires good cooling!
# Monitor temperature closely
```

**Problem: Temperature throttling**

```bash
# Check temperature
vcgencmd measure_temp

# Check throttling status
vcgencmd get_throttled

# Solutions:
# - Add heatsinks
# - Add fan
# - Improve case ventilation
```

---

## Advanced Configuration

### Using JACK Audio

**Install JACK:**
```bash
sudo apt install jackd2 qjackctl
```

**Start JACK server:**
```bash
jackd -d alsa -d hw:1,0 -r 48000 -p 128 -n 2
```

**Note:** Poor House Juno currently uses ALSA directly. JACK integration is future work.

### USB Audio Quirks

**Some USB audio interfaces need special configuration:**

```bash
# Edit /etc/modprobe.d/alsa-base.conf
sudo nano /etc/modprobe.d/alsa-base.conf

# Add:
options snd-usb-audio nrpacks=1
options snd-usb-audio pid=0x1234 vid=0x5678 device_setup=1
# Replace with your device's PID/VID from lsusb
```

### Headless Operation

**SSH daemon should start automatically. If not:**
```bash
sudo systemctl enable ssh
sudo systemctl start ssh
```

**Or use serial console (no network needed):**
```bash
# Enable in raspi-config
sudo raspi-config
# Interface Options > Serial Port > Enable
```

---

## Performance Benchmarks

### Raspberry Pi 4 (4 GB)

| Test Scenario | CPU Usage | Latency |
|---------------|-----------|---------|
| Idle (no voices) | 5-8% | 2.67 ms |
| 1 voice + chorus | 15-20% | 2.67 ms |
| 3 voices + chorus | 25-30% | 2.67 ms |
| 6 voices + chorus | 35-42% | 2.67 ms |
| 6 voices + max modulation | 45-50% | 2.67 ms |

**Buffer Size:** 128 samples @ 48 kHz
**Compiler:** GCC 10.2.1 with -O3 optimization
**Governor:** performance

### Comparison: Pi 3 vs Pi 4

| Model | CPU Usage (6 voices) | Max Polyphony |
|-------|---------------------|---------------|
| Pi 3B+ | 70-80% | 5-6 voices |
| Pi 4 (2GB) | 40-50% | 6+ voices |
| Pi 4 (4GB) | 35-45% | 6+ voices |

**Recommendation:** Raspberry Pi 4 with at least 2 GB RAM.

---

## References

- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/)
- [ALSA Project](https://www.alsa-project.org/)
- [Real-Time Linux](https://rt.wiki.kernel.org/)
- [Raspberry Pi Audio Guide](https://learn.adafruit.com/usb-audio-cards-with-a-raspberry-pi)

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
