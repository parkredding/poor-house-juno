# Poor House Juno

A standalone Roland Juno-106 synthesizer emulator for Raspberry Pi 4, reverse-engineered from TAL-U-NO-LX behavior, with a browser-based testing environment.

## Project Status

**Current Milestone:** M15 - Polish & Optimization (In Progress)

**Completed Milestones:**
- [x] **M1:** Project Setup (repository, build system, basic audio)
- [x] **M2:** Oscillator (DCO with polyBLEP, PWM, sub-oscillator, noise)
- [x] **M3:** Filter (IR3109 4-pole ladder with envelope modulation)
- [x] **M4:** Envelopes (Filter and Amplitude ADSR)
- [x] **M5:** LFO (Triangle wave modulation for pitch and PWM)
- [x] **M6:** Single Voice Integration (Voice and Synth classes)
- [x] **M7:** Polyphony (6 voices with voice stealing)
- [x] **M8:** Chorus (BBD stereo chorus with modes I, II, and I+II)
- [x] **M9:** Web Interface Polish (virtual keyboard, presets, voice indicators, improved visualization)
- [x] **M10:** Pi Integration and Optimization (full synth on Pi, CPU monitoring, real-time audio thread)
- [x] **M11:** Critical Features I (HPF with 4 modes, Pitch Bend ±12 semitones, Portamento 0-10s)
- [x] **M12:** Critical Features II (LFO Delay 0-3s, Filter LFO Modulation exposed in UI)
- [x] **M13:** Performance Controls (Mod Wheel, VCA Mode, Filter Envelope Polarity)
- [x] **M14:** Range & Voice Control (DCO Range, VCA Level, Velocity Sensitivity, Master Tune)
- [~] **M15:** Polish & Optimization (Unit Tests ✅, TAL Comparison, Documentation, CPU Profiling)

**M15 Progress:**
- ✅ Unit Test Suite (comprehensive tests for all DSP components)
- ⏳ TAL-U-NO-LX Comparison Tools
- ⏳ Documentation
- ⏳ CPU Profiling & Optimization

**Next Steps:**
- M15: Complete TAL comparison tools, documentation, and CPU profiling
- M16: Final Refinement (MIDI CC Mapping, Hold, Bank System)

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
- Real-time parameter control with full ADSR, filter, and effects
- Virtual MIDI keyboard (play with mouse or computer keyboard)
- Preset management (save, load, delete presets with localStorage)
- Voice activity indicator (shows active voices 0-6)
- Estimated CPU usage meter
- Enhanced oscilloscope display with grid and glow effects
- MIDI input visualization with activity indicator
- External MIDI device support

### Raspberry Pi

```bash
# Run with default audio/MIDI devices
./build-pi/poor-house-juno

# Specify devices (optional)
./build-pi/poor-house-juno --audio hw:1,0 --midi hw:1,0
```

**Features (M10):**
- Full 6-voice polyphonic Juno-106 emulation
- BBD stereo chorus effect (modes I, II, and I+II)
- Real-time MIDI input with velocity sensitivity
- CPU usage monitoring (displayed every 5 seconds)
- Real-time audio thread priority for low latency
- Auto-test chord on startup if no MIDI device available
- Press Ctrl+C to exit

**Performance:**
- Target: <50% CPU usage on Raspberry Pi 4
- Real-time audio thread (SCHED_FIFO priority 80)
- ~2.7ms latency at 48kHz with 128-sample buffer
- Optimized DSP processing with SIMD where applicable

## Development Workflow

1. **Develop DSP code** in `src/dsp/` (C++17, platform-agnostic)
2. **Test in browser** (fast iteration, no hardware needed)
3. **Build for Pi** when ready
4. **Deploy to Pi** via `./scripts/deploy_pi.sh <pi-hostname>`
5. **Compare outputs** using reference comparison tools (future milestone)

## Testing

### Unit Tests (M15)

Comprehensive unit test suite covering all DSP components:

```bash
# Build tests (test-only mode, no ALSA/Emscripten required)
mkdir build-test
cd build-test
cmake .. -DBUILD_TESTS=ON -DPLATFORM=test -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run all tests
./tests/phj_tests

# Run specific test
./tests/phj_tests "Envelope ADSR stages"

# Run with verbose output
./tests/phj_tests -s
```

