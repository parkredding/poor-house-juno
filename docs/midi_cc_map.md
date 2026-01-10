# Poor House Juno - MIDI CC Mapping Reference

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Current MIDI Implementation](#current-midi-implementation)
3. [Pitch Bend](#pitch-bend)
4. [Modulation Wheel (CC #1)](#modulation-wheel-cc-1)
5. [Future CC Mappings (M16)](#future-cc-mappings-m16)
6. [MIDI Message Format](#midi-message-format)
7. [Implementation Details](#implementation-details)

---

## Overview

Poor House Juno implements MIDI input for note control and performance parameters. This document describes the current MIDI implementation and planned future enhancements.

**Current Status (M15):**
- ✅ Note On/Off (all channels)
- ✅ Pitch Bend (configurable range)
- ✅ Modulation Wheel (CC #1)
- ⏳ Sustain Pedal (CC #64) - Planned M16
- ⏳ Full CC mapping - Planned M16

**MIDI Channels:**
- All channels supported (omni mode)
- Channel filtering not implemented

---

## Current MIDI Implementation

### MIDI Messages Supported (M15)

| Message Type    | Status Byte | Data Bytes | Function                  | Status |
|-----------------|-------------|------------|---------------------------|--------|
| Note On         | 0x90-0x9F   | Note, Vel  | Trigger voice             | ✅     |
| Note Off        | 0x80-0x8F   | Note, Vel  | Release voice             | ✅     |
| Pitch Bend      | 0xE0-0xEF   | LSB, MSB   | Pitch bend wheel          | ✅     |
| Control Change  | 0xB0-0xBF   | CC, Value  | Mod wheel (CC #1)         | ✅     |

### Note On/Off

**MIDI Message Format:**

```
Note On:  0x90 <note> <velocity>
Note Off: 0x80 <note> <velocity>

Note On with velocity 0 is also interpreted as Note Off
```

**Parameters:**
- **Note:** 0-127 (MIDI note number)
  - 0 = C-1 (very low, 8.176 Hz)
  - 60 = C4 (middle C, 261.63 Hz)
  - 127 = G9 (very high, 12,543.85 Hz)
- **Velocity:** 0-127
  - 0 = Note Off (minimum)
  - 1 = Softest
  - 127 = Hardest

**Velocity Behavior (M14):**
- Velocity affects filter cutoff (if velocity-to-filter enabled)
- Velocity affects amplitude (if velocity-to-amp enabled)
- Amount controlled by synth parameters (0.0-1.0)

**Implementation:**

```cpp
// In Synth::handleMidi()
if ((status & 0xF0) == 0x90 && data2 > 0) {
    // Note On
    handleNoteOn(data1, data2 / 127.0f);  // Convert to 0.0-1.0
} else if ((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0)) {
    // Note Off
    handleNoteOff(data1);
}
```

---

## Pitch Bend

**MIDI Message Format:**

```
Pitch Bend: 0xE0 <LSB> <MSB>

14-bit value:
  value = (MSB << 7) | LSB
  range: 0 - 16383
  center: 8192
```

**Normalization:**

```cpp
// Convert 14-bit value to -1.0 to +1.0
int bendValue = ((data2 << 7) | data1) - 8192;  // -8192 to +8191
float normalizedBend = bendValue / 8192.0f;     // -1.0 to +1.0
```

**Pitch Bend Range:**

The range is configurable (M11):

```cpp
// Default: ±2 semitones
performanceParams_.pitchBendRange = 2.0f;

// Range can be set from ±1 to ±12 semitones
```

**Conversion to Frequency:**

```cpp
// In Voice::process()
float pitchBendSemitones = pitchBend_ * pitchBendRange_;  // e.g., -2.0 to +2.0
float pitchBendRatio = pow(2.0f, pitchBendSemitones / 12.0f);
float finalFreq = currentFreq_ * pitchBendRatio;
```

**Examples:**

| Wheel Position | 14-bit Value | Normalized | Semitones (±2) | Frequency |
|----------------|--------------|------------|----------------|-----------|
| Down           | 0            | -1.0       | -2.0           | 0.891×    |
| Center         | 8192         | 0.0        | 0.0            | 1.0×      |
| Up             | 16383        | +1.0       | +2.0           | 1.122×    |

---

## Modulation Wheel (CC #1)

**MIDI Message Format:**

```
Control Change: 0xB0 <CC #1> <value>

value: 0-127
```

**Normalization:**

```cpp
// Convert 0-127 to 0.0-1.0
float modWheel = data2 / 127.0f;
```

**Function (M13):**

The modulation wheel controls **global LFO depth**:

```cpp
// In Synth::processStereo()
float lfoValue = lfo_.process();               // -1.0 to +1.0
float modulatedLfo = lfoValue * modWheel_;    // Scaled by mod wheel

// Apply to all voices
for (int i = 0; i < NUM_VOICES; ++i) {
    voices_[i].setLfoValue(modulatedLfo);
}
```

**Behavior:**

| Mod Wheel | LFO Depth | Effect                    |
|-----------|-----------|---------------------------|
| 0         | 0%        | No LFO (even if enabled)  |
| 64        | 50%       | Half LFO depth            |
| 127       | 100%      | Full LFO depth            |

**Use Cases:**
- Start with wheel down (no vibrato)
- Gradually increase for expression
- Classic performance technique

---

## Future CC Mappings (M16)

### Planned CC Assignments

**Standard MIDI CCs:**

| CC #  | Parameter            | Range      | Function                        | Priority |
|-------|----------------------|------------|---------------------------------|----------|
| 1     | Modulation Wheel     | 0-127      | LFO depth                       | ✅ Done  |
| 64    | Sustain Pedal        | 0-127      | Hold notes (>63 = on)           | 🔴 High  |
| 5     | Portamento Time      | 0-127      | Glide time (0-10 seconds)       | 🟡 Med   |
| 74    | Filter Cutoff        | 0-127      | VCF cutoff frequency            | 🔴 High  |
| 71    | Filter Resonance     | 0-127      | VCF resonance                   | 🔴 High  |
| 73    | Filter Env Amount    | 0-127      | VCF envelope modulation         | 🔴 High  |
| 75    | Filter LFO Amount    | 0-127      | VCF LFO modulation              | 🟡 Med   |
| 72    | Filter Release       | 0-127      | VCF envelope release time       | 🟢 Low   |
| 76    | Filter Attack        | 0-127      | VCF envelope attack time        | 🟢 Low   |
| 77    | Filter Decay         | 0-127      | VCF envelope decay time         | 🟢 Low   |
| 78    | Filter Sustain       | 0-127      | VCF envelope sustain level      | 🟢 Low   |
| 7     | Volume               | 0-127      | Master volume                   | 🟡 Med   |
| 10    | Pan                  | 0-127      | Stereo pan (N/A for Juno)       | -        |

**Custom/Extended CCs:**

| CC #  | Parameter            | Range      | Function                        | Priority |
|-------|----------------------|------------|---------------------------------|----------|
| 102   | LFO Rate             | 0-127      | LFO frequency (0.1-30 Hz)       | 🟡 Med   |
| 103   | LFO Delay            | 0-127      | LFO delay time (0-3 seconds)    | 🟢 Low   |
| 104   | Chorus Mode          | 0-127      | 0-31=Off, 32-63=I, 64-95=II, 96-127=I+II | 🟡 Med |
| 105   | DCO Saw Level        | 0-127      | Sawtooth level                  | 🟢 Low   |
| 106   | DCO Pulse Level      | 0-127      | Pulse level                     | 🟢 Low   |
| 107   | DCO Sub Level        | 0-127      | Sub-oscillator level            | 🟢 Low   |
| 108   | DCO Noise Level      | 0-127      | Noise level                     | 🟢 Low   |
| 109   | DCO Pulse Width      | 0-127      | Pulse width (5-95%)             | 🟢 Low   |
| 110   | DCO PWM Depth        | 0-127      | PWM modulation depth            | 🟢 Low   |

### Sustain Pedal (CC #64)

**MIDI Message:**

```
Control Change: 0xB0 <CC #64> <value>

value:
  0-63: Pedal up (release notes)
  64-127: Pedal down (sustain notes)
```

**Planned Implementation (M16):**

```cpp
void Synth::handleSustainPedal(int value) {
    bool pedalDown = (value >= 64);

    if (pedalDown) {
        // Pedal pressed: buffer note-off events
        sustainPedalDown_ = true;
    } else {
        // Pedal released: send all buffered note-offs
        sustainPedalDown_ = false;
        releaseAllSustainedNotes();
    }
}

void Synth::handleNoteOff(int midiNote) {
    if (sustainPedalDown_) {
        // Buffer note-off for later
        sustainedNotes_.insert(midiNote);
    } else {
        // Immediate note-off
        for (int i = 0; i < NUM_VOICES; ++i) {
            if (voices_[i].getCurrentNote() == midiNote) {
                voices_[i].noteOff();
            }
        }
    }
}
```

### MIDI Learn (Future)

**Planned Feature:**

Allow users to assign any CC to any parameter:

```cpp
struct MidiMapping {
    int cc;              // CC number (0-127)
    ParamId param;       // Synth parameter
    float min;           // Minimum value
    float max;           // Maximum value
    bool logarithmic;    // Linear or logarithmic mapping
};

std::vector<MidiMapping> midiMappings_;
```

**User Flow:**
1. Click "Learn" button next to parameter
2. Move MIDI controller
3. Mapping is saved
4. Subsequent CC messages update parameter

---

## MIDI Message Format

### Status Bytes

**Channel Messages (0x80-0xEF):**

```
Status Byte = Message Type | MIDI Channel

Message Type (upper nibble):
  0x80: Note Off
  0x90: Note On
  0xA0: Polyphonic Aftertouch (not implemented)
  0xB0: Control Change
  0xC0: Program Change (not implemented)
  0xD0: Channel Aftertouch (not implemented)
  0xE0: Pitch Bend

MIDI Channel (lower nibble):
  0x0: Channel 1
  0x1: Channel 2
  ...
  0xF: Channel 16
```

**Example:**
```
0x90 = Note On, Channel 1
0x9F = Note On, Channel 16
0xB3 = Control Change, Channel 4
```

**System Messages (0xF0-0xFF):**

Not currently implemented (not needed for basic operation).

### Data Bytes

**7-bit Values (0x00-0x7F):**

All data bytes use 7 bits (0-127):
- Bit 7 is always 0
- Valid range: 0-127

**Examples:**
```
Note number: 0-127 (C-1 to G9)
Velocity: 0-127 (0 = off, 127 = max)
CC value: 0-127 (parameter value)
```

**14-bit Values (Pitch Bend):**

Pitch bend uses two 7-bit bytes:
- LSB (Least Significant Byte): 0-127
- MSB (Most Significant Byte): 0-127
- Combined: 0-16383 (14-bit)

**Conversion:**
```cpp
int value14bit = (MSB << 7) | LSB;
// Example: MSB=64, LSB=0 → (64 << 7) | 0 = 8192 (center)
```

---

## Implementation Details

### Platform-Specific MIDI Handling

**Raspberry Pi (ALSA):**

```cpp
// In src/platform/pi/midi_driver.cpp
void MidiDriver::processMIDI() {
    unsigned char buffer[3];
    int bytes = snd_rawmidi_read(midiIn_, buffer, sizeof(buffer));

    if (bytes > 0) {
        synth_->handleMidi(buffer[0], buffer[1], buffer[2]);
    }
}
```

**Web (Web MIDI API):**

```javascript
// In web/js/midi.js
function handleMIDIMessage(event) {
    const [status, data1, data2] = event.data;

    synthNode.port.postMessage({
        type: 'midi',
        data: [status, data1, data2]
    });
}
```

### MIDI Handler in Synth Engine

```cpp
// In src/dsp/synth.cpp
void Synth::handleMidi(uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t messageType = status & 0xF0;
    uint8_t channel = status & 0x0F;

    switch (messageType) {
        case 0x90:  // Note On
            if (data2 > 0) {
                handleNoteOn(data1, data2 / 127.0f);
            } else {
                handleNoteOff(data1);
            }
            break;

        case 0x80:  // Note Off
            handleNoteOff(data1);
            break;

        case 0xE0:  // Pitch Bend
            {
                int bendValue = ((data2 << 7) | data1) - 8192;
                float normalizedBend = bendValue / 8192.0f;
                handlePitchBend(normalizedBend);
            }
            break;

        case 0xB0:  // Control Change
            handleControlChange(data1, data2);
            break;

        default:
            // Ignore other message types
            break;
    }
}

void Synth::handleControlChange(uint8_t cc, uint8_t value) {
    float normalizedValue = value / 127.0f;

    switch (cc) {
        case 1:  // Modulation Wheel
            handleModWheel(normalizedValue);
            break;

        case 64:  // Sustain Pedal (M16)
            // handleSustainPedal(value);
            break;

        case 74:  // Filter Cutoff (M16)
            // setFilterCutoff(normalizedValue);
            break;

        // ... other CCs
    }
}
```

---

## MIDI CC Value Mapping

### Linear Mapping

Most parameters use linear mapping:

```cpp
float paramValue = (ccValue / 127.0f) * (max - min) + min;
```

**Example: Filter Resonance**
```cpp
// CC value 0-127 → Resonance 0.0-1.0
float resonance = ccValue / 127.0f;
```

### Logarithmic Mapping

Some parameters (e.g., filter cutoff) use logarithmic mapping:

```cpp
float paramValue = min * pow(max / min, ccValue / 127.0f);
```

**Example: Filter Cutoff**
```cpp
// CC value 0-127 → Cutoff 20Hz-20kHz (logarithmic)
float cutoffHz = 20.0f * pow(1000.0f, ccValue / 127.0f);
```

### Discrete Mapping

Some parameters have discrete values (e.g., chorus mode):

```cpp
int mode = (ccValue * numModes) / 128;  // 128 to avoid overflow
```

**Example: Chorus Mode**
```cpp
// CC value 0-127 → Mode 0-3
// 0-31: Off, 32-63: I, 64-95: II, 96-127: I+II
int chorusMode = (ccValue / 32);  // Divides into 4 ranges
```

---

## MIDI Implementation Chart

**Standard MIDI Implementation Chart for Poor House Juno (M15):**

| Function              | Transmitted | Recognized | Remarks                    |
|-----------------------|-------------|------------|----------------------------|
| Basic Channel         | -           | 1-16       | Omni mode (all channels)   |
| Mode                  | -           | Mode 1     | Omni On, Poly              |
| Note Number           | -           | 0-127      | Full range                 |
| Velocity (Note On)    | -           | ✓          | 0-127                      |
| Velocity (Note Off)   | -           | ✗          | Ignored                    |
| Aftertouch (Poly)     | -           | ✗          | Not implemented            |
| Aftertouch (Channel)  | -           | ✗          | Not implemented            |
| Pitch Bend            | -           | ✓          | Configurable range (±1-12) |
| Control Change        | -           | Partial    | CC #1 (mod wheel)          |
| Program Change        | -           | ✗          | Not implemented (M16)      |
| System Exclusive      | -           | ✗          | Not implemented            |
| System Common         | -           | ✗          | Not implemented            |
| System Real Time      | -           | ✗          | Not implemented            |

**Legend:**
- ✓ = Implemented
- ✗ = Not implemented
- Partial = Partially implemented
- \- = Not applicable (synth does not transmit MIDI)

---

## Testing MIDI Implementation

### Test Procedure

**1. Note On/Off Test:**
```
Send: 0x90 60 100  (Note On, C4, velocity 100)
Wait: 1 second
Send: 0x80 60 0    (Note Off, C4)

Expected: Hear note for 1 second
```

**2. Velocity Test:**
```
Send: 0x90 60 1    (Very soft)
Send: 0x90 62 64   (Medium)
Send: 0x90 64 127  (Very hard)

Expected: Increasing brightness/loudness (if velocity sensitivity enabled)
```

**3. Pitch Bend Test:**
```
Send: 0xE0 0x00 0x40    (Center: 8192)
Send: 0xE0 0x00 0x00    (Down: 0)
Send: 0xE0 0xFF 0x7F    (Up: 16383)

Expected: Pitch remains stable, then bends down 2 semitones, then up 2 semitones
```

**4. Mod Wheel Test:**
```
Send: 0xB0 0x01 0x00    (Mod wheel down: 0)
Send: 0xB0 0x01 0x40    (Mod wheel half: 64)
Send: 0xB0 0x01 0x7F    (Mod wheel up: 127)

Expected: LFO depth increases from 0% to 100%
```

---

## References

- [MIDI 1.0 Specification](https://www.midi.org/specifications)
- [MIDI CC List](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2)
- [MIDI Implementation Chart Guide](https://www.midi.org/specifications-old/item/midi-implementation-chart)

---

**Last Updated:** January 10, 2026
