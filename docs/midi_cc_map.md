# MIDI CC Mapping Reference

**Poor House Juno - Complete MIDI Control Change Implementation (M16)**

---

## Table of Contents

1. [Overview](#overview)
2. [Complete CC List](#complete-cc-list)
3. [Oscillator (DCO) Controls](#oscillator-dco-controls)
4. [Filter (VCF) Controls](#filter-vcf-controls)
5. [LFO Controls](#lfo-controls)
6. [Envelope Controls](#envelope-controls)
7. [Performance Controls](#performance-controls)
8. [Effects Controls](#effects-controls)
9. [Controller Profiles](#controller-profiles)
10. [MIDI Implementation Chart](#midi-implementation-chart)

---

## Overview

Poor House Juno implements **comprehensive MIDI CC mapping** (M16 feature) allowing complete control of all synthesis parameters via MIDI controllers. This enables hardware controller integration and DAW automation.

### MIDI Channel

- **Default:** Responds to all MIDI channels (omni mode)
- **Future:** Channel selection may be added

### CC Value Range

- **Input:** 0-127 (7-bit MIDI standard)
- **Internal:** Mapped to appropriate parameter ranges (linear, logarithmic, or exponential)

---

## Complete CC List

### Quick Reference Table

| CC # | Parameter | Range | Type | Notes |
|------|-----------|-------|------|-------|
| **1** | Mod Wheel | 0-127 | Linear | LFO depth control |
| **14** | DCO Saw Level | 0-127 | Linear | Sawtooth waveform mix |
| **15** | DCO Pulse Level | 0-127 | Linear | Pulse waveform mix |
| **16** | DCO Sub Level | 0-127 | Linear | Sub-oscillator mix |
| **17** | DCO Noise Level | 0-127 | Linear | White noise mix |
| **18** | DCO LFO Target | 0-127 | Discrete | 0=Off, 1=Pitch, 2=PWM, 3=Both |
| **19** | DCO Range | 0-127 | Discrete | 0=16', 1=8', 2=4' |
| **20** | Filter LFO Amount | 0-127 | Linear | LFO → Filter modulation |
| **21** | Filter Key Track | 0-127 | Discrete | 0=Off, 1=Half, 2=Full |
| **22** | HPF Mode | 0-127 | Discrete | 0=Off, 1=30Hz, 2=60Hz, 3=120Hz |
| **23** | VCA Mode | 0-127 | Toggle | <64=ENV, ≥64=GATE |
| **24** | Filter Env Polarity | 0-127 | Toggle | <64=Normal, ≥64=Inverse |
| **25** | VCA Level | 0-127 | Linear | Output level control |
| **26** | Master Tune | 0-127 | Bipolar | 0=-50¢, 64=0¢, 127=+50¢ |
| **27** | Velocity → Filter | 0-127 | Linear | Velocity sensitivity (filter) |
| **28** | Velocity → Amp | 0-127 | Linear | Velocity sensitivity (amp) |
| **29** | Voice Allocation | 0-127 | Discrete | 0=Oldest, 1=Newest, 2=Low, 3=High |
| **64** | Sustain Pedal | 0-127 | Toggle | <64=Off, ≥64=On |
| **71** | Filter Resonance | 0-127 | Linear | Resonance amount |
| **73** | Filter Env Amount | 0-127 | Bipolar | 0=-100%, 64=0%, 127=+100% |
| **74** | Filter Cutoff | 0-127 | Log | Main filter frequency control |
| **75** | LFO Rate | 0-127 | Exp | 0=0.1Hz, 127=30Hz |
| **76** | LFO Delay | 0-127 | Linear | 0=0s, 127=3s |
| **77** | DCO Pulse Width | 0-127 | Linear | 0=5%, 127=95% |
| **78** | DCO PWM Depth | 0-127 | Linear | Pulse width modulation depth |
| **79** | Filter Env Attack | 0-127 | Exp | 0=1ms, 127=3s |
| **80** | Filter Env Decay | 0-127 | Exp | 0=2ms, 127=12s |
| **81** | Filter Env Sustain | 0-127 | Linear | Sustain level 0-100% |
| **82** | Filter Env Release | 0-127 | Exp | 0=2ms, 127=12s |
| **83** | Amp Env Attack | 0-127 | Exp | 0=1ms, 127=3s |
| **84** | Amp Env Decay | 0-127 | Exp | 0=2ms, 127=12s |
| **85** | Amp Env Sustain | 0-127 | Linear | Sustain level 0-100% |
| **86** | Amp Env Release | 0-127 | Exp | 0=2ms, 127=12s |
| **91** | Chorus Mode | 0-127 | Discrete | 0=Off, 1=I, 2=II, 3=I+II |
| **102** | Portamento Time | 0-127 | Exp | 0=0s, 127=10s |
| **103** | Pitch Bend Range | 0-127 | Linear | 0=±2, 127=±12 semitones |

**Total:** 29 CCs mapped

---

## Oscillator (DCO) Controls

### CC #14: Saw Level (0-127)

**Parameter:** DCO sawtooth waveform mix level
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Controls the amount of sawtooth in the mix

**Example:**
```
CC 14, Value 0   → No saw (0%)
CC 14, Value 64  → Half saw (50%)
CC 14, Value 127 → Full saw (100%)
```

### CC #15: Pulse Level (0-127)

**Parameter:** DCO pulse waveform mix level
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Controls the amount of pulse wave in the mix

### CC #16: Sub Level (0-127)

**Parameter:** Sub-oscillator mix level
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Adds weight and bass (one octave below main oscillator)

### CC #17: Noise Level (0-127)

**Parameter:** White noise mix level
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Adds breath, air, or percussive character

### CC #18: LFO Target (0-127)

**Parameter:** DCO LFO modulation destination
**Mapping:** Discrete (4 values)
**Values:**
- 0-31: Off (no LFO modulation)
- 32-63: Pitch (vibrato)
- 64-95: PWM (pulse width modulation)
- 96-127: Both (pitch + PWM)

### CC #19: DCO Range (0-127)

**Parameter:** Octave transpose
**Mapping:** Discrete (3 values)
**Values:**
- 0-42: 16' (down 1 octave)
- 43-84: 8' (normal pitch)
- 85-127: 4' (up 1 octave)

### CC #77: Pulse Width (0-127)

**Parameter:** Pulse waveform duty cycle
**Mapping:** Linear (0-127 → 5%-95%)
**Usage:** Changes timbre of pulse wave (50% = square wave)

### CC #78: PWM Depth (0-127)

**Parameter:** Pulse width modulation depth
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** How much LFO modulates pulse width when LFO target includes PWM

---

## Filter (VCF) Controls

### CC #74: Filter Cutoff (0-127)

**Parameter:** Filter cutoff frequency
**Mapping:** Logarithmic (0-127 → 20Hz-20kHz)
**Usage:** Primary tone control

**Frequency Table:**

| Value | Frequency |
|-------|-----------|
| 0 | 20 Hz |
| 32 | 100 Hz |
| 64 | 632 Hz |
| 96 | 3.2 kHz |
| 127 | 20 kHz |

### CC #71: Filter Resonance (0-127)

**Parameter:** Filter resonance amount
**Mapping:** Linear (0-127 → 0.0-4.0)
**Usage:** Emphasizes cutoff frequency; self-oscillates at maximum

### CC #73: Filter Env Amount (0-127)

**Parameter:** Filter envelope modulation depth
**Mapping:** Bipolar (0-127 → -1.0 to +1.0)
**Values:**
- 0: Full negative (-4 octaves)
- 64: Center (no modulation)
- 127: Full positive (+4 octaves)

### CC #20: Filter LFO Amount (0-127)

**Parameter:** LFO modulation of filter cutoff
**Mapping:** Linear (0-127 → 0.0-1.0)
**Range:** 0 to ±2 octaves

### CC #21: Filter Key Track (0-127)

**Parameter:** Keyboard tracking mode
**Mapping:** Discrete (3 values)
**Values:**
- 0-42: Off (same timbre across keyboard)
- 43-84: Half (subtle tracking)
- 85-127: Full (cutoff follows note 1:1)

### CC #22: HPF Mode (0-127)

**Parameter:** High-pass filter cutoff
**Mapping:** Discrete (4 values)
**Values:**
- 0-31: Off (full bass)
- 32-63: 30 Hz (remove rumble)
- 64-95: 60 Hz (thin bass)
- 96-127: 120 Hz (significant bass cut)

---

## LFO Controls

### CC #75: LFO Rate (0-127)

**Parameter:** LFO frequency
**Mapping:** Exponential (0-127 → 0.1Hz-30Hz)
**Formula:**
```
rate = 0.1 * pow(300.0, normalized)
```

**Rate Table:**

| Value | Rate |
|-------|------|
| 0 | 0.1 Hz (10s cycle) |
| 32 | 0.6 Hz |
| 64 | 3.2 Hz |
| 96 | 15 Hz |
| 127 | 30 Hz |

### CC #76: LFO Delay (0-127)

**Parameter:** LFO delay time (M12 feature)
**Mapping:** Linear (0-127 → 0.0-3.0 seconds)
**Usage:** LFO fades in over delay period after note-on

---

## Envelope Controls

### Filter Envelope (ADSR)

| CC # | Parameter | Mapping | Range |
|------|-----------|---------|-------|
| **79** | Attack | Exponential | 1ms - 3s |
| **80** | Decay | Exponential | 2ms - 12s |
| **81** | Sustain | Linear | 0.0 - 1.0 |
| **82** | Release | Exponential | 2ms - 12s |

**Exponential Formula (Attack):**
```cpp
attack = 0.001 * pow(3000.0, normalized)
```

### Amp Envelope (ADSR)

| CC # | Parameter | Mapping | Range |
|------|-----------|---------|-------|
| **83** | Attack | Exponential | 1ms - 3s |
| **84** | Decay | Exponential | 2ms - 12s |
| **85** | Sustain | Linear | 0.0 - 1.0 |
| **86** | Release | Exponential | 2ms - 12s |

**Time Mapping:**

| Value | Attack Time | Decay/Release Time |
|-------|-------------|-------------------|
| 0 | 1 ms | 2 ms |
| 32 | 18 ms | 37 ms |
| 64 | 55 ms | 200 ms |
| 96 | 500 ms | 2.2 s |
| 127 | 3 s | 12 s |

---

## Performance Controls

### CC #1: Mod Wheel (0-127)

**Parameter:** LFO depth multiplier (M13 feature)
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Real-time control of LFO modulation amount

### CC #64: Sustain Pedal (0-127)

**Parameter:** Sustain pedal (M16 feature)
**Mapping:** Toggle (< 64 = off, ≥ 64 = on)
**Behavior:**
- When on: Note-off messages are buffered
- When released: All sustained notes are released

### CC #23: VCA Mode (0-127)

**Parameter:** VCA control mode (M13 feature)
**Mapping:** Toggle (< 64 = ENV, ≥ 64 = GATE)
**Modes:**
- ENV: Amplitude envelope controls VCA (normal)
- GATE: Sustain held at 100% (organ style)

### CC #24: Filter Env Polarity (0-127)

**Parameter:** Filter envelope polarity (M13 feature)
**Mapping:** Toggle (< 64 = Normal, ≥ 64 = Inverse)
**Modes:**
- Normal: Positive envelope opens filter
- Inverse: Positive envelope closes filter

### CC #25: VCA Level (0-127)

**Parameter:** VCA output level (M14 feature)
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** Overall synthesizer output level

### CC #26: Master Tune (0-127)

**Parameter:** Global pitch offset (M14 feature)
**Mapping:** Bipolar (0-127 → -50 to +50 cents)
**Center:** Value 64 = 0 cents (A440)

### CC #27: Velocity → Filter (0-127)

**Parameter:** Velocity sensitivity for filter (M14 feature)
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** How much note velocity affects filter cutoff

### CC #28: Velocity → Amp (0-127)

**Parameter:** Velocity sensitivity for amplitude (M14 feature)
**Mapping:** Linear (0-127 → 0.0-1.0)
**Usage:** How much note velocity affects volume

### CC #29: Voice Allocation Mode (0-127)

**Parameter:** Voice stealing priority (M16 feature)
**Mapping:** Discrete (4 values)
**Values:**
- 0-31: Oldest First (default, round-robin)
- 32-63: Newest First
- 64-95: Low-Note Priority (preserve bass)
- 96-127: High-Note Priority (preserve leads)

### CC #102: Portamento Time (0-127)

**Parameter:** Glide time (M11 feature)
**Mapping:** Exponential (0-127 → 0-10 seconds)
**Usage:** Time to glide between notes in legato playing

### CC #103: Pitch Bend Range (0-127)

**Parameter:** Pitch bend wheel sensitivity (M11 feature)
**Mapping:** Linear (0-127 → ±2 to ±12 semitones)
**Default:** ±2 semitones

---

## Effects Controls

### CC #91: Chorus Mode (0-127)

**Parameter:** BBD chorus mode (M8 feature)
**Mapping:** Discrete (4 values)
**Values:**
- 0-31: Off (dry signal only)
- 32-63: Mode I (bright, 2.5ms delay)
- 64-95: Mode II (warm, 4.0ms delay)
- 96-127: Mode I+II (lush, both active)

---

## Controller Profiles

### Arturia MiniLab (Native Support)

The Arturia MiniLab is **natively supported** with default mappings:

| MiniLab Control | CC # | Parameter |
|-----------------|------|-----------|
| Knob 1 | 74 | Filter Cutoff |
| Knob 2 | 71 | Filter Resonance |
| Knob 3 | 73 | Filter Env Amount |
| Knob 4 | 75 | LFO Rate |
| Knob 5 | 14 | DCO Saw Level |
| Knob 6 | 15 | DCO Pulse Level |
| Knob 7 | 16 | DCO Sub Level |
| Knob 8 | 17 | DCO Noise Level |
| Pad 1 | 22 | HPF Mode (toggle) |
| Pad 2 | 91 | Chorus Mode (cycle) |
| Mod Wheel | 1 | LFO Depth |
| Pitch Bend | - | Pitch Bend |

### Generic MIDI Controller

**Recommended Assignments:**

**Essential Controls:**
- Knob 1: Filter Cutoff (CC #74)
- Knob 2: Filter Resonance (CC #71)
- Knob 3: LFO Rate (CC #75)
- Knob 4: Filter Env Amount (CC #73)

**Waveform Mix:**
- Knob 5-8: Saw/Pulse/Sub/Noise (CC #14-17)

**Envelopes:**
- Sliders 1-4: Filter ADSR (CC #79-82)
- Sliders 5-8: Amp ADSR (CC #83-86)

---

## MIDI Implementation Chart

### Channel Voice Messages

| Message | Status | Data 1 | Data 2 | Recognized |
|---------|--------|--------|--------|------------|
| Note Off | 8x | Note # | Velocity | ✅ Yes |
| Note On | 9x | Note # | Velocity | ✅ Yes |
| Poly Pressure | Ax | Note # | Pressure | ❌ No |
| Control Change | Bx | Controller | Value | ✅ Yes (29 CCs) |
| Program Change | Cx | Program | - | ❌ No (future) |
| Channel Pressure | Dx | Pressure | - | ❌ No |
| Pitch Bend | Ex | LSB | MSB | ✅ Yes |

### System Messages

| Message | Recognized |
|---------|------------|
| MIDI Clock | ❌ No (future) |
| Song Position | ❌ No |
| Song Select | ❌ No |
| Tune Request | ❌ No |
| System Reset | ✅ Yes (stops all notes) |

---

## Implementation Examples

### Setting Filter Cutoff to 50%

**MIDI Message:**
```
B0 4A 40    (Control Change, CC #74, Value 64)
```

**Result:** Filter cutoff set to ~630 Hz (50% of logarithmic range)

### Enabling Sustain Pedal

**MIDI Message:**
```
B0 40 7F    (Control Change, CC #64, Value 127)
```

**Result:** All subsequent note-offs are buffered until pedal released

### Setting LFO to Vibrato at 4 Hz

**MIDI Messages:**
```
B0 12 40    (Control Change, CC #18, Value 64) → LFO target = Pitch
B0 4B 50    (Control Change, CC #75, Value 80) → LFO rate ≈ 4 Hz
```

### Inverting Filter Envelope

**MIDI Message:**
```
B0 18 7F    (Control Change, CC #24, Value 127)
```

**Result:** Filter envelope polarity inverted (closes on attack)

---

## DAW Automation

All CC parameters can be automated in a DAW:

**Example (Ableton Live):**
1. Add Poor House Juno as audio track (web version in browser)
2. Enable MIDI track
3. Add automation for CC #74 (Filter Cutoff)
4. Draw automation curve

**Example (Reaper):**
1. Insert MIDI track → Poor House Juno
2. Right-click track → Show track envelopes → MIDI CC #74
3. Draw automation points

---

## Future Enhancements

**Planned for future milestones:**

- **Program Change:** Patch bank selection (0-127)
- **MIDI Learn:** Assign any CC to any parameter
- **MPE Support:** Multi-dimensional polyphonic expression
- **MIDI Clock Sync:** LFO rate synced to tempo
- **SysEx:** Patch dump and recall

---

## Troubleshooting

### CC Not Responding

**Check:**
1. MIDI channel (currently omni, responds to all)
2. CC number (verify in table above)
3. Parameter range (some are discrete, require specific values)

### Discrete Parameters Not Working

**Problem:** CCs for discrete parameters (e.g., Chorus Mode) need specific ranges.

**Solution:**
- Off: 0-31
- Mode I: 32-63
- Mode II: 64-95
- Mode I+II: 96-127

Send CC #91 with value 80 (not 2) for Mode II.

### Envelope Times Too Short/Long

**Problem:** Exponential mapping may seem unintuitive.

**Solution:**
- Values 0-50: Very fast (1-50ms)
- Values 50-100: Musical range (50ms-1s)
- Values 100-127: Slow (1s-12s)

Use values 60-90 for most sounds.

---

## Quick Reference Card

**Print this for your studio:**

```
Poor House Juno - MIDI CC Quick Reference

OSCILLATOR          FILTER           LFO
CC 14: Saw          CC 74: Cutoff    CC 75: Rate
CC 15: Pulse        CC 71: Res       CC 76: Delay
CC 16: Sub          CC 73: Env Amt
CC 17: Noise        CC 20: LFO Amt

ENVELOPES           PERFORMANCE      EFFECTS
CC 79-82: Filt ADSR CC 1: Mod Wheel  CC 91: Chorus
CC 83-86: Amp ADSR  CC 64: Sustain   CC 22: HPF
                    CC 25: VCA Level

Master Tune: CC 26 (64=center)
Voice Allocation: CC 29
Portamento: CC 102 | Bend Range: CC 103
```

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
**MIDI Implementation:** Milestone 16 (M16)
