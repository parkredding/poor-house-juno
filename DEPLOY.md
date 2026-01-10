# Quick Deployment Guide

**Poor House Juno - Raspberry Pi with 3.5mm Audio + Arturia MiniLab**

This guide covers deploying Poor House Juno on a Raspberry Pi using:
- **Audio Output:** Raspberry Pi built-in 3.5mm headphone jack
- **MIDI Controller:** Arturia MiniLab (knobs as modifiers)

---

## Prerequisites

- Raspberry Pi 4 (2GB+ RAM)
- Raspberry Pi OS installed
- Arturia MiniLab connected via USB
- Speakers/headphones connected to 3.5mm jack

---

## Quick Setup (15 minutes)

### 1. Clone and Build

```bash
# Clone repository
git clone https://github.com/parkredding/poor-house-juno.git
cd poor-house-juno

# Install dependencies
./scripts/setup_pi.sh

# Build for Raspberry Pi
make pi
```

### 2. Configure Audio for 3.5mm Jack

```bash
# Test the built-in 3.5mm jack
speaker-test -D hw:0,0 -c 2 -r 48000
# You should hear white noise from both channels
# Press Ctrl+C to stop

# Adjust volume
alsamixer
# Use arrow keys to set volume to ~80%
# Press Esc to exit
```

### 3. Connect Arturia MiniLab

```bash
# List MIDI devices
amidi -l

# You should see something like:
# IO  hw:1,0,0  Arturia MiniLab MIDI 1

# Test MIDI input
amidi -p hw:1,0,0 -d
# Move knobs/play keys - you should see hex data
# Press Ctrl+C to stop
```

### 4. Run the Synth

```bash
# Run with 3.5mm audio and MiniLab MIDI
./build-pi/poor-house-juno --audio hw:0,0 --midi hw:1,0,0
```

**Expected output:**
```
Poor House Juno - Raspberry Pi Edition
=======================================
6-Voice Polyphonic Juno-106 Emulator
=======================================

Audio initialized: 48000 Hz, 128 samples/buffer
MIDI initialized: hw:1,0,0

Audio running at 48000 Hz
Buffer size: 128 samples
Latency: ~2.67 ms

Ready for MIDI input. Press Ctrl+C to exit.
```

---

## Arturia MiniLab Controls

The MiniLab knobs are already mapped and ready to use:

| Knob | Parameter | Function |
|------|-----------|----------|
| **1** | Filter Cutoff | Brightness/tone |
| **2** | Resonance | Filter emphasis |
| **3** | Filter Env | Envelope depth |
| **4** | LFO Rate | Modulation speed |
| **5** | Saw Level | Sawtooth mix |
| **6** | Pulse Level | Pulse wave mix |
| **7** | Sub Level | Sub-oscillator mix |
| **8** | Noise Level | Noise mix |

**Mod Wheel:** Controls LFO depth in real-time
**Pitch Bend:** Pitch bend (±2 semitones default)

**All 29 MIDI CCs are supported** - see `docs/midi_cc_map.md` for complete mapping.

---

## Performance Tips

### CPU Usage
- Normal: 35-45% with 6 voices + chorus
- Target: <50% for stable performance
- Check CPU: Watch the console output (updated every 5 seconds)

### Audio Quality
The 3.5mm jack provides:
- 16-bit/48kHz output
- Adequate for practice, jamming, and small performances
- For studio quality, use a USB audio interface (see `docs/pi_setup.md`)

### Reducing Latency
Current latency: ~2.7ms (128 samples @ 48kHz)

To reduce further (requires more CPU):
```bash
# Edit src/platform/pi/audio_driver.cpp
# Change BUFFER_SIZE from 128 to 64
# Rebuild: make pi
```

---

## Auto-Start on Boot (Optional)

To make the synth start automatically when the Pi boots:

```bash
# Copy the systemd service
sudo cp scripts/poor-house-juno.service /etc/systemd/system/

# Edit the service file to use 3.5mm audio
sudo nano /etc/systemd/system/poor-house-juno.service
# Change ExecStart line to:
# ExecStart=/home/pi/poor-house-juno/build-pi/poor-house-juno --audio hw:0,0 --midi hw:1,0,0

# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable poor-house-juno
sudo systemctl start poor-house-juno

# Check status
sudo systemctl status poor-house-juno

# View logs
journalctl -u poor-house-juno -f
```

To stop auto-start:
```bash
sudo systemctl disable poor-house-juno
```

---

## Troubleshooting

### No Sound from 3.5mm Jack

**Check volume:**
```bash
alsamixer
# Ensure volume is not muted (MM = muted, use M to toggle)
```

**Force audio to 3.5mm jack:**
```bash
# Some Pi models route to HDMI by default
sudo raspi-config
# System Options > Audio > Headphones
```

**Test directly:**
```bash
speaker-test -D hw:0,0 -c 2
```

### MIDI Not Working

**Check connections:**
```bash
# List MIDI devices
amidi -l

# If MiniLab not shown, reconnect USB
# Try different USB port
```

**Check permissions:**
```bash
# Add user to audio group (if not already)
sudo usermod -a -G audio,dialout $USER
# Log out and back in
```

### CPU Usage Too High

**Optimize performance:**
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable desktop (if running Pi OS Desktop)
sudo systemctl set-default multi-user.target
sudo reboot
```

### Audio Clicks/Pops

**Increase buffer size:**
```bash
# Edit src/platform/pi/audio_driver.cpp
# Change BUFFER_SIZE from 128 to 256
# Rebuild: make pi
```

**Check for throttling:**
```bash
vcgencmd get_throttled
# 0x0 = good, other values = thermal throttling
# Add heatsink/fan if throttling occurs
```

---

## Next Steps

### Customize Your Sound
Edit the default preset in `src/platform/pi/main.cpp` (function `initializeDefaultParameters`)

### Explore All Features
- **29 MIDI CCs** for complete control (see `docs/midi_cc_map.md`)
- **4 voice allocation modes** (Oldest, Newest, Low-Note, High-Note)
- **HPF modes** for bass control (Off/30Hz/60Hz/120Hz)
- **Chorus modes** (Off, I, II, I+II)
- **Pitch bend range** configurable ±2 to ±12 semitones
- **Portamento/glide** for smooth note transitions
- **Sustain pedal** support

### Advanced Setup
See comprehensive documentation:
- `docs/pi_setup.md` - Full Pi setup guide
- `docs/midi_cc_map.md` - Complete MIDI CC reference
- `docs/architecture.md` - System architecture
- `README.md` - Full project documentation

---

## Performance Targets

**Achieved on Raspberry Pi 4 (4GB):**
- CPU Usage: 35-45% (6 voices + chorus)
- Latency: 2.67ms (128 samples @ 48kHz)
- Polyphony: 6 voices
- Sample Rate: 48kHz
- Quality: Production-ready

**Good to know:**
- Pi 3B+ can run it but at 70-80% CPU
- Raspberry Pi 5 runs it at ~25% CPU with headroom for more effects

---

## Quick Reference Commands

```bash
# Build
make pi

# Run with default devices
./build-pi/poor-house-juno

# Run with specific devices
./build-pi/poor-house-juno --audio hw:0,0 --midi hw:1,0,0

# List audio devices
aplay -l

# List MIDI devices
amidi -l

# Test audio
speaker-test -D hw:0,0 -c 2

# Test MIDI
amidi -p hw:1,0,0 -d

# Check CPU temp
vcgencmd measure_temp

# Adjust volume
alsamixer
```

---

**Enjoy your Poor House Juno synthesizer!**

For questions, issues, or contributions:
https://github.com/parkredding/poor-house-juno
