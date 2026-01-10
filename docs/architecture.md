# Architecture Documentation

**Poor House Juno - System Architecture and Build System**

---

## Table of Contents

1. [Overview](#overview)
2. [Project Structure](#project-structure)
3. [Platform Abstraction](#platform-abstraction)
4. [Build System](#build-system)
5. [Web Platform (Emscripten)](#web-platform-emscripten)
6. [Raspberry Pi Platform (ALSA)](#raspberry-pi-platform-alsa)
7. [Audio Thread Architecture](#audio-thread-architecture)
8. [Parameter Management](#parameter-management)
9. [Testing Infrastructure](#testing-infrastructure)

---

## Overview

Poor House Juno is designed as a **dual-platform synthesizer** with a shared DSP core that runs identically on:

1. **Web (Browser):** Development and testing environment using WebAssembly
2. **Raspberry Pi 4:** Production target for standalone hardware synth

### Design Goals

- **Single Codebase:** DSP code is 100% shared between platforms
- **Bit-Accurate Output:** Both platforms produce identical audio
- **Fast Iteration:** Web platform enables rapid development without hardware
- **Production Ready:** Pi platform provides low-latency standalone operation

### Technology Stack

| Component | Web | Raspberry Pi | Shared |
|-----------|-----|--------------|--------|
| **DSP Core** | | | ✅ C++17 |
| **Build System** | CMake + Emscripten | CMake | ✅ |
| **Audio API** | Web Audio (AudioWorklet) | ALSA | ❌ |
| **MIDI API** | Web MIDI API | ALSA RawMIDI | ❌ |
| **Threading** | Web Workers | pthread | ❌ |
| **UI** | HTML5 + JavaScript | None (headless) | ❌ |

---

## Project Structure

```
poor-house-juno/
├── src/
│   ├── dsp/                    # Platform-agnostic DSP core
│   │   ├── types.h             # Common types and constants
│   │   ├── parameters.h        # Synth parameter definitions
│   │   ├── oscillator.cpp/h    # Simple sine oscillator
│   │   ├── dco.cpp/h          # Digitally Controlled Oscillator
│   │   ├── filter.cpp/h       # IR3109 4-pole ladder filter
│   │   ├── envelope.cpp/h     # ADSR envelope generator
│   │   ├── lfo.cpp/h          # Triangle LFO
│   │   ├── chorus.cpp/h       # BBD stereo chorus
│   │   ├── voice.cpp/h        # Per-voice synthesis
│   │   └── synth.cpp/h        # 6-voice polyphonic engine
│   │
│   └── platform/              # Platform-specific code
│       ├── pi/                # Raspberry Pi implementation
│       │   ├── main.cpp       # Entry point, setup, main loop
│       │   ├── audio_driver.cpp/h  # ALSA audio output
│       │   └── midi_driver.cpp/h   # ALSA MIDI input
│       │
│       └── web/               # Web/Emscripten implementation
│           ├── main.cpp       # WASM bindings (Embind)
│           └── audio_worklet.js    # AudioWorklet processor
│
├── web/                       # Web interface files
│   ├── index.html            # Main UI
│   ├── css/juno.css          # Styling
│   └── js/
│       ├── app.js            # Main application logic
│       ├── audio.js          # Web Audio integration
│       ├── keyboard.js       # Virtual MIDI keyboard
│       ├── midi.js           # Web MIDI API handler
│       ├── presets.js        # Preset management
│       └── ui.js             # UI controls and visualization
│
├── tests/                     # Unit test suite
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── test_oscillator.cpp
│   ├── test_filter.cpp
│   ├── test_envelope.cpp
│   ├── test_lfo.cpp
│   ├── test_voice.cpp
│   └── test_chorus.cpp
│
├── tools/                     # Analysis and comparison tools
│   ├── analyze_tal.py
│   ├── measure_filter.py
│   ├── measure_chorus.py
│   ├── generate_reference.py
│   └── requirements.txt
│
├── scripts/                   # Build and deployment scripts
│   ├── build_pi.sh
│   ├── build_web.sh
│   ├── deploy_pi.sh
│   ├── deploy_web.sh
│   └── setup_pi.sh
│
├── CMakeLists.txt            # Root CMake configuration
└── docs/                      # Documentation
    ├── architecture.md        # This file
    ├── dsp_design.md
    ├── juno106_analysis.md
    ├── filter_tuning.md
    ├── chorus_analysis.md
    ├── pi_setup.md
    ├── web_interface.md
    └── midi_cc_map.md
```

---

## Platform Abstraction

### Shared DSP Core

All DSP code in `src/dsp/` is **platform-agnostic**:

- Pure C++17 (no platform-specific APIs)
- No external dependencies (except STL)
- No file I/O or system calls
- Float-based processing (no SIMD intrinsics in core code)

**Design Pattern:**
```cpp
// DSP classes expose simple interfaces
class Synth {
public:
    void handleNoteOn(int midiNote, int velocity);
    void handleNoteOff(int midiNote);
    void process(Sample* outputLeft, Sample* outputRight, int numSamples);
    // ... parameter setters
};
```

Platform code is responsible for:
- Audio I/O (getting buffers to/from hardware)
- MIDI I/O (parsing MIDI messages, calling synth methods)
- Threading and scheduling
- UI (web only)

---

## Build System

### CMake Configuration

**Root `CMakeLists.txt`:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(PoorHouseJuno VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform selection: web, pi, or test
if(NOT DEFINED PLATFORM)
    set(PLATFORM "test" CACHE STRING "Target platform")
endif()

# DSP source files (shared)
set(DSP_SOURCES
    src/dsp/oscillator.cpp
    src/dsp/dco.cpp
    src/dsp/filter.cpp
    src/dsp/envelope.cpp
    src/dsp/lfo.cpp
    src/dsp/chorus.cpp
    src/dsp/voice.cpp
    src/dsp/synth.cpp
)

# Platform-specific builds
if(PLATFORM STREQUAL "web")
    add_subdirectory(web_build)
elseif(PLATFORM STREQUAL "pi")
    add_subdirectory(pi_build)
elseif(PLATFORM STREQUAL "test")
    add_subdirectory(tests)
endif()
```

### Build Targets

**1. Web Build (Emscripten):**
```bash
mkdir build-web
cd build-web
emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)
```

**Outputs:**
- `poor-house-juno.js` - JavaScript glue code
- `poor-house-juno.wasm` - WebAssembly binary
- Files copied to `web/` directory

**2. Raspberry Pi Build:**
```bash
mkdir build-pi
cd build-pi
cmake .. -DPLATFORM=pi -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Output:**
- `poor-house-juno` - Native ARM executable

**3. Unit Tests:**
```bash
mkdir build-test
cd build-test
cmake .. -DPLATFORM=test -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./tests/phj_tests
```

**Output:**
- `phj_tests` - Test executable (using Catch2)

### Makefile Convenience Targets

```makefile
.PHONY: web pi test serve clean

web:
	@mkdir -p build-web
	@cd build-web && emcmake cmake .. -DPLATFORM=web && emmake make -j8

pi:
	@mkdir -p build-pi
	@cd build-pi && cmake .. -DPLATFORM=pi && make -j8

test:
	@mkdir -p build-test
	@cd build-test && cmake .. -DPLATFORM=test -DBUILD_TESTS=ON && make -j8
	@./build-test/tests/phj_tests

serve:
	@cd web && python3 -m http.server 8000

clean:
	@rm -rf build-web build-pi build-test
```

---

## Web Platform (Emscripten)

### Emscripten Integration

**Embind Bindings (`src/platform/web/main.cpp`):**
```cpp
#include <emscripten/bind.h>
#include "synth.h"

using namespace emscripten;
using namespace phj;

// Expose Synth class to JavaScript
EMSCRIPTEN_BINDINGS(phj_synth) {
    class_<Synth>("Synth")
        .constructor<>()
        .function("setSampleRate", &Synth::setSampleRate)
        .function("handleNoteOn", &Synth::handleNoteOn)
        .function("handleNoteOff", &Synth::handleNoteOff)
        .function("handlePitchBend", &Synth::handlePitchBend)
        .function("handleControlChange", &Synth::handleControlChange)
        .function("process", &Synth::process, allow_raw_pointers())
        // ... parameter setters
        ;
}
```

**Memory Management:**
- Shared heap between WASM and JavaScript
- Audio buffers allocated in WASM memory
- JavaScript accesses via typed arrays (Float32Array)

### AudioWorklet Processor

**`web/audio_worklet.js`:**
```javascript
class JunoProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        // WASM synth instance initialized from main thread
        this.synth = null;
    }

    process(inputs, outputs, parameters) {
        if (!this.synth) return true;

        const outputLeft = outputs[0][0];   // Left channel
        const outputRight = outputs[0][1];  // Right channel
        const numSamples = outputLeft.length;

        // Allocate WASM memory for output
        const leftPtr = this.synth._malloc(numSamples * 4);
        const rightPtr = this.synth._malloc(numSamples * 4);

        // Call WASM process function
        this.synth.process(leftPtr, rightPtr, numSamples);

        // Copy results to output buffers
        const leftData = new Float32Array(this.synth.HEAPF32.buffer, 
                                          leftPtr, numSamples);
        const rightData = new Float32Array(this.synth.HEAPF32.buffer, 
                                           rightPtr, numSamples);

        outputLeft.set(leftData);
        outputRight.set(rightData);

        // Free WASM memory
        this.synth._free(leftPtr);
        this.synth._free(rightPtr);

        return true;
    }
}

registerProcessor('juno-processor', JunoProcessor);
```

**Advantages:**
- Runs in separate thread (no main thread blocking)
- Low latency (128-sample buffers typical)
- Direct WASM memory access (no copying overhead)

### Web Audio Context Setup

**`web/js/audio.js`:**
```javascript
class AudioEngine {
    async init() {
        // Create AudioContext
        this.audioContext = new AudioContext({ sampleRate: 48000 });

        // Load WASM module
        const wasmModule = await import('./poor-house-juno.js');
        await wasmModule.default();

        // Create synth instance
        this.synth = new wasmModule.Synth();
        this.synth.setSampleRate(this.audioContext.sampleRate);

        // Load AudioWorklet
        await this.audioContext.audioWorklet.addModule('audio_worklet.js');

        // Create processor node
        this.workletNode = new AudioWorkletNode(
            this.audioContext, 
            'juno-processor'
        );

        // Send synth instance to worklet
        this.workletNode.port.postMessage({ synth: this.synth });

        // Connect to output
        this.workletNode.connect(this.audioContext.destination);
    }
}
```

---

## Raspberry Pi Platform (ALSA)

### System Architecture

```
┌─────────────┐
│ Main Thread │ (UI, MIDI routing, parameter updates)
└──────┬──────┘
       │
       ├──→ ┌──────────────┐
       │    │ MIDI Thread  │ (reads ALSA MIDI, enqueues events)
       │    └──────────────┘
       │
       └──→ ┌──────────────┐
            │ Audio Thread │ (ALSA PCM, processes buffers, SCHED_FIFO)
            └──────────────┘
```

### Audio Driver (`src/platform/pi/audio_driver.cpp`)

**ALSA Setup:**
```cpp
class AudioDriver {
public:
    bool init(const std::string& deviceName, int sampleRate, int bufferSize) {
        // Open PCM device
        snd_pcm_open(&pcmHandle_, deviceName.c_str(), 
                     SND_PCM_STREAM_PLAYBACK, 0);

        // Set hardware parameters
        snd_pcm_hw_params_t* hwParams;
        snd_pcm_hw_params_malloc(&hwParams);
        snd_pcm_hw_params_any(pcmHandle_, hwParams);

        // Interleaved stereo, float format
        snd_pcm_hw_params_set_access(pcmHandle_, hwParams, 
                                      SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcmHandle_, hwParams, 
                                      SND_PCM_FORMAT_FLOAT_LE);
        snd_pcm_hw_params_set_channels(pcmHandle_, hwParams, 2);
        snd_pcm_hw_params_set_rate_near(pcmHandle_, hwParams, 
                                         &sampleRate, nullptr);
        snd_pcm_hw_params_set_buffer_size_near(pcmHandle_, hwParams, 
                                                &bufferSize);

        snd_pcm_hw_params(pcmHandle_, hwParams);
        snd_pcm_hw_params_free(hwParams);

        return true;
    }

    void audioCallback() {
        // Process audio from synth
        synth_->process(bufferLeft_, bufferRight_, bufferSize_);

        // Interleave samples
        for (int i = 0; i < bufferSize_; ++i) {
            interleavedBuffer_[i * 2 + 0] = bufferLeft_[i];
            interleavedBuffer_[i * 2 + 1] = bufferRight_[i];
        }

        // Write to ALSA
        int err = snd_pcm_writei(pcmHandle_, interleavedBuffer_, 
                                  bufferSize_);
        if (err < 0) {
            snd_pcm_recover(pcmHandle_, err, 0);
        }
    }
};
```

**Thread Configuration:**
```cpp
void* audioThreadFunc(void* arg) {
    // Set real-time priority
    struct sched_param param;
    param.sched_priority = 80;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    AudioDriver* driver = static_cast<AudioDriver*>(arg);

    while (driver->isRunning()) {
        driver->audioCallback();
    }

    return nullptr;
}
```

**Performance:**
- Sample Rate: 48000 Hz
- Buffer Size: 128 samples (2.67 ms latency)
- Thread Priority: `SCHED_FIFO` priority 80
- Target CPU: < 50%

### MIDI Driver (`src/platform/pi/midi_driver.cpp`)

**ALSA RawMIDI:**
```cpp
class MidiDriver {
public:
    bool init(const std::string& deviceName) {
        // Open MIDI input device
        snd_rawmidi_open(&inputHandle_, nullptr, 
                         deviceName.c_str(), SND_RAWMIDI_NONBLOCK);
        return true;
    }

    void poll() {
        unsigned char buffer[3];
        int bytesRead = snd_rawmidi_read(inputHandle_, buffer, sizeof(buffer));

        if (bytesRead == 3) {
            unsigned char status = buffer[0] & 0xF0;
            unsigned char channel = buffer[0] & 0x0F;

            switch (status) {
                case 0x90:  // Note On
                    if (buffer[2] > 0) {
                        synth_->handleNoteOn(buffer[1], buffer[2]);
                    } else {
                        synth_->handleNoteOff(buffer[1]);
                    }
                    break;

                case 0x80:  // Note Off
                    synth_->handleNoteOff(buffer[1]);
                    break;

                case 0xB0:  // Control Change
                    synth_->handleControlChange(buffer[1], buffer[2]);
                    break;

                case 0xE0:  // Pitch Bend
                    int pitchBend = (buffer[2] << 7) | buffer[1];
                    synth_->handlePitchBend(pitchBend);
                    break;
            }
        }
    }
};
```

---

## Audio Thread Architecture

### Lock-Free Parameter Updates

**Problem:** Audio thread must access parameters without blocking.

**Solution:** Atomic variables for simple parameters, lock-free ring buffer for complex updates.

**Example:**
```cpp
class Synth {
private:
    std::atomic<float> filterCutoff_;
    std::atomic<float> filterResonance_;
    // ... other atomic parameters

public:
    void setFilterCutoff(float cutoff) {
        filterCutoff_.store(cutoff, std::memory_order_relaxed);
    }

    void process(Sample* left, Sample* right, int numSamples) {
        // Read atomically in audio thread
        float cutoff = filterCutoff_.load(std::memory_order_relaxed);
        
        // Update filter parameters
        for (auto& voice : voices_) {
            voice.filter_.setParameters(cutoff, ...);
        }

        // Process audio...
    }
};
```

### Voice Management

**Polyphony:**
- 6 voices (matching Juno-106)
- Voice stealing when all voices active
- Prefer stealing voices in RELEASE stage

**Voice Allocation Modes (M16):**
1. **Oldest First** (default): Steal oldest active voice
2. **Newest First**: Steal most recently triggered voice
3. **Low-Note Priority**: Preserve lowest notes, steal highest
4. **High-Note Priority**: Preserve highest notes, steal lowest

---

## Parameter Management

### Parameter Structure

**`src/dsp/parameters.h`:**
```cpp
struct DcoParams {
    float sawLevel;        // 0.0 - 1.0
    float pulseLevel;      // 0.0 - 1.0
    float subLevel;        // 0.0 - 1.0
    float noiseLevel;      // 0.0 - 1.0
    float pulseWidth;      // 0.05 - 0.95
    float pwmDepth;        // 0.0 - 1.0
    float detune;          // -100 to +100 cents
    bool enableDrift;
    // ... more parameters
};

struct FilterParams {
    float cutoff;          // 0.0 - 1.0 (log scale)
    float resonance;       // 0.0 - 1.0
    float envAmount;       // -1.0 - 1.0 (bipolar)
    float lfoAmount;       // 0.0 - 1.0
    float keyTrack;        // 0.0, 0.5, 1.0 (Off/Half/Full)
    int hpfMode;           // 0-3 (Off/30Hz/60Hz/120Hz)
    // ... more parameters
};

// Similar for EnvelopeParams, etc.
```

### MIDI CC Mapping (M16)

**Full list in `docs/midi_cc_map.md`**, key mappings:

| CC # | Parameter | Range |
|------|-----------|-------|
| 1 | Mod Wheel (LFO Depth) | 0-127 |
| 64 | Sustain Pedal | 0/127 |
| 71 | Filter Resonance | 0-127 |
| 74 | Filter Cutoff | 0-127 |
| 75 | LFO Rate | 0-127 |
| 91 | Chorus Mode | 0-127 |

---

## Testing Infrastructure

### Unit Test Framework

**Catch2** - Header-only C++ test framework

**Build Configuration:**
```cmake
# tests/CMakeLists.txt
add_executable(phj_tests
    test_main.cpp
    test_oscillator.cpp
    test_filter.cpp
    test_envelope.cpp
    test_lfo.cpp
    test_voice.cpp
    test_chorus.cpp
    ${DSP_SOURCES}
)

target_link_libraries(phj_tests PRIVATE Catch2::Catch2)
```

### Test Structure

**Example:**
```cpp
#include <catch2/catch.hpp>
#include "filter.h"

TEST_CASE("Filter basic processing", "[filter]") {
    phj::Filter filter;
    filter.setSampleRate(48000.0f);

    SECTION("Filter attenuates high frequencies") {
        // Low frequency signal passes through
        float lowFreq = filter.process(1.0f);
        REQUIRE(lowFreq > 0.9f);

        // High frequency signal is attenuated
        // ... test code
    }
}
```

**Current Status:** 32/32 tests passing (237 assertions)

---

## Deployment

### Web Deployment (GitHub Pages)

**Script:** `scripts/deploy_web.sh`

```bash
#!/bin/bash
# Build web version
make web

# Create/update gh-pages branch
git checkout -B gh-pages
cp -r web/* .
git add -A
git commit -m "Deploy web interface"
git push -f origin gh-pages
git checkout main
```

**Live Demo:** https://parkredding.github.io/poor-house-juno/

### Raspberry Pi Deployment

**Script:** `scripts/deploy_pi.sh`

```bash
#!/bin/bash
PI_HOST=$1

# Cross-compile or build on Pi
make pi

# Copy to Pi
scp build-pi/poor-house-juno $PI_HOST:~/poor-house-juno/
ssh $PI_HOST "sudo systemctl restart poor-house-juno"
```

**Systemd Service:**
```ini
[Unit]
Description=Poor House Juno Synthesizer
After=sound.target

[Service]
Type=simple
User=pi
ExecStart=/home/pi/poor-house-juno/poor-house-juno
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

---

## Performance Metrics

### Web Platform

- **Sample Rate:** 48000 Hz
- **Buffer Size:** 128 samples (2.67 ms)
- **CPU Usage:** < 30% single core (typical)
- **Memory:** ~2 MB WASM heap

### Raspberry Pi 4

- **Sample Rate:** 48000 Hz
- **Buffer Size:** 128 samples (2.67 ms)
- **Latency:** ~5-10 ms round-trip
- **CPU Usage:** Target < 50%, measured ~35% with 6 voices + chorus
- **Memory:** ~10 MB RSS

---

## Future Enhancements

1. **Cross-Compilation:** Build Pi binaries from x86/x64 host
2. **Hardware UI:** Add support for physical controls (pots, buttons)
3. **Preset Banks:** SD card storage for 128-patch banks
4. **MIDI Clock:** Sync LFO to external MIDI clock
5. **Effects:** Reverb, delay, distortion

---

## References

- [Emscripten Documentation](https://emscripten.org/docs/)
- [ALSA Project](https://www.alsa-project.org/)
- [Web Audio API Specification](https://www.w3.org/TR/webaudio/)
- [Web MIDI API Specification](https://www.w3.org/TR/webmidi/)
- [Catch2 Testing Framework](https://github.com/catchorg/Catch2)

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
