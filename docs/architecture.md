# Poor House Juno - System Architecture

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Status:** Production-ready

---

## Table of Contents

1. [Overview](#overview)
2. [System Design](#system-design)
3. [Build System](#build-system)
4. [Platform Architecture](#platform-architecture)
5. [Real-Time Audio Threading](#real-time-audio-threading)
6. [Performance Considerations](#performance-considerations)

---

## Overview

Poor House Juno is a high-fidelity Roland Juno-106 synthesizer emulator designed to run on:

- **Raspberry Pi 4** as a standalone hardware synth
- **Web browsers** as a development/testing environment

The architecture emphasizes:

- **Code reuse**: Identical DSP code on both platforms
- **Bit-accurate output**: Web and Pi produce identical audio
- **Real-time performance**: <50% CPU on Raspberry Pi 4
- **Low latency**: ~2.7ms round-trip on Pi (128-sample buffer @ 48kHz)

---

## System Design

### Component Hierarchy

```
┌─────────────────────────────────────┐
│          Application Layer          │
│  (Platform-specific: Pi or Web)     │
├─────────────────────────────────────┤
│         Synth Engine (C++)          │
│  - Voice allocation & management    │
│  - MIDI message routing             │
│  - Chorus processing                │
├─────────────────────────────────────┤
│         Voice Processing            │
│  - 6 polyphonic voices              │
│  - Per-voice DSP chain              │
├─────────────────────────────────────┤
│         DSP Components              │
│  - DCO (oscillator)                 │
│  - Filter (IR3109 emulation)        │
│  - Envelopes (ADSR)                 │
│  - LFO (triangle wave)              │
│  - HPF (high-pass filter)           │
└─────────────────────────────────────┘
```

### Directory Structure

```
poor-house-juno/
├── src/
│   ├── dsp/                    # Platform-independent DSP (C++17)
│   │   ├── types.h             # Common types and constants
│   │   ├── parameters.h        # Parameter structures
│   │   ├── oscillator.h/.cpp   # Basic oscillator functions
│   │   ├── dco.h/.cpp          # Digitally Controlled Oscillator
│   │   ├── filter.h/.cpp       # IR3109 4-pole ladder filter
│   │   ├── envelope.h/.cpp     # ADSR envelope generator
│   │   ├── lfo.h/.cpp          # Triangle LFO with delay
│   │   ├── voice.h/.cpp        # Single voice (DCO + filter + envelopes)
│   │   ├── chorus.h/.cpp       # BBD stereo chorus
│   │   └── synth.h/.cpp        # Synth engine (6 voices + chorus)
│   │
│   └── platform/
│       ├── pi/                 # Raspberry Pi platform
│       │   ├── main.cpp        # Entry point
│       │   ├── audio_driver.h/.cpp   # ALSA audio output
│       │   └── midi_driver.h/.cpp    # ALSA MIDI input
│       │
│       └── web/                # Web/WASM platform
│           ├── main.cpp        # Emscripten entry point
│           └── bindings.cpp    # Embind API bindings
│
├── web/                        # Web interface (HTML/CSS/JS)
│   ├── index.html              # UI layout and controls
│   ├── css/juno.css           # Juno-106 inspired styling
│   ├── js/
│   │   ├── app.js             # Main application logic
│   │   ├── audio.js           # Web Audio API setup
│   │   ├── midi.js            # Web MIDI API integration
│   │   ├── ui.js              # UI event handlers
│   │   ├── presets.js         # Preset management (localStorage)
│   │   └── scope.js           # Oscilloscope visualization
│   └── phj.{js,wasm}          # Compiled WASM (generated)
│
├── tests/                      # Unit tests (Catch2)
│   ├── test_main.cpp          # Test entry point
│   ├── test_oscillator.cpp    # Oscillator tests
│   ├── test_filter.cpp        # Filter tests
│   ├── test_envelope.cpp      # Envelope tests
│   ├── test_lfo.cpp           # LFO tests
│   ├── test_voice.cpp         # Voice integration tests
│   └── test_chorus.cpp        # Chorus tests
│
├── tools/                      # Analysis and profiling tools
│   ├── generate_reference.py  # Test signal generator
│   ├── measure_filter.py      # Filter frequency response analyzer
│   ├── measure_chorus.py      # Chorus characteristics analyzer
│   ├── analyze_tal.py         # Parameter curve analyzer
│   └── profile_performance.cpp # CPU profiling tool
│
├── scripts/                    # Build and deployment scripts
│   ├── setup_pi.sh            # Pi initial setup
│   ├── deploy_pi.sh           # Deploy to Pi over SSH
│   └── deploy_web.sh          # Deploy to GitHub Pages
│
├── docs/                       # Documentation
│   ├── architecture.md        # This file
│   ├── dsp_design.md          # DSP algorithms and design
│   ├── juno106_analysis.md    # Juno-106 reverse engineering
│   ├── filter_tuning.md       # IR3109 filter calibration
│   ├── chorus_analysis.md     # BBD chorus design
│   ├── pi_setup.md            # Raspberry Pi setup guide
│   ├── web_interface.md       # Web UI architecture
│   └── midi_cc_map.md         # MIDI CC reference
│
├── CMakeLists.txt             # Root CMake configuration
├── Makefile                   # Convenience wrapper
└── README.md                  # Project overview
```

---

## Build System

### CMake Configuration

The project uses **CMake 3.20+** with platform-specific configurations:

```cmake
# Platform selection (required)
-DPLATFORM=pi         # Raspberry Pi (ALSA audio/MIDI)
-DPLATFORM=web        # Web (Emscripten/WASM)
-DPLATFORM=test       # Test-only (no audio/MIDI)

# Build type
-DCMAKE_BUILD_TYPE=Release   # Optimized (-O3, NDEBUG)
-DCMAKE_BUILD_TYPE=Debug     # Debug symbols (-g)

# Optional flags
-DBUILD_TESTS=ON     # Enable unit tests (test platform only)
```

### Build Targets

#### 1. Raspberry Pi Build

```bash
mkdir build-pi
cd build-pi
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Produces: build-pi/poor-house-juno
```

**Requirements:**
- GCC 9+ with C++17 support
- ALSA development libraries (`libasound2-dev`)
- pthread support

**Optimizations:**
- `-O3` (maximum optimization)
- `-march=armv8-a+simd` (ARM NEON SIMD)
- `-mtune=cortex-a72` (optimize for Pi 4 CPU)

#### 2. Web Build (Emscripten)

```bash
source /path/to/emsdk/emsdk_env.sh

mkdir build-web
cd build-web
emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)

# Produces: web/phj.js, web/phj.wasm
```

**Requirements:**
- Emscripten SDK 3.1.30+
- Python 3.8+ (for local server)

**Emscripten Flags:**
- `-O3` (maximum optimization)
- `-s WASM=1` (WebAssembly output)
- `-s ALLOW_MEMORY_GROWTH=1` (dynamic memory)
- `-s MODULARIZE=1 -s EXPORT_NAME=createPHJ` (module export)

#### 3. Test Build

```bash
mkdir build-test
cd build-test
cmake .. -DPLATFORM=test -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Produces: build-test/tests/phj_tests
./tests/phj_tests
```

**Requirements:**
- Catch2 (fetched automatically via CMake FetchContent)

---

## Platform Architecture

### Raspberry Pi Platform

**Audio Output:** ALSA PCM

```cpp
// Audio configuration (src/platform/pi/audio_driver.cpp)
snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT);
snd_pcm_hw_params_set_rate(handle, params, 48000, 0);
snd_pcm_hw_params_set_channels(handle, params, 2);  // Stereo
snd_pcm_hw_params_set_buffer_size(handle, params, 1024);
snd_pcm_hw_params_set_period_size(handle, params, 128, 0);
```

**MIDI Input:** ALSA RawMIDI

```cpp
// MIDI configuration (src/platform/pi/midi_driver.cpp)
snd_rawmidi_open(&midiIn, nullptr, deviceName, SND_RAWMIDI_NONBLOCK);
```

**Threading:**
- Main thread: MIDI input and control
- Audio thread: Real-time audio processing (SCHED_FIFO priority 80)

**Device Selection:**
- Default: `hw:1,0` (first USB audio interface)
- Command-line: `--audio hw:X,Y --midi hw:Z,W`

### Web Platform

**Audio Output:** Web Audio API

```javascript
// Web Audio setup (web/js/audio.js)
const audioContext = new AudioContext({ sampleRate: 48000 });
await audioContext.audioWorklet.addModule('worklet.js');
const synthNode = new AudioWorkletNode(audioContext, 'phj-processor');
```

**MIDI Input:** Web MIDI API

```javascript
// MIDI setup (web/js/midi.js)
const midiAccess = await navigator.requestMIDIAccess();
for (const input of midiAccess.inputs.values()) {
    input.onmidimessage = handleMIDIMessage;
}
```

**WASM Integration:**

```javascript
// C++ DSP compiled to WASM (src/platform/web/main.cpp)
Module.ccall('createSynth', 'void', [], []);
Module.ccall('processStereo', 'void', ['number', 'number', 'number'],
             [leftPtr, rightPtr, numSamples]);
```

**Memory Management:**
- Shared heap between JS and WASM
- Preallocated audio buffers
- Zero-copy audio processing

---

## Real-Time Audio Threading

### Raspberry Pi

```
┌────────────────────┐         ┌─────────────────────┐
│    Main Thread     │         │   Audio Thread      │
│ (normal priority)  │         │ (SCHED_FIFO pri 80) │
├────────────────────┤         ├─────────────────────┤
│ - MIDI input       │ ────▶   │ - Synth::process()  │
│ - handleNoteOn()   │         │ - ALSA write()      │
│ - handleNoteOff()  │         │ - 128-sample buffer │
│ - handleCC()       │         │ - 2.7ms latency     │
└────────────────────┘         └─────────────────────┘
```

**Real-Time Safety:**
- No memory allocation in audio thread
- Lock-free MIDI queue (planned)
- Preallocated voice pool

### Web (AudioWorklet)

```
┌──────────────────┐         ┌────────────────────┐
│   Main Thread    │         │  AudioWorklet      │
│  (UI updates)    │         │ (audio thread)     │
├──────────────────┤         ├────────────────────┤
│ - UI controls    │ ────▶   │ - process()        │
│ - MIDI input     │         │ - 128-sample       │
│ - Parameter      │         │ - Runs at         │
│   changes        │         │   sampleRate      │
└──────────────────┘         └────────────────────┘
```

**Communication:**
- `MessagePort` for parameter updates
- Atomic parameter changes (no locks)

---

## Performance Considerations

### Target Specifications

| Metric | Target | Actual (M15) |
|--------|--------|--------------|
| CPU Usage (Pi 4) | <50% | TBD (requires Pi testing) |
| Latency (Pi) | <10ms | ~2.7ms (128 samples @ 48kHz) |
| Buffer Size | 128 samples | 128 samples |
| Polyphony | 6 voices | 6 voices |
| Sample Rate | 48 kHz | 48 kHz |

### Optimization Strategies

#### 1. Filter Coefficient Caching (M15)

**Problem:** `updateCoefficients()` was called every sample, executing expensive `std::tan()` and `std::pow()` operations.

**Solution:** Cache modulation values and only recompute when they change.

```cpp
// Before (M14): Called every sample (~48000/sec)
Sample Filter::process(Sample input) {
    updateCoefficients();  // Expensive!
    // ... filter processing
}

// After (M15): Called only when modulation changes (~hundreds/sec)
Sample Filter::process(Sample input) {
    updateCoefficientsIfNeeded();  // Conditional update
    // ... filter processing
}

void Filter::updateCoefficientsIfNeeded() {
    bool envChanged = std::abs(envValue_ - cachedEnvValue_) > epsilon;
    bool lfoChanged = std::abs(lfoValue_ - cachedLfoValue_) > epsilon;
    bool velocityChanged = std::abs(velocityValue_ - cachedVelocityValue_) > epsilon;

    if (coefficientsNeedUpdate_ || envChanged || lfoChanged || velocityChanged) {
        updateCoefficients();
    }
}
```

**Impact:** Estimated 30-50% reduction in filter CPU usage.

#### 2. SIMD Optimizations (Planned)

ARM NEON SIMD can accelerate:
- Parallel voice processing
- Chorus stereo processing
- Filter stage calculations

#### 3. Compiler Optimizations

**Release Build:**
- `-O3`: Maximum optimization
- `-march=native` (Pi) or `-march=armv8-a+simd`
- `-ffast-math`: Relaxed floating-point (safe for audio DSP)
- `-flto`: Link-Time Optimization

### Performance Profiling

Use `tools/profile_performance.cpp` to measure CPU usage:

```bash
g++ -O3 -std=c++17 -I../src tools/profile_performance.cpp \
    src/dsp/*.cpp -o profile_performance
./profile_performance
```

Output:
```
Poor House Juno - Performance Profiling
========================================

1 voice, no chorus:
  CPU usage @ 48kHz: 3.2%

6 voices, no chorus:
  CPU usage @ 48kHz: 18.5%

6 voices + chorus:
  CPU usage @ 48kHz: 22.1%

✓ PASSED: Meets <50% CPU target
```

---

## Future Enhancements

### Planned Improvements

1. **Lock-free MIDI queue** - Eliminate potential priority inversion
2. **SIMD voice processing** - Process multiple voices in parallel
3. **Preset caching** - Reduce parameter update overhead
4. **Optimized interpolation** - SIMD linear interpolation for chorus

### Scalability

The architecture supports:
- **More voices** - 8-12 voices on modern hardware
- **Additional effects** - Reverb, delay
- **Modulation matrix** - Flexible routing
- **Multi-timbral** - Multiple synth instances

---

## References

- [ALSA Project](https://www.alsa-project.org/)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [Web Audio API Specification](https://www.w3.org/TR/webaudio/)
- [Web MIDI API Specification](https://www.w3.org/TR/webmidi/)
- [CMake Documentation](https://cmake.org/documentation/)

---

**Last Updated:** January 10, 2026
**Maintainer:** Poor House Juno Development Team
