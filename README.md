# Poor House Juno

A standalone Roland Juno-106 synthesizer emulator for Raspberry Pi 4, reverse-engineered from TAL-U-NO-LX behavior, with a browser-based testing environment.

## Project Status

**Current Milestone:** M6 - Single Voice Integration ✅

**Completed Milestones:**
- [x] **M1:** Project Setup (repository, build system, basic audio)
- [x] **M2:** Oscillator (DCO with polyBLEP, PWM, sub-oscillator, noise)
- [x] **M3:** Filter (IR3109 4-pole ladder with envelope modulation)
- [x] **M4:** Envelopes (Filter and Amplitude ADSR)
- [x] **M5:** LFO (Triangle wave modulation for pitch and PWM)
- [x] **M6:** Single Voice Integration (Voice and Synth classes)

**Next Steps:**
- M7: Polyphony (6 voices)
- M8: Chorus (BBD emulation)
- M9: Web Interface Polish
- M10+: Pi Integration and Optimization

## Overview

Poor House Juno is a high-fidelity Juno-106 emulator that:

- **Runs on Raspberry Pi 4** as a standalone hardware synth
- **Includes a web-based test environment** for development without hardware
- **Uses identical DSP code** on both platforms for bit-accurate output
- **Reverse-engineers TAL-U-NO-LX** to match its sound and behavior

### Key Features (Planned)

- 6-voice polyphony with voice stealing
- Digitally Controlled Oscillator (DCO) with saw, pulse, PWM, sub-oscillator, and noise
- IR3109 4-pole ladder filter emulation with resonance
- Dual ADSR envelopes (filter and amp)
- Triangle LFO with multiple destinations
- BBD stereo chorus (modes I, II, and I+II)
- MIDI input via ALSA (Pi) or Web MIDI API (browser)
- Low-latency audio via ALSA (Pi) or Web Audio (browser)

## Architecture

```
poor-house-juno/
├── src/
│   ├── dsp/              # Core DSP (shared between Pi and Web)
│   │   ├── types.h
│   │   ├── oscillator.h/.cpp
│   │   └── ... (filter, envelope, LFO, chorus, etc.)
│   └── platform/
│       ├── pi/           # Raspberry Pi (ALSA audio/MIDI)
│       └── web/          # Web (Emscripten + Web Audio)
├── web/                  # Browser-based test interface
├── scripts/              # Build and deployment scripts
└── tests/                # Unit and integration tests
```

## Quick Start

### Web Build (Development/Testing)

**Requirements:**
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- Python 3 (for local web server)

**Build:**

```bash
# Install Emscripten (first time only)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..

# Build for web
make web

# Serve and test
make serve
# Open http://localhost:8000 in your browser
```

### Raspberry Pi Build

**Requirements:**
- Raspberry Pi 4 with Raspberry Pi OS
- ALSA development libraries
- CMake 3.20+

**Setup on Pi:**

```bash
# One-time setup (on the Pi)
./scripts/setup_pi.sh

# Build
make pi

# Run
./build-pi/poor-house-juno
```

**Cross-compile from x86/x64:**

```bash
# Cross-compilation setup is planned for future milestones
# For now, build natively on the Pi
```

## Usage

### Web Interface

**Option 1: GitHub Pages (Live Demo)**
- Visit the live demo at: https://parkredding.github.io/poor-house-juno/
- Note: Deployment requires manual build and push (see Deployment section below)

**Option 2: Local Development**
1. Build and serve the web version (see above)
2. Open http://localhost:8000 in a modern browser (Chrome/Edge recommended for best Web Audio support)
3. Click **"Start Audio"** to initialize the audio engine
4. Use the controls to adjust all synth parameters
5. Click **"Play A4"** to test a 440 Hz note
6. Connect a MIDI controller for live playback (requires Web MIDI support)

**Features:**
- Real-time parameter control
- MIDI input visualization
- Oscilloscope display
- MIDI device selection

### Raspberry Pi

```bash
# Run with default audio/MIDI devices
./build-pi/poor-house-juno

# Specify devices
./build-pi/poor-house-juno --audio hw:1,0 --midi hw:1,0
```

**Current behavior (M1):**
- Plays a 440 Hz sine wave for 3 seconds on startup (if no MIDI available)
- Responds to MIDI Note On/Off messages
- Press Ctrl+C to exit

## Development Workflow

1. **Develop DSP code** in `src/dsp/` (C++17, platform-agnostic)
2. **Test in browser** (fast iteration, no hardware needed)
3. **Build for Pi** when ready
4. **Deploy to Pi** via `./scripts/deploy_pi.sh <pi-hostname>`
5. **Compare outputs** using reference comparison tools (future milestone)

