# Poor House Juno - Raspberry Pi Setup Guide

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Initial Raspberry Pi Setup](#initial-raspberry-pi-setup)
4. [ALSA Configuration](#alsa-configuration)
5. [MIDI Device Setup](#midi-device-setup)
6. [Real-Time Priority Configuration](#real-time-priority-configuration)
7. [Performance Tuning](#performance-tuning)
8. [Building Poor House Juno](#building-poor-house-juno)
9. [Running the Synthesizer](#running-the-synthesizer)
10. [Troubleshooting](#troubleshooting)

---

## Overview

Poor House Juno runs on Raspberry Pi 4 as a standalone hardware synthesizer. This guide covers the complete setup process, from initial Pi configuration to running the synth with low-latency audio.

**Target Performance:**
- CPU Usage: <50% (6 voices + chorus)
- Latency: ~2.7 ms (128-sample buffer @ 48 kHz)
- Sample Rate: 48 kHz
- Buffer Size: 128 samples

---

## Hardware Requirements

### Minimum Requirements

**Raspberry Pi:**
- **Model:** Raspberry Pi 4 (4GB or 8GB RAM recommended)
- **Reason:** Sufficient CPU for real-time audio processing
- **Alternatives:** Pi 4 2GB may work but not recommended

**Audio Interface:**
- **Type:** USB audio interface (class-compliant)
- **Sample Rates:** Must support 48 kHz
- **Latency:** Low-latency ASIO/ALSA drivers
- **Recommended:** Behringer UCA202, Focusrite Scarlett Solo, or similar

**MIDI Controller:**
- **Type:** USB MIDI controller or MIDI-to-USB cable
- **Class-Compliant:** Must work with Linux ALSA
- **Examples:** Akai MPK Mini, M-Audio Oxygen, Roland A-49

**Power Supply:**
- **Official Raspberry Pi 4 Power Supply** (5V, 3A USB-C)
- **Important:** Underpowered supplies cause audio dropouts!

**Storage:**
- **microSD Card:** 16GB minimum (32GB recommended)
- **Speed:** Class 10 or better
- **Alternative:** USB SSD for better performance

### Recommended Accessories

- **Case:** With fan for cooling (audio processing generates heat)
- **HDMI Monitor:** For initial setup (can run headless after)
- **Keyboard/Mouse:** For initial setup

### Tested Hardware

| Component        | Model                  | Status      | Notes                  |
|------------------|------------------------|-------------|------------------------|
| Audio Interface  | Behringer UCA202       | ✅ Tested   | Good budget option     |
| Audio Interface  | Focusrite Scarlett 2i2 | ✅ Tested   | Excellent quality      |
| MIDI Controller  | Akai MPK Mini          | ✅ Tested   | Compact, good keys     |
| MIDI Controller  | M-Audio Oxygen 49      | ✅ Tested   | Full-size keyboard     |
| Power Supply     | Official Pi 4 PSU      | ✅ Required | Non-official may fail  |

---

## Initial Raspberry Pi Setup

### 1. Install Raspberry Pi OS

**Operating System:** Raspberry Pi OS (64-bit) - Bullseye or Bookworm

**Installation Steps:**

1. Download **Raspberry Pi Imager**: https://www.raspberrypi.com/software/

2. Flash microSD card:
   - Insert microSD card into computer
   - Open Raspberry Pi Imager
   - Choose OS: "Raspberry Pi OS (64-bit)"
   - Choose Storage: Your microSD card
   - Click "Write"

3. **Advanced Options** (before writing):
   - Set hostname: `poor-house-juno`
   - Enable SSH
   - Set username/password
   - Configure WiFi (if needed)

4. Insert microSD card into Pi and boot

### 2. Initial Configuration

**Boot and Login:**
```bash
# Default credentials (if not changed during imaging)
Username: pi
Password: raspberry
```

**Update System:**
```bash
sudo apt update
sudo apt upgrade -y
```

**Configure Pi:**
```bash
sudo raspi-config
```

**Recommended Settings:**
- **System Options → Boot:** Console Autologin (headless) or Desktop (if using GUI)
- **Performance Options → GPU Memory:** 16 MB (minimum, we don't need GPU)
- **Localisation Options:** Set timezone, keyboard layout
- **Interface Options:** Enable SSH (for remote access)

**Reboot:**
```bash
sudo reboot
```

### 3. Install Build Dependencies

```bash
# Install required packages
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libasound2-dev \
    pkg-config

# Optional: Install text editor of choice
sudo apt install -y vim nano
```

---

## ALSA Configuration

ALSA (Advanced Linux Sound Architecture) handles audio I/O on Raspberry Pi.

### 1. List Audio Devices

```bash
# List playback devices
aplay -l

# Expected output:
card 0: Headphones [bcm2835 Headphones], device 0: bcm2835 Headphones [bcm2835 Headphones]
card 1: CODEC [USB Audio CODEC], device 0: USB Audio [USB Audio]
```

**Identify Your Audio Interface:**
- `card 0`: Built-in headphone jack (NOT RECOMMENDED - high latency)
- `card 1`: USB audio interface (RECOMMENDED)

### 2. Test Audio Output

```bash
# Test card 1, device 0 (USB audio interface)
speaker-test -D hw:1,0 -c 2 -r 48000

# You should hear pink noise in left and right channels
# Press Ctrl+C to stop
```

### 3. Configure Default Audio Device

**Option A: Temporary (current session only):**
```bash
export ALSA_CARD=1
export ALSA_PCM_CARD=1
```

**Option B: Permanent (recommended):**

Create `/etc/asound.conf`:
```bash
sudo nano /etc/asound.conf
```

Add:
```
defaults.pcm.card 1
defaults.ctl.card 1

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

Save and exit (Ctrl+X, Y, Enter).

### 4. Verify Configuration

```bash
# Check ALSA configuration
aplay -L

# Play test file (if available)
aplay /usr/share/sounds/alsa/Front_Center.wav
```

### 5. Audio Latency Tuning

**Buffer Size Configuration:**

The buffer size affects latency and stability:
- **Smaller buffer:** Lower latency, higher CPU usage, more dropouts
- **Larger buffer:** Higher latency, lower CPU usage, more stable

**Poor House Juno defaults:**
- Buffer size: 1024 samples
- Period size: 128 samples
- Latency: ~2.7 ms @ 48 kHz

**To adjust (in `src/platform/pi/audio_driver.cpp`):**
```cpp
snd_pcm_hw_params_set_buffer_size(handle, params, 1024);  // Total buffer
snd_pcm_hw_params_set_period_size(handle, params, 128, 0); // Processing block
```

---

## MIDI Device Setup

### 1. List MIDI Devices

```bash
# List MIDI input devices
amidi -l

# Expected output:
Dir Device    Name
IO  hw:1,0,0  USB MIDI MIDI 1
```

**Identify Your MIDI Controller:**
- Usually appears as `hw:X,0,0` where X is the card number
- USB MIDI controllers are typically card 1 or higher

### 2. Test MIDI Input

```bash
# Listen to MIDI events (replace X with your card number)
aseqdump -p hw:1,0,0

# Play some keys on your MIDI controller
# You should see:
#   Note on  channel 0, note 60, velocity 127
#   Note off channel 0, note 60, velocity 0
# Press Ctrl+C to stop
```

### 3. MIDI Permissions

Ensure your user has permission to access MIDI devices:

```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Log out and log back in for changes to take effect
```

### 4. MIDI Configuration File

Poor House Juno auto-detects MIDI devices, but you can specify manually:

```bash
# Run with specific MIDI device
./build-pi/poor-house-juno --midi hw:1,0,0
```

---

## Real-Time Priority Configuration

For low-latency audio, the audio thread needs real-time scheduling priority.

### 1. Enable Real-Time Scheduling

**Edit limits configuration:**
```bash
sudo nano /etc/security/limits.conf
```

**Add these lines:**
```
@audio   -  rtprio     95
@audio   -  memlock    unlimited
@audio   -  nice      -19
```

Save and exit.

### 2. Verify Real-Time Permissions

**Logout and login again**, then check:

```bash
# Check rtprio limit
ulimit -r

# Should output: 95
```

### 3. Test Real-Time Priority

Poor House Juno sets real-time priority automatically:

```cpp
// In src/platform/pi/audio_driver.cpp
struct sched_param param;
param.sched_priority = 80;
pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
```

**Priority Levels:**
- **SCHED_FIFO:** Real-time FIFO scheduling
- **Priority 80:** High priority (0-99 range, 99 = highest)

**Verify during runtime:**
```bash
# Run Poor House Juno in one terminal
./build-pi/poor-house-juno

# In another terminal, check thread priority
ps -eLo pid,tid,class,rtprio,comm | grep poor-house
```

Expected output:
```
 1234  1234  TS     -    poor-house-juno
 1234  1235  FF    80    poor-house-juno  (audio thread)
```

---

## Performance Tuning

### 1. CPU Governor

Set CPU to performance mode for consistent audio:

```bash
# Check current governor
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Set to performance (temporary)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Make permanent (add to /etc/rc.local)
sudo nano /etc/rc.local
```

Add before `exit 0`:
```bash
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
```

### 2. Disable Unnecessary Services

Free up CPU cycles:

```bash
# Disable Bluetooth (if not needed)
sudo systemctl disable bluetooth

# Disable WiFi (if using Ethernet)
sudo systemctl disable wpa_supplicant

# Disable GUI (if running headless)
sudo systemctl set-default multi-user.target
```

### 3. Overclocking (Optional, Advanced)

**WARNING:** Overclocking may void warranty and cause instability!

```bash
sudo nano /boot/config.txt
```

Add:
```
# Mild overclock (safe)
over_voltage=2
arm_freq=1750

# GPU (we don't use it, keep low)
gpu_freq=250
```

Reboot and monitor temperature:
```bash
vcgencmd measure_temp
```

**Keep temperature below 80°C under load!**

### 4. Cooling

**Recommended:**
- Active cooling (fan) for sustained performance
- Heatsinks on CPU and RAM chips
- Good airflow around Pi

**Monitor temperature:**
```bash
watch -n 1 vcgencmd measure_temp
```

---

## Building Poor House Juno

### 1. Clone Repository

```bash
cd ~
git clone https://github.com/parkredding/poor-house-juno.git
cd poor-house-juno
```

### 2. Build for Raspberry Pi

```bash
# Create build directory
mkdir build-pi
cd build-pi

# Configure with CMake
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release

# Build (use all CPU cores)
make -j$(nproc)
```

**Build time:** 2-5 minutes on Pi 4

**Expected output:**
```
[ 10%] Building CXX object CMakeFiles/poor-house-juno.dir/src/dsp/dco.cpp.o
[ 20%] Building CXX object CMakeFiles/poor-house-juno.dir/src/dsp/filter.cpp.o
...
[100%] Linking CXX executable poor-house-juno
[100%] Built target poor-house-juno
```

### 3. Verify Build

```bash
ls -lh poor-house-juno

# Expected: Executable ~200-500 KB
```

---

## Running the Synthesizer

### 1. Basic Usage

```bash
cd ~/poor-house-juno/build-pi
./poor-house-juno
```

**Expected output:**
```
Poor House Juno - Raspberry Pi
Sample Rate: 48000 Hz
Buffer Size: 128 samples
Voices: 6

Opening audio device: hw:1,0
Opening MIDI device: hw:1,0,0

Audio thread started with real-time priority 80
MIDI thread started

Ready! Play some keys...
```

### 2. Command-Line Options

```bash
# Specify audio device
./poor-house-juno --audio hw:1,0

# Specify MIDI device
./poor-house-juno --midi hw:1,0,0

# Both
./poor-house-juno --audio hw:1,0 --midi hw:1,0,0

# Help
./poor-house-juno --help
```

### 3. Auto-Start on Boot (Optional)

**Create systemd service:**

```bash
sudo nano /etc/systemd/system/poor-house-juno.service
```

**Add:**
```ini
[Unit]
Description=Poor House Juno Synthesizer
After=sound.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/poor-house-juno/build-pi
ExecStart=/home/pi/poor-house-juno/build-pi/poor-house-juno
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

**Enable and start:**
```bash
sudo systemctl daemon-reload
sudo systemctl enable poor-house-juno
sudo systemctl start poor-house-juno

# Check status
sudo systemctl status poor-house-juno
```

---

## Troubleshooting

### Problem: No Audio Output

**Symptoms:** Synth runs but no sound

**Solutions:**

1. **Check audio device:**
   ```bash
   aplay -l
   ```
   Ensure your USB audio interface is detected.

2. **Test audio output:**
   ```bash
   speaker-test -D hw:1,0 -c 2
   ```

3. **Check volume:**
   ```bash
   alsamixer
   # Press F6 to select sound card
   # Adjust volume with arrow keys
   # Unmute with 'M' key
   ```

4. **Verify device in Poor House Juno:**
   ```bash
   ./poor-house-juno --audio hw:1,0
   ```

### Problem: MIDI Not Working

**Symptoms:** Audio works but MIDI input has no effect

**Solutions:**

1. **List MIDI devices:**
   ```bash
   amidi -l
   ```

2. **Test MIDI input:**
   ```bash
   aseqdump -p hw:1,0,0
   ```
   Play some keys - you should see events.

3. **Check permissions:**
   ```bash
   groups
   ```
   Should include `audio` group. If not:
   ```bash
   sudo usermod -a -G audio $USER
   # Logout and login
   ```

4. **Specify MIDI device:**
   ```bash
   ./poor-house-juno --midi hw:1,0,0
   ```

### Problem: Audio Dropouts/Glitches

**Symptoms:** Crackling, pops, or silence during playback

**Solutions:**

1. **Increase buffer size** (in `src/platform/pi/audio_driver.cpp`):
   ```cpp
   snd_pcm_hw_params_set_buffer_size(handle, params, 2048);  // Was 1024
   snd_pcm_hw_params_set_period_size(handle, params, 256, 0); // Was 128
   ```

2. **Check CPU usage:**
   ```bash
   top
   ```
   If CPU > 80%, reduce load or disable services.

3. **Set CPU governor to performance:**
   ```bash
   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   ```

4. **Verify real-time priority:**
   ```bash
   ulimit -r  # Should be 95
   ```

5. **Check USB power:**
   - Use official Raspberry Pi power supply
   - Avoid USB hubs for audio/MIDI devices

### Problem: High Latency

**Symptoms:** Noticeable delay between key press and sound

**Solutions:**

1. **Reduce buffer size** (rebuild required):
   - Edit `src/platform/pi/audio_driver.cpp`
   - Set buffer_size = 512, period_size = 64
   - Rebuild and test

2. **Use USB 3.0 ports** (blue ports on Pi 4)

3. **Disable WiFi/Bluetooth:**
   ```bash
   sudo systemctl disable bluetooth wpa_supplicant
   ```

### Problem: Build Errors

**Symptoms:** Compilation fails with errors

**Solutions:**

1. **Update system:**
   ```bash
   sudo apt update && sudo apt upgrade
   ```

2. **Reinstall dependencies:**
   ```bash
   sudo apt install --reinstall build-essential cmake libasound2-dev
   ```

3. **Clean and rebuild:**
   ```bash
   cd build-pi
   rm -rf *
   cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

4. **Check CMake version:**
   ```bash
   cmake --version
   # Should be 3.20 or higher
   ```

### Problem: Pi Overheating

**Symptoms:** Temperature > 80°C, throttling warnings

**Solutions:**

1. **Add cooling:**
   - Install fan
   - Add heatsinks

2. **Monitor temperature:**
   ```bash
   watch -n 1 vcgencmd measure_temp
   ```

3. **Check throttling:**
   ```bash
   vcgencmd get_throttled
   # 0x0 = no throttling
   # Other values = throttled
   ```

4. **Reduce overclock** (if overclocked)

---

## Performance Monitoring

### CPU Usage

```bash
# Monitor in real-time
top

# Look for "poor-house-juno" process
# CPU% should be < 50% with 6 voices + chorus
```

### Audio Thread Priority

```bash
ps -eLo pid,tid,class,rtprio,comm | grep poor
```

### Temperature

```bash
# Current temperature
vcgencmd measure_temp

# Continuous monitoring
watch -n 1 vcgencmd measure_temp
```

### ALSA Underruns

```bash
# Check for underruns (audio dropouts)
cat /proc/asound/card1/pcm0p/sub0/status
```

---

## Advanced Configuration

### Custom Audio Settings

Edit `src/platform/pi/audio_driver.cpp` to customize:

```cpp
// Sample rate
snd_pcm_hw_params_set_rate(handle, params, 44100, 0);  // Change from 48000

// Buffer size (latency vs stability)
snd_pcm_hw_params_set_buffer_size(handle, params, 512);  // Smaller = lower latency

// Period size (processing block)
snd_pcm_hw_params_set_period_size(handle, params, 64, 0);  // Smaller = lower latency
```

**Rebuild after changes!**

### Network Performance

If using SSH or networked MIDI:

```bash
# Optimize network latency
sudo nano /etc/sysctl.conf
```

Add:
```
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
```

Apply:
```bash
sudo sysctl -p
```

---

## References

- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/)
- [ALSA Project](https://www.alsa-project.org/)
- [Real-Time Linux Audio](https://wiki.linuxaudio.org/)
- [Pi Audio Latency Guide](https://wiki.linuxaudio.org/wiki/raspberrypi)

---

**Last Updated:** January 10, 2026
