# Web Interface Documentation

**Poor House Juno - WebAssembly and Web Audio Implementation**

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [WebAssembly Integration](#webassembly-integration)
4. [AudioWorklet Processing](#audioworklet-processing)
5. [Web MIDI API](#web-midi-api)
6. [User Interface](#user-interface)
7. [Preset Management](#preset-management)
8. [Build Process](#build-process)
9. [Deployment](#deployment)
10. [Browser Compatibility](#browser-compatibility)

---

## Overview

The Poor House Juno web interface provides a **full-featured development and testing environment** that runs entirely in the browser. It uses the same DSP code as the Raspberry Pi version, ensuring bit-accurate output across platforms.

### Key Technologies

- **WebAssembly (WASM):** Compiled C++ DSP code runs at near-native speed
- **Emscripten:** C++ to WebAssembly compiler with JavaScript bindings
- **AudioWorklet:** Low-latency audio processing in separate thread
- **Web MIDI API:** MIDI input from hardware controllers
- **HTML5 Canvas:** Real-time oscilloscope visualization
- **LocalStorage:** Client-side preset storage

### Features

✅ **Full Synthesis Engine:** Complete 6-voice Juno-106 emulation
✅ **Virtual Keyboard:** Play with mouse or computer keyboard
✅ **MIDI Support:** Connect hardware MIDI controllers
✅ **Real-Time Visualization:** Oscilloscope and CPU meter
✅ **Preset Management:** Save/load/delete custom patches
✅ **Parameter Control:** All synth parameters accessible via UI
✅ **Stereo Output:** Full stereo chorus effect
✅ **Low Latency:** Typically < 10ms with AudioWorklet

---

## Architecture

### System Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        Main Thread                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │   UI     │  │  WASM    │  │  MIDI    │  │ Presets  │  │
│  │ Controls │◄─┤  Bindings│◄─┤  Input   │  │ Storage  │  │
│  └────┬─────┘  └────┬─────┘  └──────────┘  └──────────┘  │
│       │             │                                       │
│       └─────────────┴──────────┐                           │
│                                 │                           │
└─────────────────────────────────┼───────────────────────────┘
                                  │ postMessage
                                  ↓
┌─────────────────────────────────────────────────────────────┐
│                     AudioWorklet Thread                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Juno Processor (audio_worklet.js)            │  │
│  │  ┌────────────────────────────────────────────────┐  │  │
│  │  │  WASM Synth Instance (synth.process())        │  │  │
│  │  │  - 6 voices                                    │  │  │
│  │  │  - BBD chorus                                  │  │  │
│  │  │  - Real-time DSP                               │  │  │
│  │  └────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
│                              │                              │
└──────────────────────────────┼──────────────────────────────┘
                               ↓
                      ┌──────────────────┐
                      │ Audio Destination│
                      │   (Speakers)     │
                      └──────────────────┘
```

### File Structure

```
web/
├── index.html              # Main HTML page
├── css/
│   └── juno.css           # Styling (panel, knobs, switches)
├── js/
│   ├── app.js             # Main application logic
│   ├── audio.js           # AudioContext and AudioWorklet setup
│   ├── keyboard.js        # Virtual MIDI keyboard
│   ├── midi.js            # Web MIDI API handler
│   ├── presets.js         # Preset management (localStorage)
│   └── ui.js              # UI controls and visualization
├── audio_worklet.js       # AudioWorklet processor
├── poor-house-juno.js     # Emscripten-generated JS glue code
└── poor-house-juno.wasm   # Compiled WebAssembly binary
```

---

## WebAssembly Integration

### Emscripten Compilation

**Build Configuration (`CMakeLists.txt`):**
```cmake
if(PLATFORM STREQUAL "web")
    add_executable(poor-house-juno ${DSP_SOURCES} ${WEB_SOURCES})
    
    set_target_properties(poor-house-juno PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s EXPORTED_RUNTIME_METHODS=['cwrap','ccall'] \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME='PoorHouseJuno' \
                    --bind"
    )
endif()
```

**Emscripten Flags Explained:**
- `-s WASM=1`: Generate WebAssembly (not asm.js)
- `-s ALLOW_MEMORY_GROWTH=1`: Dynamic heap size
- `-s EXPORTED_RUNTIME_METHODS`: Export helper functions
- `-s MODULARIZE=1`: ES6 module format
- `-s EXPORT_NAME='PoorHouseJuno'`: Module name
- `--bind`: Enable Embind (C++ ↔ JS bindings)

### Embind Bindings

**C++ Side (`src/platform/web/main.cpp`):**
```cpp
#include <emscripten/bind.h>
#include "synth.h"

using namespace emscripten;
using namespace phj;

EMSCRIPTEN_BINDINGS(phj_synth) {
    class_<Synth>("Synth")
        .constructor<>()
        .function("setSampleRate", &Synth::setSampleRate)
        .function("handleNoteOn", &Synth::handleNoteOn)
        .function("handleNoteOff", &Synth::handleNoteOff)
        .function("handlePitchBend", &Synth::handlePitchBend)
        .function("handleControlChange", &Synth::handleControlChange)
        .function("process", &Synth::process, allow_raw_pointers())
        .function("setFilterCutoff", &Synth::setFilterCutoff)
        .function("setFilterResonance", &Synth::setFilterResonance)
        // ... all parameter setters
        ;
}
```

**JavaScript Side (`web/js/audio.js`):**
```javascript
// Load WASM module
const PoorHouseJunoModule = await import('./poor-house-juno.js');
const Module = await PoorHouseJunoModule.default();

// Create synth instance
this.synth = new Module.Synth();
this.synth.setSampleRate(this.audioContext.sampleRate);

// Call methods
this.synth.handleNoteOn(60, 100);  // Middle C, velocity 100
this.synth.setFilterCutoff(0.5);   // 50% cutoff
```

### Memory Management

**WASM Heap Layout:**
```
┌────────────────────────────────────┐
│   Stack (grows down)               │
│                                    │
├────────────────────────────────────┤
│   Static Data (globals)            │
├────────────────────────────────────┤
│   Heap (grows up)                  │
│   - Synth instance                 │
│   - Audio buffers                  │
│   - Voice state                    │
└────────────────────────────────────┘
```

**Allocating Audio Buffers:**
```javascript
const numSamples = 128;
const bytesPerSample = 4;  // float32

// Allocate in WASM heap
const leftPtr = Module._malloc(numSamples * bytesPerSample);
const rightPtr = Module._malloc(numSamples * bytesPerSample);

// Process audio
this.synth.process(leftPtr, rightPtr, numSamples);

// Access as typed array
const leftData = new Float32Array(
    Module.HEAPF32.buffer,
    leftPtr,
    numSamples
);

// Copy to output
outputLeft.set(leftData);

// Free memory
Module._free(leftPtr);
Module._free(rightPtr);
```

---

## AudioWorklet Processing

### AudioWorklet vs ScriptProcessor

**Old: ScriptProcessorNode (deprecated)**
- Runs in main thread (UI blocking)
- Higher latency
- Less reliable timing

**New: AudioWorkletNode (since Chrome 66)**
- Runs in dedicated audio thread
- Lower latency (~128 samples typical)
- Precise timing
- No main thread blocking

### AudioWorklet Setup

**1. Load AudioWorklet Module (`audio.js`):**
```javascript
async initAudio() {
    // Create AudioContext
    this.audioContext = new AudioContext({ sampleRate: 48000 });
    
    // Load WASM module
    const Module = await this.loadWasmModule();
    this.synth = new Module.Synth();
    this.synth.setSampleRate(this.audioContext.sampleRate);
    
    // Register AudioWorklet processor
    await this.audioContext.audioWorklet.addModule('audio_worklet.js');
    
    // Create worklet node
    this.workletNode = new AudioWorkletNode(
        this.audioContext,
        'juno-processor',
        {
            numberOfInputs: 0,
            numberOfOutputs: 1,
            outputChannelCount: [2]  // Stereo
        }
    );
    
    // Connect to output
    this.workletNode.connect(this.audioContext.destination);
    
    // Pass synth instance to worklet
    this.workletNode.port.postMessage({
        type: 'init',
        synth: this.synth,
        sampleRate: this.audioContext.sampleRate
    });
}
```

**2. AudioWorklet Processor (`audio_worklet.js`):**
```javascript
class JunoProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.synth = null;
        
        // Listen for messages from main thread
        this.port.onmessage = (e) => {
            if (e.data.type === 'init') {
                this.synth = e.data.synth;
            }
        };
    }
    
    process(inputs, outputs, parameters) {
        if (!this.synth) return true;
        
        // Get output buffers
        const outputLeft = outputs[0][0];   // Left channel
        const outputRight = outputs[0][1];  // Right channel
        const numSamples = outputLeft.length;
        
        // Allocate WASM memory
        const Module = this.synth.constructor.prototype.constructor.Module;
        const leftPtr = Module._malloc(numSamples * 4);
        const rightPtr = Module._malloc(numSamples * 4);
        
        // Process audio in WASM
        this.synth.process(leftPtr, rightPtr, numSamples);
        
        // Copy results to output
        const leftData = new Float32Array(
            Module.HEAPF32.buffer,
            leftPtr,
            numSamples
        );
        const rightData = new Float32Array(
            Module.HEAPF32.buffer,
            rightPtr,
            numSamples
        );
        
        outputLeft.set(leftData);
        outputRight.set(rightData);
        
        // Free WASM memory
        Module._free(leftPtr);
        Module._free(rightPtr);
        
        return true;  // Keep processor alive
    }
}

registerProcessor('juno-processor', JunoProcessor);
```

### Latency Analysis

**Total Latency = Input Delay + Processing Delay + Output Delay**

**Typical Values:**
- **Input Delay:** ~128 samples (2.67 ms @ 48 kHz)
- **Processing Delay:** < 1 sample (negligible with WASM)
- **Output Delay:** ~128 samples (2.67 ms @ 48 kHz)
- **Total:** ~5-6 ms (acceptable for real-time performance)

**Comparison:**
- ScriptProcessor: 15-30 ms (noticeable lag)
- AudioWorklet: 5-10 ms (feels immediate)
- Native Plugin: 2-5 ms (ideal)

---

## Web MIDI API

### MIDI Input Setup

**Request MIDI Access (`midi.js`):**
```javascript
class MidiHandler {
    async init() {
        try {
            this.midiAccess = await navigator.requestMIDIAccess();
            this.setupMidiInputs();
        } catch (err) {
            console.warn('Web MIDI not available:', err);
        }
    }
    
    setupMidiInputs() {
        // List all MIDI inputs
        for (const input of this.midiAccess.inputs.values()) {
            console.log('MIDI Input:', input.name);
            input.onmidimessage = (e) => this.handleMidiMessage(e);
        }
        
        // Listen for device connections
        this.midiAccess.onstatechange = (e) => {
            if (e.port.type === 'input') {
                if (e.port.state === 'connected') {
                    console.log('MIDI device connected:', e.port.name);
                    e.port.onmidimessage = (e) => this.handleMidiMessage(e);
                }
            }
        };
    }
    
    handleMidiMessage(event) {
        const [status, data1, data2] = event.data;
        const command = status & 0xF0;
        const channel = status & 0x0F;
        
        switch (command) {
            case 0x90:  // Note On
                if (data2 > 0) {
                    this.synth.handleNoteOn(data1, data2);
                    this.ui.highlightKey(data1);
                } else {
                    // Velocity 0 = Note Off
                    this.synth.handleNoteOff(data1);
                    this.ui.unhighlightKey(data1);
                }
                break;
                
            case 0x80:  // Note Off
                this.synth.handleNoteOff(data1);
                this.ui.unhighlightKey(data1);
                break;
                
            case 0xB0:  // Control Change
                this.synth.handleControlChange(data1, data2);
                this.ui.updateControlFromMidi(data1, data2);
                break;
                
            case 0xE0:  // Pitch Bend
                const pitchBend = (data2 << 7) | data1;  // 14-bit value
                this.synth.handlePitchBend(pitchBend);
                break;
        }
    }
}
```

### Browser Compatibility

**Web MIDI API Support:**
- ✅ Chrome/Edge (full support)
- ✅ Opera (full support)
- ❌ Firefox (requires flag: `dom.webmidi.enabled`)
- ❌ Safari (not supported)

**Fallback:**
- Use virtual keyboard (mouse/computer keyboard)
- Display warning if MIDI not available

---

## User Interface

### Control Panel Layout

```
┌────────────────────────────────────────────────────────────┐
│  Poor House Juno                              [Start Audio]│
├────────────────────────────────────────────────────────────┤
│  LFO        DCO                 VCF              VCA        │
│  [Rate]    [Saw] [Pulse]       [Cutoff]        [Level]    │
│  [Delay]   [Sub] [Noise]       [Resonance]     [Env]      │
│            [PWM] [PW]           [Env] [LFO]                │
│                                 [Key]                       │
├────────────────────────────────────────────────────────────┤
│  Envelope (Filter)         Envelope (Amp)                  │
│  [A] [D] [S] [R]          [A] [D] [S] [R]                 │
├────────────────────────────────────────────────────────────┤
│  Chorus: [Off] [I] [II] [I+II]     HPF: [Off] [1] [2] [3]│
├────────────────────────────────────────────────────────────┤
│  Oscilloscope                   CPU: 35% | Voices: 3/6    │
│  [Waveform display]                                        │
├────────────────────────────────────────────────────────────┤
│  Virtual Keyboard                                          │
│  [C] [D] [E] [F] [G] [A] [B] [C] ...                     │
├────────────────────────────────────────────────────────────┤
│  Presets: [Dropdown ▼] [Save] [Load] [Delete]            │
└────────────────────────────────────────────────────────────┘
```

### UI Controls Implementation

**Slider/Knob (`ui.js`):**
```javascript
class SliderControl {
    constructor(element, parameter, min, max, synth) {
        this.element = element;
        this.parameter = parameter;
        this.min = min;
        this.max = max;
        this.synth = synth;
        
        this.element.addEventListener('input', (e) => {
            const normalized = (e.target.value - this.min) / (this.max - this.min);
            this.updateSynth(normalized);
        });
    }
    
    updateSynth(value) {
        // Map to parameter setter
        switch (this.parameter) {
            case 'filterCutoff':
                this.synth.setFilterCutoff(value);
                break;
            case 'filterResonance':
                this.synth.setFilterResonance(value);
                break;
            // ... all parameters
        }
    }
}
```

**Virtual Keyboard (`keyboard.js`):**
```javascript
class VirtualKeyboard {
    constructor(canvas, synth) {
        this.canvas = canvas;
        this.synth = synth;
        this.activeNotes = new Set();
        
        this.canvas.addEventListener('mousedown', (e) => this.handleMouseDown(e));
        this.canvas.addEventListener('mouseup', (e) => this.handleMouseUp(e));
        
        // Computer keyboard mapping
        document.addEventListener('keydown', (e) => this.handleKeyDown(e));
        document.addEventListener('keyup', (e) => this.handleKeyUp(e));
    }
    
    handleMouseDown(e) {
        const note = this.pixelToNote(e.offsetX, e.offsetY);
        if (note >= 0 && !this.activeNotes.has(note)) {
            this.synth.handleNoteOn(note, 100);
            this.activeNotes.add(note);
            this.draw();
        }
    }
    
    handleKeyDown(e) {
        // Map keyboard keys to MIDI notes
        const keyMap = {
            'a': 60, 's': 62, 'd': 64, 'f': 65,  // C, D, E, F
            'g': 67, 'h': 69, 'j': 71, 'k': 72   // G, A, B, C
        };
        
        const note = keyMap[e.key];
        if (note && !this.activeNotes.has(note)) {
            this.synth.handleNoteOn(note, 100);
            this.activeNotes.add(note);
            this.draw();
        }
    }
}
```

### Visualization

**Oscilloscope (`ui.js`):**
```javascript
class Oscilloscope {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.buffer = new Float32Array(1024);
        this.writeIndex = 0;
    }
    
    addSamples(samples) {
        for (let i = 0; i < samples.length; ++i) {
            this.buffer[this.writeIndex] = samples[i];
            this.writeIndex = (this.writeIndex + 1) % this.buffer.length;
        }
    }
    
    draw() {
        const ctx = this.ctx;
        const width = this.canvas.width;
        const height = this.canvas.height;
        
        // Clear
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, width, height);
        
        // Draw waveform
        ctx.strokeStyle = '#0f0';
        ctx.lineWidth = 2;
        ctx.beginPath();
        
        for (let i = 0; i < width; ++i) {
            const bufferIndex = Math.floor(i / width * this.buffer.length);
            const sample = this.buffer[bufferIndex];
            const y = height / 2 - sample * height / 2;
            
            if (i === 0) {
                ctx.moveTo(i, y);
            } else {
                ctx.lineTo(i, y);
            }
        }
        
        ctx.stroke();
    }
}
```

---

## Preset Management

### LocalStorage Structure

**Data Format:**
```javascript
{
    "presets": [
        {
            "name": "Lush Pad",
            "params": {
                "sawLevel": 0.8,
                "pulseLevel": 0.5,
                "subLevel": 0.3,
                "filterCutoff": 0.6,
                "filterResonance": 0.4,
                // ... all parameters
            }
        },
        {
            "name": "Brass Stab",
            "params": { /* ... */ }
        }
    ]
}
```

### Preset Manager (`presets.js`)

```javascript
class PresetManager {
    constructor(synth) {
        this.synth = synth;
        this.loadPresets();
    }
    
    loadPresets() {
        const data = localStorage.getItem('phj_presets');
        this.presets = data ? JSON.parse(data) : [];
    }
    
    savePresets() {
        localStorage.setItem('phj_presets', JSON.stringify(this.presets));
    }
    
    savePreset(name) {
        const params = this.synth.getAllParameters();
        this.presets.push({ name, params });
        this.savePresets();
    }
    
    loadPreset(index) {
        if (index >= 0 && index < this.presets.length) {
            const preset = this.presets[index];
            this.synth.setAllParameters(preset.params);
        }
    }
    
    deletePreset(index) {
        this.presets.splice(index, 1);
        this.savePresets();
    }
}
```

---

## Build Process

**Build Command:**
```bash
make web

# Or manually:
mkdir build-web
cd build-web
emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)
```

**Output Files:**
- `poor-house-juno.js` (~200 KB)
- `poor-house-juno.wasm` (~300 KB)

**Optimization Flags:**
```cmake
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
```

---

## Deployment

### GitHub Pages

**Deploy Script (`scripts/deploy_web.sh`):**
```bash
#!/bin/bash
make web
git checkout -B gh-pages
cp -r web/* .
git add -A
git commit -m "Deploy"
git push -f origin gh-pages
git checkout main
```

**Live Demo:** https://parkredding.github.io/poor-house-juno/

---

## Browser Compatibility

| Browser | WASM | AudioWorklet | Web MIDI | Status |
|---------|------|--------------|----------|--------|
| **Chrome 88+** | ✅ | ✅ | ✅ | ✅ Full |
| **Edge 88+** | ✅ | ✅ | ✅ | ✅ Full |
| **Firefox 89+** | ✅ | ✅ | ⚠️ Flag | ⚠️ Partial |
| **Safari 14+** | ✅ | ✅ | ❌ | ⚠️ No MIDI |
| **Mobile Chrome** | ✅ | ✅ | ❌ | ⚠️ No MIDI |

**Recommended:** Chrome or Edge for full features.

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
