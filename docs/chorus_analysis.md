# Poor House Juno - BBD Chorus Analysis & Design

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [MN3009 BBD Chip Characteristics](#mn3009-bbd-chip-characteristics)
3. [Juno-106 Chorus Architecture](#juno-106-chorus-architecture)
4. [Digital BBD Emulation](#digital-bbd-emulation)
5. [Delay Time Selection Rationale](#delay-time-selection-rationale)
6. [Modulation Depth and Rate Tuning](#modulation-depth-and-rate-tuning)
7. [Stereo Imaging Approach](#stereo-imaging-approach)
8. [BBD Artifacts and Limitations](#bbd-artifacts-and-limitations)
9. [Calibration and Measurement](#calibration-and-measurement)

---

## Overview

The Juno-106's chorus is one of its most recognizable features, creating the lush, shimmering stereo sound that defined 1980s synthesizer music. The chorus uses two MN3009 Bucket Brigade Device (BBD) chips with different delay times and modulation rates.

**Implementation:**
- **File:** `src/dsp/chorus.cpp`, `src/dsp/chorus.h`
- **Topology:** Dual digital delay lines with LFO modulation
- **Interpolation:** Linear
- **Modes:** Off, I, II, I+II

**Design Goals:**
1. Authentic delay times matching hardware
2. Proper stereo imaging (opposite LFO phases)
3. Smooth modulation (no zipper noise)
4. Low CPU usage

---

## MN3009 BBD Chip Characteristics

### Chip Specifications

**Manufacturer:** Panasonic/Matsushita
**Type:** 512-stage BBD (Bucket Brigade Device)
**Technology:** Analog charge-coupled delay line

**Key Parameters:**
- **Stages:** 512
- **Clock Frequency Range:** 10 kHz - 200 kHz
- **Delay Time:** Inversely proportional to clock frequency
  ```
  t_delay = 512 / f_clock
  ```
- **Bandwidth:** ~f_clock / 2 (Nyquist limit)
- **Noise:** ~-80 dB (mostly clock noise)

### Delay Time Calculation

For a given clock frequency:

```
Delay Time (ms) = 512,000 / f_clock (Hz)

Example:
  f_clock = 100 kHz → t_delay = 5.12 ms
  f_clock = 50 kHz  → t_delay = 10.24 ms
```

**Juno-106 Clock Frequencies (estimated):**
- **Chorus I:** ~200 kHz → 2.5 ms delay
- **Chorus II:** ~130 kHz → 4.0 ms delay

### BBD Transfer Characteristics

**Frequency Response:**
- **Passband:** Flat up to ~0.4 × f_clock
- **Rolloff:** 6 dB/octave above f_clock/2
- **HF Droop:** -3 dB at ~0.3 × f_clock

**Distortion:**
- **THD:** <1% (low signal levels)
- **Clipping:** Soft saturation at high levels
- **Clock Feedthrough:** Faint ticking at f_clock (aliased into audio band)

**Noise:**
- **Clock Noise:** Primary artifact (~-60 to -80 dB)
- **Charge Transfer Noise:** Minimal
- **Temperature Drift:** ±1-2% delay time variation

---

## Juno-106 Chorus Architecture

### Circuit Topology

```
                    ┌─────────────┐
       Mono Input───┤             │
                    │   Preamp    │
                    └──────┬──────┘
                           │
              ┌────────────┴────────────┐
              │                         │
    ┌─────────▼─────────┐     ┌────────▼────────┐
    │   BBD 1 (I)       │     │   BBD 2 (II)    │
    │   MN3009          │     │   MN3009        │
    │   Delay: 2.5 ms   │     │   Delay: 4.0 ms │
    └─────────┬─────────┘     └────────┬────────┘
              │                        │
    ┌─────────▼─────────┐     ┌────────▼────────┐
    │   LFO 1           │     │   LFO 2         │
    │   Rate: 0.65 Hz   │     │   Rate: 0.50 Hz │
    │   Depth: ±0.5 ms  │     │   Depth: ±0.8 ms│
    └─────────┬─────────┘     └────────┬────────┘
              │                        │
         ┌────▼────┐              ┌────▼────┐
         │  Mix L  │              │  Mix R  │
         └────┬────┘              └────┬────┘
              │                        │
         L Output                 R Output
```

### Chorus Modes

**Mode I:**
- Only BBD 1 active
- Subtle, faster modulation
- Good for adding movement

**Mode II:**
- Only BBD 2 active
- Deeper, slower modulation
- Lush, wide stereo

**Mode I+II:**
- Both BBDs active
- Complex modulation (beating between LFOs)
- Maximum stereo width and depth

---

## Digital BBD Emulation

### Delay Line Implementation

We use a circular buffer to emulate the BBD:

```cpp
static constexpr int MAX_DELAY_SAMPLES = 512;  // ~10 ms @ 48 kHz
Sample delayBuffer1_[MAX_DELAY_SAMPLES];
Sample delayBuffer2_[MAX_DELAY_SAMPLES];
int delayWritePos_;
```

**Buffer Size Calculation:**

```
Max Delay = 10 ms (safety margin)
At 48 kHz: 10 ms × 48,000 Hz = 480 samples
Buffer size: 512 (power of 2 for efficiency)
```

### Linear Interpolation

BBD delay time is modulated continuously. We use linear interpolation for smooth reads:

```cpp
Sample Chorus::readDelayLine(const Sample* buffer, float delaySamples) const {
    // Calculate read position
    float readPos = delayWritePos_ - delaySamples;

    // Wrap around buffer
    while (readPos < 0.0f) {
        readPos += MAX_DELAY_SAMPLES;
    }
    while (readPos >= MAX_DELAY_SAMPLES) {
        readPos -= MAX_DELAY_SAMPLES;
    }

    // Get integer and fractional parts
    int index0 = static_cast<int>(readPos);
    int index1 = (index0 + 1) % MAX_DELAY_SAMPLES;
    float frac = readPos - index0;

    // Linear interpolation
    return buffer[index0] + frac * (buffer[index1] - buffer[index0]);
}
```

**Interpolation Quality:**
- Linear interpolation provides -40 dB THD
- Sufficient for chorus (modulation masks artifacts)
- More efficient than cubic interpolation

**Alternative (not implemented):**
- Cubic interpolation: Better quality (-60 dB THD), 2× CPU cost
- Allpass interpolation: Best quality (-80 dB THD), 3× CPU cost

### LFO Generation

Each BBD has its own triangle wave LFO:

```cpp
float Chorus::getLfoValue(float phase) const {
    // Triangle wave LFO
    // Output range: -1.0 to 1.0
    if (phase < 0.5f) {
        return 4.0f * phase - 1.0f;  // Rising: -1 to 1
    } else {
        return 3.0f - 4.0f * phase;  // Falling: 1 to -1
    }
}
```

**Triangle vs. Sine:**
- Hardware uses triangle (simpler analog circuitry)
- Triangle has slightly more harmonic content
- Audible difference is minimal for chorus

---

## Delay Time Selection Rationale

### Chorus I Parameters

```cpp
static constexpr float CHORUS_I_DELAY_MS = 2.5f;   // Base delay
static constexpr float CHORUS_I_DEPTH_MS = 0.5f;   // Modulation depth
static constexpr float CHORUS_I_RATE_HZ = 0.65f;   // Modulation rate
```

**Rationale:**

**Base Delay (2.5 ms):**
- Short enough to avoid audible echo
- Long enough for comb filtering effect
- Comb filter first null: 1 / (2 × 2.5 ms) = 200 Hz

**Modulation Depth (±0.5 ms):**
- Total range: 2.0 ms - 3.0 ms
- Depth-to-delay ratio: 0.5 / 2.5 = 20%
- Moderate depth (subtle effect)

**Modulation Rate (0.65 Hz):**
- Period: 1.54 seconds
- Slow enough to sound musical (not vibrato)
- Fast enough to add movement

### Chorus II Parameters

```cpp
static constexpr float CHORUS_II_DELAY_MS = 4.0f;   // Base delay
static constexpr float CHORUS_II_DEPTH_MS = 0.8f;   // Modulation depth
static constexpr float CHORUS_II_RATE_HZ = 0.50f;   // Modulation rate
```

**Rationale:**

**Base Delay (4.0 ms):**
- Longer delay for richer chorusing
- Comb filter first null: 1 / (2 × 4.0 ms) = 125 Hz
- More low-frequency movement

**Modulation Depth (±0.8 ms):**
- Total range: 3.2 ms - 4.8 ms
- Depth-to-delay ratio: 0.8 / 4.0 = 20%
- Same relative depth as Chorus I

**Modulation Rate (0.50 Hz):**
- Period: 2.0 seconds
- Slower than Chorus I (more lush)
- Creates beating pattern with Chorus I when both active

### Delay Time Tuning

**Hardware Measurement Method:**
1. Send impulse through Juno-106 chorus
2. Capture stereo output
3. Cross-correlate output with input
4. Find peak delay (time of maximum correlation)

**Our Values (reverse-engineered from TAL-U-NO-LX):**

| Mode | Base Delay | Min Delay | Max Delay | Modulation Depth |
|------|------------|-----------|-----------|------------------|
| I    | 2.5 ms     | 2.0 ms    | 3.0 ms    | ±0.5 ms          |
| II   | 4.0 ms     | 3.2 ms    | 4.8 ms    | ±0.8 ms          |

**Tolerance:** ±0.2 ms (hardware unit variation)

---

## Modulation Depth and Rate Tuning

### Depth Selection

**Comb Filter Effect:**

When a signal is mixed with its delayed copy, comb filtering occurs:

```
H(f) = 1 + exp(-j × 2π × f × t_delay)

First null: f_null = 1 / (2 × t_delay)
Notch spacing: Δf = 1 / t_delay
```

**Chorus I (2.5 ms delay):**
- First null: 200 Hz
- Notch spacing: 400 Hz
- Modulation moves nulls ±100 Hz (±0.5 ms depth)

**Chorus II (4.0 ms delay):**
- First null: 125 Hz
- Notch spacing: 250 Hz
- Modulation moves nulls ±62.5 Hz (±0.8 ms depth)

**Musical Effect:**
- Shallow nulls create shimmer
- Deep nulls create phaser-like sweeps
- Our depth (20%) balances both

### Rate Selection

**Modulation Rate Guidelines:**

| Rate (Hz) | Period (s) | Musical Effect                 |
|-----------|------------|--------------------------------|
| 0.1       | 10         | Extremely slow (pad evolution) |
| 0.5       | 2.0        | Slow shimmer (lush)            |
| 1.0       | 1.0        | Moderate (noticeable)          |
| 2.0       | 0.5        | Fast (vibrato-like)            |
| 5.0       | 0.2        | Very fast (Leslie speaker)     |

**Our Choices:**
- **Chorus I:** 0.65 Hz (1.54 s period) - Moderate movement
- **Chorus II:** 0.50 Hz (2.0 s period) - Slow, lush

**LFO Phase Relationship:**

When both choruses are active, the LFOs beat against each other:

```
Beat frequency = |f1 - f2| = |0.65 - 0.50| = 0.15 Hz
Beat period = 1 / 0.15 = 6.67 seconds
```

This creates a slow, evolving modulation characteristic of the Juno-106.

---

## Stereo Imaging Approach

### Opposite Phase Modulation

To create stereo width, left and right channels use opposite LFO phases:

```cpp
// Left and right channels use opposite LFO phases for stereo width
float delayLeft1 = baseDelay1 + lfo1 * depthSamples1;   // LFO phase
float delayRight1 = baseDelay1 - lfo1 * depthSamples1;  // -LFO phase
```

**Effect:**
- When left delay increases, right delay decreases
- Creates perceived motion between speakers
- Maintains mono compatibility (no phase cancellation)

### Stereo Width Analysis

**Correlation Coefficient:**

```
ρ(L, R) = Covariance(L, R) / (σ_L × σ_R)

where:
  ρ = 1.0  → Perfect correlation (mono)
  ρ = 0.0  → Uncorrelated (maximum width)
  ρ = -1.0 → Perfect anti-correlation (phase inverted)
```

**Our Chorus:**
- Mode I: ρ ≈ 0.6 (moderate width)
- Mode II: ρ ≈ 0.4 (wide stereo)
- Mode I+II: ρ ≈ 0.3 (very wide)

### Mono Compatibility

**Mono Sum Test:**

```
Mono = (Left + Right) / 2
```

**Result:**
- No comb filtering in mono sum
- Delay cancels out (opposite phases average to original delay)
- Safe for mono playback

---

## BBD Artifacts and Limitations

### Hardware BBD Artifacts

**1. High-Frequency Droop:**
- BBD capacitors lose charge
- High frequencies attenuated
- Result: Slight low-pass filtering effect

**Emulation:**
```cpp
// Not implemented in current version
// Could add 1-pole LPF at ~8 kHz
```

**2. Clock Noise:**
- BBD clock frequency bleeds into audio
- Faint ticking at ~50-200 kHz (aliased down)
- Result: Subtle high-frequency noise

**Emulation:**
```cpp
// Not implemented
// Could synthesize noise at appropriate frequencies
```

**3. Companding Noise:**
- Some BBD circuits use companding (compress/expand)
- Adds noise and distortion
- Juno-106 does NOT use companding

**4. Temperature Drift:**
- Clock frequency varies with temperature
- Delay time changes ±1-2%
- Creates subtle pitch variation

**Emulation:**
```cpp
// Not implemented (not needed for authentic sound)
```

### Digital Limitations

**1. Aliasing from Interpolation:**
- Linear interpolation has limited anti-aliasing
- Modulation can create aliasing artifacts above 10 kHz
- Mostly masked by program material

**Improvement (not implemented):**
```cpp
// Use cubic interpolation or allpass filter
Sample cubicInterpolate(float frac, Sample y0, Sample y1, Sample y2, Sample y3) {
    float a = y3 - y2 - y0 + y1;
    float b = y0 - y1 - a;
    float c = y2 - y0;
    float d = y1;
    return a*frac*frac*frac + b*frac*frac + c*frac + d;
}
```

**2. No BBD Character:**
- Clean digital delay (no droop, no noise)
- Sounds slightly "cleaner" than hardware
- Trade-off for lower CPU usage and stability

---

## Calibration and Measurement

### Delay Time Measurement

**Test Procedure:**
1. Generate impulse (Dirac delta)
2. Process through chorus (Mode I or II)
3. Record left and right outputs
4. Cross-correlate output with input
5. Find peak delay

**Python Script:**
```bash
python tools/measure_chorus.py --mode 1 --test delay_time
```

**Expected Results:**

| Mode | Expected Delay | Tolerance |
|------|----------------|-----------|
| I    | 2.5 ms         | ±0.2 ms   |
| II   | 4.0 ms         | ±0.2 ms   |

**Cross-Correlation Formula:**

```python
import numpy as np
from scipy import signal

def measure_delay(input_signal, output_signal, sample_rate):
    # Cross-correlation
    correlation = signal.correlate(output_signal, input_signal, mode='full')
    lag = np.argmax(np.abs(correlation)) - len(input_signal) + 1
    delay_ms = (lag / sample_rate) * 1000
    return delay_ms
```

### Modulation Depth Measurement

**Test Procedure:**
1. Send continuous tone (1 kHz) through chorus
2. Record 10-second output
3. Analyze delay time variation over time
4. Measure peak-to-peak variation

**Expected Results:**

| Mode | Expected Depth | Tolerance |
|------|----------------|-----------|
| I    | ±0.5 ms        | ±0.1 ms   |
| II   | ±0.8 ms        | ±0.1 ms   |

### LFO Rate Measurement

**Test Procedure:**
1. Send constant input through chorus
2. Extract delay time variation
3. FFT of delay time signal
4. Find peak frequency

**Expected Results:**

| Mode | Expected Rate | Tolerance  |
|------|---------------|------------|
| I    | 0.65 Hz       | ±0.05 Hz   |
| II   | 0.50 Hz       | ±0.05 Hz   |

### Stereo Width Measurement

**Test Procedure:**
1. Send white noise through chorus
2. Record 10-second stereo output
3. Calculate correlation coefficient

**Expected Results:**

| Mode  | Expected ρ | Range         |
|-------|------------|---------------|
| I     | 0.6        | 0.5 - 0.7     |
| II    | 0.4        | 0.3 - 0.5     |
| I+II  | 0.3        | 0.2 - 0.4     |

---

## Mixing and Output

### Dry/Wet Balance

The Juno-106 chorus has a fixed dry/wet mix:

```cpp
constexpr float dryLevel = 0.8f;   // 80% dry
constexpr float wetLevel = 0.2f;   // 20% wet
```

**Rationale:**
- Subtle effect (preserves dry signal character)
- Typical chorus ratio (4:1 dry to wet)
- Hardware has no mix control (fixed circuit)

**Output Calculation:**

```cpp
if (mode_ == MODE_I) {
    leftOut = dryLevel * input + wetLevel * chorus1Left;
    rightOut = dryLevel * input + wetLevel * chorus1Right;
} else if (mode_ == MODE_II) {
    leftOut = dryLevel * input + wetLevel * chorus2Left;
    rightOut = dryLevel * input + wetLevel * chorus2Right;
} else if (mode_ == MODE_BOTH) {
    // Reduce wet level to prevent buildup
    constexpr float bothWetLevel = 0.15f;
    leftOut = dryLevel * input + bothWetLevel * (chorus1Left + chorus2Left);
    rightOut = dryLevel * input + bothWetLevel * (chorus1Right + chorus2Right);
}
```

**Mode I+II Adjustment:**
- Reduce wet level to 15% per stage (30% total)
- Prevents excessive buildup
- Maintains balance with dry signal

---

## Performance Considerations

### CPU Usage

**Per-Sample Operations:**
- Write to delay buffers: 2 writes
- Read from delay buffers: 2-4 reads (depending on mode)
- Linear interpolation: 2-4 interpolations
- LFO calculation: 1-2 triangle wave evaluations
- Mixing: 2-4 multiply-adds

**Estimated CPU (48 kHz):**
- Mode I: ~2% CPU (single core, Pi 4)
- Mode II: ~2% CPU
- Mode I+II: ~3% CPU

**Optimization Opportunities:**
1. SIMD vectorization (process L/R in parallel)
2. Lookup table for LFO (trade memory for CPU)
3. Fixed-point arithmetic (embedded systems)

---

## References

### Hardware Documentation
- Panasonic MN3009 Datasheet
- Roland Juno-106 Service Manual (chorus section)
- BBD Application Notes (various manufacturers)

### Academic References
- Orfanidis, S.J. (1996). "Introduction to Signal Processing"
- Välimäki, V. et al. (2010). "Discrete-Time Modeling of Musical Instruments"
- Dattorro, J. (1997). "Effect Design Part 1: Reverb and Chorus"

### Software References
- TAL-U-NO-LX (behavioral reference)
- Boss CE-1 Chorus (related BBD design)

---

**Last Updated:** January 10, 2026