## Testing

The web interface includes built-in testing tools:

- **Oscilloscope:** Real-time waveform visualization
- **MIDI monitor:** Display incoming MIDI messages
- **Parameter control:** Live parameter adjustment
- **A/B comparison:** (Planned) Compare output to TAL-U-NO-LX reference recordings

**Unit tests** (planned):
```bash
mkdir build-test
cd build-test
cmake .. -DBUILD_TESTS=ON
make
ctest
```

## Technical Details

### DSP Implementation

- **Sample rate:** 48000 Hz (primary), 44100 Hz supported
- **Buffer size:** 128 samples (configurable)
- **Precision:** 32-bit float internal processing
- **Algorithm:** Zero-Delay Feedback (ZDF) for filter, polyBLEP for oscillators

### Platform Integration

**Raspberry Pi:**
- Audio: ALSA (PCM playback, float format)
- MIDI: ALSA RawMIDI
- Threading: pthread for audio/MIDI threads
- Latency target: <10ms round-trip

**Web:**
- Audio: Web Audio API with AudioWorklet
- MIDI: Web MIDI API
- WASM: Emscripten with Embind
- Shared WASM heap for zero-copy audio buffers

### Performance Targets

- **Pi 4:** 6 voices + chorus + effects at <50% CPU (128-sample buffer)
- **Web:** 6 voices + chorus at <30% single core

## Building from Source

### Manual CMake Build (Web)

```bash
source /path/to/emsdk/emsdk_env.sh

mkdir build-web
cd build-web
emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)

# Files will be copied to web/ directory automatically
cd ../web
python3 -m http.server 8000
```

### Manual CMake Build (Pi)

```bash
mkdir build-pi
cd build-pi
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

./poor-house-juno
```

## Deployment

### Manual GitHub Pages Deployment

Since automated deployment is not available, use the manual deployment script:

```bash
# Build and deploy to GitHub Pages
./scripts/deploy_web.sh
```

**What this script does:**
1. Builds the web version using Emscripten
2. Creates or updates the `gh-pages` branch
3. Copies all web files to the branch root
4. Commits and pushes to `origin/gh-pages`

**First-time setup:**
After your first deployment, configure GitHub Pages:
1. Go to your repository on GitHub
2. Navigate to **Settings > Pages**
3. Under **Source**, select the `gh-pages` branch
4. Click **Save**

Your site will be available at `https://<username>.github.io/poor-house-juno/` within a few minutes.

**Requirements:**
- Emscripten SDK must be installed and activated
- You must have push access to the repository
- The working directory should be clean (commit your changes first)

## Project Roadmap

See [docs/architecture.md](docs/architecture.md) (planned) for detailed roadmap.

**Milestones:**
- [x] **M1:** Project Setup
- [x] **M2:** Oscillator (DCO with polyBLEP)
- [x] **M3:** Filter (IR3109 ladder)
- [x] **M4:** Envelopes (ADSR)
- [x] **M5:** LFO
- [x] **M6:** Single Voice Integration
- [ ] **M7:** Polyphony (6 voices)
- [ ] **M8:** Chorus (BBD emulation)
- [ ] **M9:** Web Interface Polish
- [ ] **M10:** Pi Integration and Optimization
- [ ] **M11:** Final Polish and Reference Matching

## Documentation

- [Architecture](docs/architecture.md) (planned)
- [DSP Design](docs/dsp_design.md) (planned)
- [Juno-106 Analysis](docs/juno106_analysis.md) (planned)
- [Filter Tuning](docs/filter_tuning.md) (planned)
- [Chorus Analysis](docs/chorus_analysis.md) (planned)
- [Pi Setup Guide](docs/pi_setup.md) (planned)

## Tools

- `tools/analyze_tal.py` - Analyze TAL-U-NO-LX parameters
- `tools/measure_filter.py` - Measure filter frequency response
- `tools/measure_chorus.py` - Analyze chorus characteristics
- `tools/export_preset.py` - Convert TAL presets

*(Tools will be implemented in future milestones)*

## Contributing

This project is in active development. Contributions are welcome once the core architecture is stable (post-M6).

## License

See [LICENSE](LICENSE) file for details.

## Credits

- **Original hardware:** Roland Juno-106
- **Inspiration:** TAL-U-NO-LX by Togu Audio Line
- **Platform:** Raspberry Pi Foundation
- **Build system:** Emscripten, CMake

## Acknowledgments

- Cytomic for VA filter design resources
- Vadim Zavalishin for "The Art of VA Filter Design"
- Julius O. Smith for DSP references
- The synth DIY and emulation community

---

**Poor House Juno** - Because you can't afford the real thing, but you can build the next best thing! 🎹
