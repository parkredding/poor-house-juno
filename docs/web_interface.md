# Poor House Juno - Web Interface Architecture

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Web Audio API Integration](#web-audio-api-integration)
3. [AudioWorklet Architecture](#audioworklet-architecture)
4. [WASM Memory Management](#wasm-memory-management)
5. [Preset System](#preset-system)
6. [Virtual Keyboard](#virtual-keyboard)
7. [MIDI Web API Integration](#midi-web-api-integration)
8. [UI Controls and Parameter Binding](#ui-controls-and-parameter-binding)
9. [Oscilloscope Visualization](#oscilloscope-visualization)
10. [Performance Monitoring](#performance-monitoring)

---

## Overview

The web interface provides a full-featured development and testing environment for Poor House Juno, running entirely in the browser using WebAssembly (WASM) and the Web Audio API.

**Key Technologies:**
- **Emscripten:** Compiles C++ DSP code to WebAssembly
- **Web Audio API:** Low-latency audio processing
- **AudioWorklet:** Real-time audio thread
- **Web MIDI API:** MIDI controller support
- **localStorage:** Preset storage

**File Structure:**
```
web/
├── index.html           # Main UI layout
├── css/
│   └── juno.css        # Juno-106 inspired styling
├── js/
│   ├── app.js          # Main application logic
│   ├── audio.js        # Web Audio setup
│   ├── midi.js         # MIDI handling
│   ├── ui.js           # UI event handlers
│   ├── presets.js      # Preset management
│   └── scope.js        # Oscilloscope visualization
├── audio_worklet.js    # AudioWorklet processor
└── phj.{js,wasm}       # Compiled WASM (generated)
```

---

## Web Audio API Integration

### Audio Context Setup

The Web Audio API provides low-latency audio processing in the browser.

**Initialization (`web/js/audio.js`):**

```javascript
// Create audio context with explicit sample rate
const audioContext = new AudioContext({
    sampleRate: 48000,
    latencyHint: 'interactive'  // Optimize for low latency
});

// Check if AudioContext is running
if (audioContext.state === 'suspended') {
    // User must interact with page to start audio
    await audioContext.resume();
}
```

**Browser Requirements:**
- Chrome/Edge 66+
- Firefox 76+
- Safari 14.1+
- Opera 53+

**Sample Rate:**
- Target: 48 kHz
- Fallback: Browser default (usually 44.1 kHz or 48 kHz)
- Automatically detected and used by WASM module

### Audio Graph

```
┌──────────────────┐
│  AudioWorklet    │
│  (WASM Synth)    │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  GainNode        │  (Master volume)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  AnalyserNode    │  (Oscilloscope data)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Destination     │  (Speakers/headphones)
└──────────────────┘
```

**Implementation:**

```javascript
// Create AudioWorklet node
await audioContext.audioWorklet.addModule('audio_worklet.js');
const synthNode = new AudioWorkletNode(audioContext, 'synth-processor', {
    numberOfInputs: 0,
    numberOfOutputs: 1,
    outputChannelCount: [2]  // Stereo
});

// Master volume
const gainNode = audioContext.createGain();
gainNode.gain.value = 0.7;

// Oscilloscope analyzer
const analyserNode = audioContext.createAnalyser();
analyserNode.fftSize = 2048;

// Connect graph
synthNode.connect(gainNode);
gainNode.connect(analyserNode);
analyserNode.connect(audioContext.destination);
```

---

## AudioWorklet Architecture

AudioWorklet runs audio processing in a separate high-priority thread, isolated from the main UI thread.

**File:** `web/audio_worklet.js`

### AudioWorklet Processor

```javascript
class SynthProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();

        // Port for receiving messages from main thread
        this.port.onmessage = this.handleMessage.bind(this);

        // Buffer pointers (will be allocated in WASM heap)
        this.leftPtr = 0;
        this.rightPtr = 0;
        this.bufferSize = 128;

        this.port.postMessage({ type: 'ready' });
    }

    // Audio processing callback (runs at sample rate)
    process(inputs, outputs, parameters) {
        if (!synthInstance) {
            // Not initialized yet, output silence
            return true;
        }

        const output = outputs[0];
        const leftChannel = output[0];
        const rightChannel = output[1];

        // Call WASM synth to fill buffers
        synthInstance.process(this.leftPtr, this.rightPtr, leftChannel.length);

        // Copy from WASM heap to output buffers
        const leftHeap = new Float32Array(
            wasmModule.HEAPF32.buffer,
            this.leftPtr,
            leftChannel.length
        );
        const rightHeap = new Float32Array(
            wasmModule.HEAPF32.buffer,
            this.rightPtr,
            rightChannel.length
        );

        leftChannel.set(leftHeap);
        rightChannel.set(rightHeap);

        return true;  // Keep processor alive
    }
}

registerProcessor('synth-processor', SynthProcessor);
```

### Main Thread Communication

**Sending Parameters:**

```javascript
// From main thread (app.js)
synthNode.port.postMessage({
    type: 'setFilterCutoff',
    data: 0.5  // Cutoff value 0.0 - 1.0
});
```

**Handling in AudioWorklet:**

```javascript
handleMessage(event) {
    const { type, data } = event.data;

    switch (type) {
        case 'setFilterCutoff':
            synthInstance.setFilterCutoff(data);
            break;

        case 'midi':
            if (data.length >= 3) {
                synthInstance.handleMidi(data[0], data[1], data[2]);
            }
            break;

        // ... other parameters
    }
}
```

**Why AudioWorklet?**
- Runs in separate real-time audio thread
- Lower latency than ScriptProcessorNode (deprecated)
- No main thread blocking
- Consistent timing (sample-accurate)

---

## WASM Memory Management

WebAssembly uses a linear memory model. Audio buffers must be allocated in WASM heap for zero-copy access.

### Memory Layout

```
WASM Heap Memory:
┌────────────────────────────────┐
│  WASM Internal Data            │  (C++ objects, stack)
├────────────────────────────────┤
│  Left Channel Buffer (512 B)   │  ← leftPtr
├────────────────────────────────┤
│  Right Channel Buffer (512 B)  │  ← rightPtr
├────────────────────────────────┤
│  Free Space                    │
└────────────────────────────────┘
```

### Buffer Allocation

**In AudioWorklet:**

```javascript
async initWasm(moduleData) {
    // Load WASM module
    const Module = await import('./synth-processor.js');
    wasmModule = await Module.default();

    // Create synth instance
    synthInstance = new wasmModule.WebSynth(sampleRate);

    // Allocate buffers in WASM heap
    this.leftPtr = wasmModule._malloc(this.bufferSize * 4);   // 4 bytes per float32
    this.rightPtr = wasmModule._malloc(this.bufferSize * 4);

    this.port.postMessage({ type: 'initialized' });
}
```

### Zero-Copy Audio Transfer

**Traditional Approach (slow):**
```javascript
// Copy from C++ → JS array → Web Audio buffer
const leftArray = synthInstance.getLeftBuffer();  // Copy #1
leftChannel.set(leftArray);                       // Copy #2
```

**Zero-Copy Approach (fast):**
```javascript
// Direct access to WASM heap
const leftHeap = new Float32Array(
    wasmModule.HEAPF32.buffer,  // Shared heap
    this.leftPtr,               // Pointer to buffer
    leftChannel.length          // Length
);

leftChannel.set(leftHeap);  // Single copy (unavoidable for Web Audio API)
```

**Performance:**
- Zero-copy saves ~0.5 ms per buffer (128 samples)
- Critical for low-latency audio

### Memory Safety

**Potential Issues:**
- Buffer overruns (writing beyond allocated memory)
- Dangling pointers (accessing freed memory)
- Memory leaks (not freeing allocated memory)

**Solutions:**
- Fixed buffer sizes (128 samples)
- Never free audio buffers (allocated once at init)
- Bounds checking in debug builds

---

## Preset System

Presets are stored in browser's localStorage for persistence across sessions.

**File:** `web/js/presets.js`

### Preset Structure

```javascript
const preset = {
    name: "String Bass",

    // DCO parameters
    dco: {
        sawLevel: 0.7,
        pulseLevel: 0.0,
        subLevel: 0.5,
        noiseLevel: 0.0,
        pulseWidth: 0.5,
        pwmDepth: 0.0,
        lfoTarget: 0,  // 0=Off, 1=Pitch, 2=PWM, 3=Both
        range: 0       // 0=16', 1=8', 2=4'
    },

    // Filter parameters
    filter: {
        cutoff: 0.3,
        resonance: 0.6,
        envAmount: 0.7,
        lfoAmount: 0.0,
        keyTrack: 2,   // 0=Off, 1=Half, 2=Full
        hpfMode: 0     // 0=Off, 1=30Hz, 2=60Hz, 3=120Hz
    },

    // Envelope parameters
    filterEnv: {
        attack: 0.01,
        decay: 0.3,
        sustain: 0.5,
        release: 0.8
    },

    ampEnv: {
        attack: 0.01,
        decay: 0.5,
        sustain: 0.7,
        release: 0.5
    },

    // LFO parameters
    lfo: {
        rate: 2.0,
        delay: 0.0
    },

    // Chorus
    chorus: {
        mode: 3  // 0=Off, 1=I, 2=II, 3=I+II
    },

    // Performance
    performance: {
        pitchBendRange: 2.0,
        portamentoTime: 0.0,
        modWheel: 0.0,
        vcaMode: 0,           // 0=ENV, 1=GATE
        filterEnvPolarity: 0  // 0=Normal, 1=Inverse
    }
};
```

### Saving Presets

```javascript
function savePreset(presetName) {
    // Get current synth parameters
    const preset = getCurrentParameters();
    preset.name = presetName;

    // Get existing presets from localStorage
    const presets = JSON.parse(localStorage.getItem('phj_presets') || '[]');

    // Add or update preset
    const existingIndex = presets.findIndex(p => p.name === presetName);
    if (existingIndex >= 0) {
        presets[existingIndex] = preset;
    } else {
        presets.push(preset);
    }

    // Save to localStorage
    localStorage.setItem('phj_presets', JSON.stringify(presets));

    // Update preset list UI
    updatePresetList();
}
```

### Loading Presets

```javascript
function loadPreset(presetName) {
    // Get presets from localStorage
    const presets = JSON.parse(localStorage.getItem('phj_presets') || '[]');
    const preset = presets.find(p => p.name === presetName);

    if (!preset) {
        console.error('Preset not found:', presetName);
        return;
    }

    // Apply all parameters
    applyPreset(preset);
}

function applyPreset(preset) {
    // DCO
    synthNode.port.postMessage({ type: 'setSawLevel', data: preset.dco.sawLevel });
    synthNode.port.postMessage({ type: 'setPulseLevel', data: preset.dco.pulseLevel });
    // ... (all other parameters)

    // Update UI controls to match
    updateUIFromPreset(preset);
}
```

### Factory Presets

```javascript
const FACTORY_PRESETS = [
    {
        name: "Init",
        dco: { sawLevel: 0.5, pulseLevel: 0, subLevel: 0, noiseLevel: 0, ... },
        filter: { cutoff: 0.5, resonance: 0, ... },
        // ...
    },
    {
        name: "String Bass",
        // ...
    },
    // ... more presets
];

// Load factory presets on first run
if (!localStorage.getItem('phj_presets')) {
    localStorage.setItem('phj_presets', JSON.stringify(FACTORY_PRESETS));
}
```

---

## Virtual Keyboard

The virtual keyboard allows mouse and computer keyboard input.

**File:** `web/js/app.js` (keyboard handling)

### Mouse Input

```javascript
// Create keyboard UI
const keyboard = document.getElementById('keyboard');

for (let i = 0; i < 25; i++) {  // 2 octaves (C3-C5)
    const key = document.createElement('div');
    const note = 48 + i;  // MIDI note number (C3 = 48)

    key.className = isBlackKey(note) ? 'key black' : 'key white';
    key.dataset.note = note;

    // Mouse events
    key.addEventListener('mousedown', () => {
        handleNoteOn(note, 127);  // Full velocity
        key.classList.add('active');
    });

    key.addEventListener('mouseup', () => {
        handleNoteOff(note);
        key.classList.remove('active');
    });

    // Prevent stuck notes when mouse leaves
    key.addEventListener('mouseleave', () => {
        if (key.classList.contains('active')) {
            handleNoteOff(note);
            key.classList.remove('active');
        }
    });

    keyboard.appendChild(key);
}
```

### Computer Keyboard Input

```javascript
// Map computer keys to MIDI notes
const keyMap = {
    'a': 48,  // C3
    'w': 49,  // C#3
    's': 50,  // D3
    'e': 51,  // D#3
    'd': 52,  // E3
    'f': 53,  // F3
    't': 54,  // F#3
    'g': 55,  // G3
    'y': 56,  // G#3
    'h': 57,  // A3
    'u': 58,  // A#3
    'j': 59,  // B3
    'k': 60,  // C4 (middle C)
    // ... more keys
};

let activeKeys = new Set();

document.addEventListener('keydown', (event) => {
    const key = event.key.toLowerCase();

    // Ignore if key is already pressed (prevent key repeat)
    if (activeKeys.has(key)) return;

    const note = keyMap[key];
    if (note !== undefined) {
        handleNoteOn(note, 100);  // Medium velocity
        activeKeys.add(key);

        // Highlight visual key
        const keyElement = document.querySelector(`[data-note="${note}"]`);
        if (keyElement) keyElement.classList.add('active');
    }
});

document.addEventListener('keyup', (event) => {
    const key = event.key.toLowerCase();
    activeKeys.delete(key);

    const note = keyMap[key];
    if (note !== undefined) {
        handleNoteOff(note);

        // Remove highlight
        const keyElement = document.querySelector(`[data-note="${note}"]`);
        if (keyElement) keyElement.classList.remove('active');
    }
});
```

### Note Handling

```javascript
function handleNoteOn(note, velocity) {
    // Send MIDI to WASM synth
    synthNode.port.postMessage({
        type: 'midi',
        data: [0x90, note, velocity]  // Note On, channel 0
    });
}

function handleNoteOff(note) {
    synthNode.port.postMessage({
        type: 'midi',
        data: [0x80, note, 0]  // Note Off, channel 0
    });
}
```

---

## MIDI Web API Integration

The Web MIDI API allows external MIDI controllers to control the synth.

**File:** `web/js/midi.js`

### MIDI Setup

```javascript
async function setupMIDI() {
    try {
        // Request MIDI access
        const midiAccess = await navigator.requestMIDIAccess();

        // List available MIDI inputs
        const inputs = Array.from(midiAccess.inputs.values());

        if (inputs.length === 0) {
            console.log('No MIDI devices found');
            return;
        }

        console.log('MIDI devices:', inputs.map(i => i.name));

        // Connect to all MIDI inputs
        inputs.forEach(input => {
            input.onmidimessage = handleMIDIMessage;
            console.log('Connected to:', input.name);
        });

        // Update UI
        document.getElementById('midi-status').textContent =
            `Connected: ${inputs[0].name}`;

    } catch (error) {
        console.error('MIDI access denied:', error);
    }
}
```

### MIDI Message Handling

```javascript
function handleMIDIMessage(event) {
    const [status, data1, data2] = event.data;

    // Note On (0x90-0x9F)
    if ((status & 0xF0) === 0x90 && data2 > 0) {
        handleNoteOn(data1, data2);
        updateMIDIIndicator();
    }

    // Note Off (0x80-0x8F) or Note On with velocity 0
    else if ((status & 0xF0) === 0x80 || ((status & 0xF0) === 0x90 && data2 === 0)) {
        handleNoteOff(data1);
    }

    // Pitch Bend (0xE0-0xEF)
    else if ((status & 0xF0) === 0xE0) {
        const bendValue = ((data2 << 7) | data1) - 8192;  // -8192 to +8191
        const normalizedBend = bendValue / 8192.0;  // -1.0 to +1.0

        synthNode.port.postMessage({
            type: 'midi',
            data: [status, data1, data2]
        });
    }

    // Control Change (0xB0-0xBF)
    else if ((status & 0xF0) === 0xB0) {
        const cc = data1;
        const value = data2 / 127.0;  // Normalize to 0.0-1.0

        // CC #1: Modulation Wheel
        if (cc === 1) {
            synthNode.port.postMessage({
                type: 'setModWheel',
                data: value
            });
            updateUIControl('modWheel', value);
        }

        // CC #64: Sustain Pedal (future M16)
        else if (cc === 64) {
            // Not yet implemented
        }
    }

    // Forward all MIDI to synth
    synthNode.port.postMessage({
        type: 'midi',
        data: [status, data1, data2]
    });
}
```

### MIDI Activity Indicator

```javascript
let midiActivityTimeout;

function updateMIDIIndicator() {
    const indicator = document.getElementById('midi-activity');
    indicator.classList.add('active');

    // Clear previous timeout
    clearTimeout(midiActivityTimeout);

    // Auto-hide after 100ms
    midiActivityTimeout = setTimeout(() => {
        indicator.classList.remove('active');
    }, 100);
}
```

---

## UI Controls and Parameter Binding

All synth parameters are bound to UI controls (sliders, knobs, switches).

**File:** `web/js/ui.js`

### Slider Binding

```javascript
// Filter Cutoff slider
const cutoffSlider = document.getElementById('filter-cutoff');
const cutoffValue = document.getElementById('cutoff-value');

cutoffSlider.addEventListener('input', (event) => {
    const value = parseFloat(event.target.value);

    // Update value display
    cutoffValue.textContent = Math.round(value * 100) + '%';

    // Send to synth
    synthNode.port.postMessage({
        type: 'setFilterCutoff',
        data: value
    });
});
```

### Rotary Knob (CSS Transform)

```javascript
// Create knob UI
function createKnob(containerId, label, min, max, defaultValue, callback) {
    const container = document.getElementById(containerId);

    const knob = document.createElement('div');
    knob.className = 'knob';

    const indicator = document.createElement('div');
    indicator.className = 'knob-indicator';
    knob.appendChild(indicator);

    let isDragging = false;
    let startY = 0;
    let startValue = defaultValue;

    knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = getCurrentValue();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        const delta = startY - e.clientY;
        const range = max - min;
        const newValue = clamp(startValue + (delta / 100) * range, min, max);

        setKnobValue(newValue);
        callback(newValue);
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });

    function setKnobValue(value) {
        const angle = ((value - min) / (max - min)) * 270 - 135;  // -135° to +135°
        indicator.style.transform = `rotate(${angle}deg)`;
    }

    setKnobValue(defaultValue);
    container.appendChild(knob);
}
```

### Switch Binding

```javascript
// Chorus mode switch (3-position)
const chorusModeSwitch = document.getElementById('chorus-mode');

chorusModeSwitch.addEventListener('change', (event) => {
    const mode = parseInt(event.target.value);  // 0, 1, 2, or 3

    synthNode.port.postMessage({
        type: 'setChorusMode',
        data: mode
    });
});
```

---

## Oscilloscope Visualization

Real-time waveform display using Canvas API and AnalyserNode.

**File:** `web/js/scope.js`

### Oscilloscope Implementation

```javascript
const canvas = document.getElementById('oscilloscope');
const ctx = canvas.getContext('2d');

// Set canvas size
canvas.width = 800;
canvas.height = 200;

// Create analyzer node (already connected in audio graph)
const bufferLength = analyserNode.frequencyBinCount;
const dataArray = new Uint8Array(bufferLength);

function drawOscilloscope() {
    requestAnimationFrame(drawOscilloscope);

    // Get time-domain data
    analyserNode.getByteTimeDomainData(dataArray);

    // Clear canvas
    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Draw grid
    ctx.strokeStyle = '#0f3460';
    ctx.lineWidth = 1;
    for (let i = 0; i < 5; i++) {
        const y = (canvas.height / 4) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
    }

    // Draw waveform
    ctx.strokeStyle = '#16db93';
    ctx.lineWidth = 2;
    ctx.beginPath();

    const sliceWidth = canvas.width / bufferLength;
    let x = 0;

    for (let i = 0; i < bufferLength; i++) {
        const v = dataArray[i] / 128.0;  // Normalize to 0-2
        const y = (v * canvas.height) / 2;

        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }

        x += sliceWidth;
    }

    ctx.stroke();
}

// Start animation loop
drawOscilloscope();
```

---

## Performance Monitoring

### CPU Usage Estimation

```javascript
let lastCallTime = performance.now();
let cpuUsage = 0;

function estimateCPUUsage() {
    const currentTime = performance.now();
    const deltaTime = currentTime - lastCallTime;
    lastCallTime = currentTime;

    // Expected time for 128 samples @ 48 kHz = 2.67 ms
    const expectedTime = (128 / 48000) * 1000;

    // CPU usage = actual time / expected time
    cpuUsage = (deltaTime / expectedTime) * 100;

    // Display
    document.getElementById('cpu-usage').textContent =
        `CPU: ${cpuUsage.toFixed(1)}%`;
}

// Call periodically
setInterval(estimateCPUUsage, 1000);
```

### Voice Activity Indicator

```javascript
// Query active voices from synth (if exposed)
function updateVoiceIndicator() {
    // Would need to expose voice count from WASM
    // For now, approximate based on MIDI activity

    const activeVoices = getActiveVoiceCount();  // Implementation-specific
    document.getElementById('voice-count').textContent =
        `Voices: ${activeVoices}/6`;
}
```

---

## Browser Compatibility

### Feature Detection

```javascript
// Check for required APIs
function checkBrowserSupport() {
    const errors = [];

    if (!window.AudioContext && !window.webkitAudioContext) {
        errors.push('Web Audio API not supported');
    }

    if (!window.AudioWorklet) {
        errors.push('AudioWorklet not supported (use Chrome 66+, Firefox 76+, or Safari 14.1+)');
    }

    if (!navigator.requestMIDIAccess) {
        console.warn('Web MIDI API not supported (MIDI controllers will not work)');
        // Not a critical error - virtual keyboard still works
    }

    if (errors.length > 0) {
        alert('Browser not supported:\n' + errors.join('\n'));
        return false;
    }

    return true;
}

// Call on page load
if (!checkBrowserSupport()) {
    // Disable UI or show error message
}
```

---

## References

- [Web Audio API Specification](https://www.w3.org/TR/webaudio/)
- [AudioWorklet Documentation](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet)
- [Web MIDI API Specification](https://www.w3.org/TR/webmidi/)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly Documentation](https://webassembly.org/)

---

**Last Updated:** January 10, 2026
