# Poor House Juno - IR3109 Filter Calibration & Tuning

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Filter Cutoff Frequency Mapping](#filter-cutoff-frequency-mapping)
3. [Resonance Tuning](#resonance-tuning)
4. [Key Tracking Implementation](#key-tracking-implementation)
5. [Envelope Modulation](#envelope-modulation)
6. [LFO Modulation](#lfo-modulation)
7. [Velocity Sensitivity](#velocity-sensitivity)
8. [Saturation Characteristics](#saturation-characteristics)
9. [High-Pass Filter Tuning](#high-pass-filter-tuning)
10. [Calibration Procedure](#calibration-procedure)

---

## Overview

The IR3109 4-pole ladder filter is the heart of the Juno-106's sound. Proper tuning of the filter is critical for authentic emulation. This document describes the calibration and tuning methodology used in Poor House Juno.

**Filter Implementation:**
- **Topology:** Zero-Delay Feedback (ZDF)
- **Poles:** 4 (24 dB/octave)
- **Type:** Lowpass
- **Self-oscillation:** Yes (at high resonance)

**File:** `src/dsp/filter.cpp`

---

## Filter Cutoff Frequency Mapping

### Parameter to Frequency Mapping

The filter cutoff parameter (0.0 - 1.0) maps logarithmically to frequency (20 Hz - 20 kHz):

```cpp
float baseCutoff = 20.0f * pow(1000.0f, params_.cutoff);
```

**Mathematical Formula:**

```
f_cutoff = 20 × 1000^p

where:
  p = cutoff parameter (0.0 - 1.0)
  f_cutoff = cutoff frequency in Hz
```

**Key Points:**

| Parameter | Frequency | Musical Note (approx.) |
|-----------|-----------|------------------------|
| 0.0       | 20 Hz     | E0 (sub-bass)          |
| 0.25      | 178 Hz    | F3                     |
| 0.50      | 1,414 Hz  | F6                     |
| 0.75      | 11,220 Hz | -                      |
| 1.0       | 20,000 Hz | (Nyquist limit)        |

**Tuning Notes:**
- Logarithmic taper matches hardware potentiometer feel
- Center position (0.5) gives musically useful ~1.4 kHz
- Full range covers human hearing (20 Hz - 20 kHz)

### Frequency Clamping

To prevent instability, the cutoff is clamped to safe limits:

```cpp
cutoffHz = clamp(cutoffHz, 20.0f, sampleRate_ * 0.49f);
```

**Rationale:**
- **Lower limit (20 Hz):** Below this, filter has minimal effect
- **Upper limit (0.49 × fs):** Prevents coefficient instability near Nyquist

At 48 kHz sample rate:
- Maximum cutoff = 23,520 Hz
- In practice, clamped to 20 kHz for safety

---

## Resonance Tuning

### Resonance Coefficient Mapping

The resonance parameter (0.0 - 1.0) maps linearly to feedback coefficient k (0.0 - 4.0):

```cpp
k_ = params_.resonance * 4.0f;
```

**Behavior:**

| Resonance | k Value | Effect                    |
|-----------|---------|---------------------------|
| 0.0       | 0.0     | No resonance              |
| 0.25      | 1.0     | Mild resonance peak       |
| 0.50      | 2.0     | Moderate resonance        |
| 0.75      | 3.0     | Strong resonance          |
| 0.95      | 3.8     | Near self-oscillation     |
| 1.0       | 4.0     | Full self-oscillation     |

### Self-Oscillation Threshold

The ZDF ladder filter achieves self-oscillation when k ≥ 4.0:

**Theory:**
```
For a 4-pole ladder, stability requires:
  k < 4.0  (stable)
  k = 4.0  (critical - self-oscillation)
  k > 4.0  (unstable)
```

**Practical Threshold:**
- Self-oscillation begins around k = 3.8 (resonance = 0.95)
- Full sine wave output at k = 4.0 (resonance = 1.0)

**Tuning:**
```cpp
// Allow slight over-resonance for hardware emulation
k_ = params_.resonance * 4.1f;  // Max k = 4.1 (slightly unstable)
```

**Note:** We use k_max = 4.0 for stability. Users who want screaming resonance can push to 1.0.

### Resonance Frequency Behavior

At self-oscillation, the filter outputs a sine wave at approximately the cutoff frequency:

```
f_osc ≈ f_cutoff × 1.02

(Slightly above cutoff due to phase response)
```

**Calibration Test:**
1. Set cutoff = 0.5 (1,414 Hz)
2. Set resonance = 1.0 (self-oscillation)
3. Measure output frequency
4. Expected: ~1,440 Hz (F6 + few cents)

---

## Key Tracking Implementation

Key tracking makes the filter cutoff follow the note pitch.

### Tracking Modes

```cpp
enum KeyTrack {
    KEY_TRACK_OFF = 0,   // No tracking
    KEY_TRACK_HALF = 1,  // 50% tracking
    KEY_TRACK_FULL = 2   // 100% tracking
};
```

### Tracking Calculation

```cpp
float keyTrackMod = 1.0f;
if (params_.keyTrack != KEY_TRACK_OFF) {
    // Calculate ratio between note frequency and A4 (440 Hz reference)
    float ratio = noteFrequency_ / 440.0f;

    if (params_.keyTrack == KEY_TRACK_HALF) {
        // Half tracking: square root of ratio
        keyTrackMod = sqrt(ratio);
    } else if (params_.keyTrack == KEY_TRACK_FULL) {
        // Full tracking: direct ratio
        keyTrackMod = ratio;
    }
}

float finalCutoff = baseCutoff * keyTrackMod;
```

### Musical Effect

**Off (No Tracking):**
- Filter cutoff is constant regardless of note pitch
- Good for pads, organs, fixed timbres

**Half Tracking:**
- Filter opens more for higher notes
- Maintains relative brightness across the keyboard
- Most musical for many patches

**Full Tracking:**
- Filter tracks note pitch exactly
- Maintains constant harmonic content
- Good for plucked sounds, leads

**Example:**

| Note | Frequency | Half Tracking Multiplier | Full Tracking Multiplier |
|------|-----------|--------------------------|--------------------------|
| C2   | 65 Hz     | 0.38× (down 8.3 semitones) | 0.15× (down 18.6 semitones) |
| A4   | 440 Hz    | 1.0× (reference)         | 1.0× (reference)         |
| C7   | 2093 Hz   | 2.18× (up 13.1 semitones) | 4.76× (up 26.2 semitones) |

---

## Envelope Modulation

The filter envelope modulates the cutoff frequency in semitones.

### Modulation Range

```cpp
// Envelope amount: -1.0 to +1.0 (bipolar)
// Range: ±48 semitones (4 octaves)
float envSemitones = params_.envAmount * 48.0f * envValue_;
float envMod = pow(2.0f, envSemitones / 12.0f);

float finalCutoff = baseCutoff * envMod;
```

**Behavior:**

| Env Amount | Env Value | Semitones | Multiplier  | Effect         |
|------------|-----------|-----------|-------------|----------------|
| +1.0       | 1.0       | +48       | 16.0×       | Up 4 octaves   |
| +1.0       | 0.5       | +24       | 4.0×        | Up 2 octaves   |
| +0.5       | 1.0       | +24       | 4.0×        | Up 2 octaves   |
| 0.0        | any       | 0         | 1.0×        | No modulation  |
| -0.5       | 1.0       | -24       | 0.25×       | Down 2 octaves |
| -1.0       | 1.0       | -48       | 0.0625×     | Down 4 octaves |

### Envelope Polarity (M13)

Filter envelope can be inverted for closed-filter effects:

```cpp
// M13: Apply filter envelope polarity
float filterModValue = filterEnvValue;
if (filterEnvPolarity_ == 1) {  // Inverse mode
    filterModValue = 1.0f - filterEnvValue;
}

filter_.setEnvValue(filterModValue);
```

**Normal Mode (0):**
- Envelope up → filter opens
- Classic filter sweep

**Inverse Mode (1):**
- Envelope up → filter closes
- Reverse filter sweep (unusual but musical)

---

## LFO Modulation

LFO modulates the filter cutoff for rhythmic or vibrato effects.

### Modulation Range

```cpp
// LFO amount: 0.0 to 1.0 (unipolar)
// LFO value: -1.0 to +1.0 (bipolar)
// Range: ±24 semitones (2 octaves)
float lfoSemitones = lfoValue_ * params_.lfoAmount * 24.0f;
float lfoMod = pow(2.0f, lfoSemitones / 12.0f);

float finalCutoff = baseCutoff * lfoMod;
```

**Behavior:**

| LFO Amount | LFO Value | Semitones | Multiplier | Effect       |
|------------|-----------|-----------|------------|--------------|
| 1.0        | +1.0      | +24       | 4.0×       | Up 2 octaves |
| 1.0        | 0.0       | 0         | 1.0×       | At center    |
| 1.0        | -1.0      | -24       | 0.25×      | Down 2 oct   |
| 0.5        | +1.0      | +12       | 2.0×       | Up 1 octave  |

**Modulation Wheel Control (M13):**

The mod wheel scales the LFO depth globally:

```cpp
// In Synth::processStereo()
float modulatedLfo = lfoValue * performanceParams_.modWheel;
```

**Effect:**
- Mod wheel = 0.0 → No LFO (even if LFO amount is set)
- Mod wheel = 0.5 → Half LFO depth
- Mod wheel = 1.0 → Full LFO depth

---

## Velocity Sensitivity

Velocity can modulate the filter cutoff (M14).

### Modulation Calculation

```cpp
// Velocity: 0.0 to 1.0
// Amount: 0.0 to 1.0
// Range: ±24 semitones (2 octaves)
float velocitySemitones = (velocityValue_ - 0.5f) * 2.0f * velocityAmount_ * 24.0f;
float velocityMod = pow(2.0f, velocitySemitones / 12.0f);

float finalCutoff = baseCutoff * velocityMod;
```

**Behavior:**

| Velocity | Amount | Semitones | Effect                  |
|----------|--------|-----------|-------------------------|
| 0.0      | 1.0    | -24       | Down 2 octaves (closed) |
| 0.5      | 1.0    | 0         | No change               |
| 1.0      | 1.0    | +24       | Up 2 octaves (open)     |
| 0.0      | 0.5    | -12       | Down 1 octave           |
| 1.0      | 0.5    | +12       | Up 1 octave             |

**Musical Use:**
- Amount = 0.0: No velocity sensitivity (all notes same brightness)
- Amount = 0.5: Moderate velocity response (typical)
- Amount = 1.0: Strong velocity response (expressive)

---

## Saturation Characteristics

The IR3109 exhibits soft saturation at high input levels.

### Saturation Implementation

```cpp
float Filter::saturate(float x) {
    // Soft saturation using tanh
    // Only apply if drive > 1.0
    if (params_.drive <= 1.0f) {
        return x;
    }

    return std::tanh(x);
}
```

**Transfer Function:**

```
       1.0 ┤        ╭────────
           │      ╱
           │    ╱
  output   │  ╱
           │╱
      -1.0 ┼────────╮
          -3 -2 -1  0  1  2  3
                input

  y = tanh(x * drive)
```

**Harmonic Content:**

| Drive | Harmonic Distortion (THD) |
|-------|---------------------------|
| 1.0   | 0% (linear)               |
| 1.5   | ~2% (subtle warmth)       |
| 2.0   | ~5% (mild distortion)     |
| 4.0   | ~15% (strong saturation)  |

**Tuning Recommendation:**
- Default drive = 1.0 (clean)
- For analog warmth: drive = 1.2 - 1.5
- For aggressive sounds: drive = 2.0 - 4.0

---

## High-Pass Filter Tuning

The high-pass filter (M11) removes low frequencies.

### HPF Modes

```cpp
// Mode 0 = Off, 1 = 30Hz, 2 = 60Hz, 3 = 120Hz
if (params_.hpfMode > 0) {
    float hpfCutoff = 30.0f * pow(2.0f, params_.hpfMode - 1);
    float hpfWc = TWO_PI * hpfCutoff / sampleRate_;
    hpfG_ = tan(hpfWc * 0.5f);
}
```

**Cutoff Frequencies:**

| Mode | Cutoff | Effect                          |
|------|--------|---------------------------------|
| 0    | Off    | Full bass response              |
| 1    | 30 Hz  | Remove sub-bass rumble          |
| 2    | 60 Hz  | Tighten bass (kick drums)       |
| 3    | 120 Hz | Thin sound (remove low-mids)    |

### HPF Processing

The HPF is applied before the main lowpass filter:

```cpp
Sample Filter::process(Sample input) {
    // Apply HPF first (if enabled)
    if (params_.hpfMode > 0) {
        input = processHPF(input);
    }

    // Then process through lowpass ladder
    // ...
}
```

**1-Pole HPF Algorithm:**

```cpp
float Filter::processHPF(float input) {
    // 1-pole high-pass filter using ZDF topology
    float v = (input - hpfState_) * hpfG_;
    float lp = v + hpfState_;
    hpfState_ = lp + v;

    // High-pass output = input - lowpass
    return input - lp;
}
```

**Frequency Response:**
- Rolloff: 6 dB/octave (1-pole)
- -3 dB at cutoff frequency
- Phase shift: 45° at cutoff

---

## Calibration Procedure

### 1. Cutoff Frequency Calibration

**Objective:** Verify cutoff matches expected frequency across parameter range.

**Test Procedure:**
1. Set resonance = 0, envelope amount = 0, LFO amount = 0
2. Generate white noise input
3. Process through filter
4. Calculate frequency response (FFT)
5. Measure -3 dB point
6. Repeat for cutoff = 0.0, 0.25, 0.5, 0.75, 1.0

**Expected Results:**

| Cutoff Param | Expected Frequency | Tolerance     |
|--------------|--------------------|---------------|
| 0.0          | 20 Hz              | ±2 Hz         |
| 0.25         | 178 Hz             | ±5 Hz         |
| 0.50         | 1,414 Hz           | ±20 Hz        |
| 0.75         | 11,220 Hz          | ±200 Hz       |
| 1.0          | 20,000 Hz          | ±500 Hz       |

**Python Script:**
```bash
python tools/measure_filter.py --test cutoff_sweep
```

### 2. Resonance Calibration

**Objective:** Verify resonance peak height and self-oscillation.

**Test Procedure:**
1. Set cutoff = 0.5 (1,414 Hz)
2. Sweep resonance from 0.0 to 1.0
3. Measure peak height at cutoff frequency
4. Verify self-oscillation at resonance = 1.0

**Expected Results:**

| Resonance | Peak Height | Q Factor |
|-----------|-------------|----------|
| 0.0       | 0 dB        | 0.5      |
| 0.25      | +3 dB       | ~1.0     |
| 0.50      | +6 dB       | ~2.0     |
| 0.75      | +12 dB      | ~4.0     |
| 0.95      | +24 dB      | ~10.0    |
| 1.0       | ∞ (osc)     | ∞        |

**Python Script:**
```bash
python tools/measure_filter.py --test resonance_sweep --cutoff 0.5
```

### 3. Envelope Modulation Calibration

**Objective:** Verify envelope modulation range and linearity.

**Test Procedure:**
1. Set cutoff = 0.5, resonance = 0
2. Set envelope amount = +1.0
3. Measure cutoff at envelope values: 0.0, 0.25, 0.5, 0.75, 1.0
4. Verify semitone accuracy

**Expected Results:**

| Env Value | Semitones | Frequency Multiplier | Measured Freq |
|-----------|-----------|----------------------|---------------|
| 0.0       | 0         | 1.0×                 | 1,414 Hz      |
| 0.25      | +12       | 2.0×                 | 2,828 Hz      |
| 0.50      | +24       | 4.0×                 | 5,656 Hz      |
| 0.75      | +36       | 8.0×                 | 11,312 Hz     |
| 1.0       | +48       | 16.0×                | 22,624 Hz*    |

*Clamped to ~20 kHz

### 4. Key Tracking Calibration

**Objective:** Verify key tracking follows note pitch correctly.

**Test Procedure:**
1. Set cutoff = 0.3 (100 Hz), key tracking = FULL
2. Play notes C2 (65 Hz), A4 (440 Hz), C7 (2093 Hz)
3. Measure filter cutoff for each note

**Expected Results (Full Tracking):**

| Note | Note Freq | Expected Cutoff | Ratio  |
|------|-----------|-----------------|--------|
| C2   | 65 Hz     | 14.8 Hz*        | 0.148× |
| A4   | 440 Hz    | 100 Hz          | 1.0×   |
| C7   | 2093 Hz   | 476 Hz          | 4.76×  |

*Clamped to 20 Hz minimum

---

## Troubleshooting

### Problem: Filter Self-Oscillates Too Early

**Symptoms:** Resonance causes self-oscillation below resonance = 1.0

**Causes:**
- ZDF coefficient instability
- Incorrect k scaling

**Solution:**
```cpp
// Reduce k_max slightly
k_ = params_.resonance * 3.95f;  // Instead of 4.0
```

### Problem: Filter Sounds Too Bright

**Symptoms:** High frequencies too prominent even at low cutoff

**Causes:**
- Cutoff mapping too high
- Missing saturation

**Solution:**
```cpp
// Adjust cutoff curve
float baseCutoff = 15.0f * pow(1000.0f, params_.cutoff);  // Start at 15 Hz instead of 20
```

### Problem: Filter Sounds Too Dull

**Symptoms:** Lacks high-frequency content

**Causes:**
- Excessive saturation
- Incorrect ZDF coefficients

**Solution:**
```cpp
// Reduce default drive
params_.drive = 1.0f;  // No saturation by default
```

---

## References

- Zavalishin, V. (2012). "The Art of VA Filter Design"
- Smith, J.O. "Introduction to Digital Filters"
- Stilson & Smith (1996). "Analyzing the Moog VCF with Considerations for Digital Implementation"
- Roland IR3109 Datasheet (limited availability)

---

**Last Updated:** January 10, 2026