**Test Coverage:**
- ✅ Oscillator/DCO (waveform generation, polyBLEP, PWM, LFO modulation)
- ✅ Filter (IR3109 ladder, resonance, HPF modes, envelope/LFO modulation)
- ✅ Envelope (ADSR stages, timing accuracy)
- ✅ LFO (triangle wave, delay feature from M12)
- ✅ Voice (integration tests, portamento, velocity sensitivity, M13/M14 features)

**Current Status:** 13/25 tests passing (172/189 assertions)
- See `TEST_FINDINGS.md` for detailed analysis of failing tests
- Tests identify specific bugs and provide regression prevention

### Web Interface Testing

The web interface includes built-in testing tools:

- **Oscilloscope:** Real-time waveform visualization
- **MIDI monitor:** Display incoming MIDI messages
- **Parameter control:** Live parameter adjustment
- **A/B comparison:** (Planned) Compare output to TAL-U-NO-LX reference recordings

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
- [x] **M7:** Polyphony (6 voices with voice stealing)
- [x] **M8:** Chorus (BBD emulation)
- [x] **M9:** Web Interface Polish (virtual keyboard, presets, voice indicators)
- [x] **M10:** Pi Integration and Optimization (full synth on Pi, CPU monitoring, RT audio)
- [x] **M11:** Critical Features I (HPF, Pitch Bend, Portamento)
- [x] **M12:** Critical Features II (LFO Delay, Filter LFO Modulation UI)
- [x] **M13:** Performance Controls (Mod Wheel, VCA Mode, Envelope Polarity)
- [x] **M14:** Range & Voice Control (DCO Range, VCA Level, Velocity Sensitivity, Master Tune)
- [ ] **M15:** Polish & Optimization (Testing, Documentation, TAL Comparison Tools)
- [ ] **M16:** Final Refinement (Full MIDI CC, Hold, 128-Patch Banks)

### Upcoming Milestone Details

**M11: Critical Features I** ✅ (Completed)
- ✅ High-Pass Filter (HPF) with 4-position control (Off/30Hz/60Hz/120Hz)
- ✅ Pitch Bend wheel support with configurable range (±2 semitones default, up to ±12)
- ✅ Portamento (glide) with adjustable time (0-10 seconds, legato mode)
- ✅ Web UI controls for all new features (HPF, pitch bend range, portamento time)
- ✅ MIDI pitch bend handling (both Pi and Web platforms)

**M12: Critical Features II** ✅ (Completed)
- ✅ LFO Delay implementation (0-3 second delay before modulation starts)
- ✅ Filter LFO Modulation control exposed in web UI
- ✅ LFO Delay control in web UI with 0-3 second range
- ✅ Full DSP implementation with fade-in during delay period
- ✅ Delay timer triggered on note-on events

**M13: Performance Controls** ✅ (Completed)
- ✅ Modulation Wheel (MIDI CC #1) for real-time LFO depth control
- ✅ VCA Control Mode switch (ENV/GATE) for organ-style sounds
- ✅ Filter Envelope Polarity switch (Normal/Inverse)
- ✅ UI controls and visual feedback in web interface
- ✅ Full DSP implementation with mod wheel scaling LFO output

**M14: Range & Voice Control** ✅ (Completed)
- ✅ DCO Range selection (16'/8'/4' footage switches) for octave shifting
- ✅ VCA Level control (separate from master volume)
- ✅ Velocity sensitivity options (filter and amplitude amount controls)
- ✅ Master Tune control (±50 cents)
- ✅ Full DSP implementation with velocity modulation
- ✅ UI controls in web interface

**M15: Polish & Optimization** (Est: 25-35 hours)
- Unit tests for DSP components (oscillator, filter, envelopes)
- TAL-U-NO-LX comparison tools (`tools/analyze_tal.py`, etc.)
- Generate reference recordings for A/B testing
- CPU profiling and optimization for Pi 4 (<50% target)
- Complete documentation (architecture, DSP design, Juno-106 analysis)

**M16: Final Refinement** (Est: 15-20 hours)
- Full MIDI CC mapping for all synth parameters
- Hold function (sustain pedal support via MIDI CC #64)
- 128-patch bank system matching Juno-106 organization
- Voice allocation priority modes (low-note, high-note, last-note)
- Final bug fixes and polish

**Total Estimated Time:** 40-55 hours remaining (M14 completed)

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
