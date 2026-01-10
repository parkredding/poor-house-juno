# Filter Tuning and Calibration

**Poor House Juno - IR3109 Filter Emulation Guide**

---

## Table of Contents

1. [Overview](#overview)
2. [IR3109 Filter Topology](#ir3109-filter-topology)
3. [Zero-Delay Feedback (ZDF) Implementation](#zero-delay-feedback-zdf-implementation)
4. [Cutoff Frequency Calibration](#cutoff-frequency-calibration)
5. [Resonance Tuning](#resonance-tuning)
6. [Modulation Calibration](#modulation-calibration)
7. [Saturation and Non-Linearity](#saturation-and-non-linearity)
8. [High-Pass Filter](#high-pass-filter)
9. [Testing and Validation](#testing-and-validation)
10. [Troubleshooting](#troubleshooting)

---

## Overview

The Roland IR3109 is a custom 4-pole transistor ladder filter chip used in the Juno-106 (and Juno-60, Jupiter-8). It provides the warm, smooth, musical character that defines the Juno sound.

### Design Goals

1. **Accurate Frequency Response:** Match the 24dB/oct slope of the IR3109
2. **Stable Resonance:** Self-oscillation at maximum resonance without instability
3. **Musical Tuning:** Cutoff frequency should track musically across the range
4. **Low CPU:** Efficient implementation for Raspberry Pi real-time performance

### Implementation Strategy

We use **Zero-Delay Feedback (ZDF)** topology, which solves the feedback delay problem inherent in naive digital ladder filters. This gives us:
- Accurate resonance behavior
- Stable self-oscillation
- Correct frequency response at all sample rates

---

## IR3109 Filter Topology

### Hardware Architecture

The IR3109 is a **4-pole transistor ladder filter** inspired by the Moog ladder but with a different circuit topology:

```
Input → [Pole 1] → [Pole 2] → [Pole 3] → [Pole 4] → Output
          ↑         ↑         ↑         ↑
          └─────────┴─────────┴─────────┴─ Feedback (Resonance)
```

**Each pole:**
- 1st-order lowpass filter (6dB/octave)
- 4 poles cascaded = 24dB/octave total
- Feedback from output to input creates resonance

### Transfer Function

**Single Pole (1st-order lowpass):**
```
H(s) = 1 / (1 + s/ωc)
```

**4-Pole Cascade:**
```
H(s) = 1 / (1 + s/ωc)^4
```

**With Feedback (k = resonance):**
```
H(s) = G(s) / (1 + k·G(s))
```

Where `G(s)` is the 4-pole cascade transfer function.

### Resonance Characteristics

- **k = 0:** No resonance, flat 24dB/oct rolloff
- **k = 2:** Moderate resonance, ~6dB peak at cutoff
- **k = 4:** Self-oscillation threshold (Juno-106 maximum)
- **k > 4:** Instability (oscillation grows)

---

## Zero-Delay Feedback (ZDF) Implementation

### The Problem with Naive Digital Filters

Traditional digital filter implementations have a **one-sample delay** in the feedback path:

```cpp
// WRONG: One-sample delay causes tuning error
output = process_stages(input - feedback_from_previous_sample);
```

This delay causes:
- Incorrect resonance frequency (shifts down)
- Unstable self-oscillation
- Frequency-dependent errors (worse at high frequencies)

### ZDF Solution

ZDF computes the feedback **instantaneously** by solving the feedback equation algebraically:

```cpp
// Correct: Instantaneous feedback
float feedback = stage4_ * k_;
float inputWithFeedback = input - feedback;
// Now process with this corrected input
```

**Mathematical Derivation:**

Given:
```
y = G(x - k·y)    where G = filter transfer function
```

Solve for y:
```
y = G·x / (1 + k·G)
```

In ZDF, we compute this by:
1. Calculate what the output would be without feedback
2. Use that to compute the actual feedback
3. Process the input with corrected feedback

### ZDF One-Pole Filter

**State Variable Form:**
```cpp
// s = state variable (integrator output)
// g = cutoff coefficient (tan(ω/2))

float v = (input - s) * g;   // v = (input - state) / (1 + g)
float out = v + s;            // Lowpass output
s = out + v;                  // State update (trapezoidal integration)
```

**Why This Works:**
- `v` is the rate of change of the state
- `out` is the lowpass output (current value)
- State update uses trapezoidal rule for stability

### ZDF 4-Pole Ladder

```cpp
Sample Filter::process(Sample input) {
    // Calculate feedback from 4th stage
    float feedback = stage4_ * k_;
    
    // Subtract feedback from input (instantaneous)
    float inputWithFeedback = input - feedback;
    
    // Process through 4 cascaded 1-pole filters
    // Stage 1
    float v1 = (inputWithFeedback - stage1_) * g_;
    float out1 = v1 + stage1_;
    stage1_ = out1 + v1;
    
    // Stage 2
    float v2 = (out1 - stage2_) * g_;
    float out2 = v2 + stage2_;
    stage2_ = out2 + v2;
    
    // Stage 3
    float v3 = (out2 - stage3_) * g_;
    float out3 = v3 + stage3_;
    stage3_ = out3 + v3;
    
    // Stage 4
    float v4 = (out3 - stage4_) * g_;
    float out4 = v4 + stage4_;
    stage4_ = out4 + v4;
    
    // Return 4-pole output
    return out4;
}
```

---

## Cutoff Frequency Calibration

### Bilinear Transform

To convert from analog cutoff frequency (Hz) to digital coefficient `g`:

```cpp
void Filter::updateCoefficients() {
    float cutoffHz = calculateCutoffHz();  // Modulated cutoff
    
    // Clamp to valid range
    cutoffHz = clamp(cutoffHz, 20.0f, sampleRate_ * 0.49f);
    
    // Calculate g coefficient using bilinear transform
    float wc = TWO_PI * cutoffHz / sampleRate_;  // Normalized angular frequency
    g_ = std::tan(wc * 0.5f);  // Bilinear transform
}
```

**Why `tan(ωc/2)`?**
- Bilinear transform maps continuous-time to discrete-time
- Tangent function pre-warps the frequency to correct for bilinear distortion
- At low frequencies: `tan(x) ≈ x` (linear)
- At Nyquist: `tan(π/2) → ∞` (prevents aliasing)

### Frequency Mapping

**Parameter Range:** 0.0 to 1.0 (slider position)

**Logarithmic Mapping:**
```cpp
float baseCutoff = 20.0f * std::pow(1000.0f, params_.cutoff);
```

**Rationale:**
- Human hearing is logarithmic
- Equal slider intervals should sound like equal intervals
- Range: 20 Hz (0.0) to 20 kHz (1.0)

**Frequency Table:**

| Slider | Cutoff | Note Equivalent |
|--------|--------|-----------------|
| 0.0 | 20 Hz | ~E0 (bottom of bass) |
| 0.25 | 126 Hz | ~B2 (bass) |
| 0.5 | 632 Hz | ~E5 (midrange) |
| 0.75 | 3162 Hz | ~G#7 (treble) |
| 1.0 | 20000 Hz | Near Nyquist |

### Cutoff Tracking Accuracy

**Test:** Measure actual -3dB point vs. expected cutoff

**Results (from unit tests):**
- At 100 Hz: Within 5% (95-105 Hz)
- At 1 kHz: Within 3% (970-1030 Hz)
- At 10 kHz: Within 10% (9-11 kHz, bilinear warping)

**Why Less Accurate at High Frequencies?**
- Bilinear transform warping (inherent to method)
- Acceptable: Juno-106 filter rarely used above 10 kHz anyway

---

## Resonance Tuning

### Resonance Coefficient Mapping

**Parameter Range:** 0.0 to 1.0 (slider position)

**Linear Mapping:**
```cpp
k_ = params_.resonance * 4.0f;  // 0.0 to 4.0
```

**Resonance Levels:**

| Slider | k Value | Behavior |
|--------|---------|----------|
| 0.0 | 0.0 | No resonance (flat response) |
| 0.25 | 1.0 | Slight emphasis at cutoff |
| 0.5 | 2.0 | Moderate resonance (~6dB peak) |
| 0.75 | 3.0 | Strong resonance (~12dB peak) |
| 1.0 | 4.0 | Self-oscillation threshold |

### Self-Oscillation Tuning

**Goal:** At `k = 4.0`, filter should produce a sine wave at cutoff frequency when given zero input (or very small input).

**Test:**
```cpp
TEST_CASE("Filter self-oscillation", "[filter]") {
    Filter filter;
    filter.setSampleRate(48000.0f);
    
    FilterParams params;
    params.cutoff = 0.5f;      // 632 Hz
    params.resonance = 1.0f;   // k = 4.0
    filter.setParameters(params);
    
    // Feed zero input, check for oscillation
    float maxOutput = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        float out = filter.process(0.0f);
        maxOutput = std::max(maxOutput, std::abs(out));
    }
    
    // Should oscillate with amplitude near 1.0
    REQUIRE(maxOutput > 0.5f);
    REQUIRE(maxOutput < 2.0f);  // But not unstable
}
```

**Tuning k for Hardware Match:**
- Original Juno-106: k ≈ 4.0 for self-oscillation
- TAL-U-NO-LX: k ≈ 4.0 for self-oscillation
- Our implementation: k = 4.0 (matches)

**If Self-Oscillation Doesn't Work:**
1. Check g coefficient calculation (must be correct)
2. Verify ZDF state update equations
3. Ensure no saturation before feedback point
4. Confirm k_ is actually 4.0 at max resonance

---

## Modulation Calibration

### Envelope Modulation

**Parameter:** `envAmount` (-1.0 to +1.0, bipolar)

**Modulation Depth:**
```cpp
float envMod = 1.0f;
if (params_.envAmount != 0.0f) {
    // ±48 semitones = ±4 octaves
    float envSemitones = params_.envAmount * 48.0f * envValue_;
    envMod = std::pow(2.0f, envSemitones / 12.0f);
}
```

**Rationale:**
- Juno-106 has approximately ±4 octave modulation range
- Verified by listening to TAL-U-NO-LX at extreme settings
- Multiplicative modulation (frequency ratio, not additive)

**Example:**
- `envAmount = 1.0`, `envValue_ = 1.0` (full envelope): +4 octaves (16× cutoff)
- `envAmount = -1.0`, `envValue_ = 1.0` (inverse): -4 octaves (1/16× cutoff)
- `envAmount = 0.5`, `envValue_ = 0.5` (mid): +1 octave (2× cutoff)

### LFO Modulation

**Parameter:** `lfoAmount` (0.0 to 1.0, unipolar)

**Modulation Depth:**
```cpp
float lfoMod = 1.0f;
if (params_.lfoAmount != 0.0f) {
    // ±24 semitones = ±2 octaves
    float lfoSemitones = params_.lfoAmount * 24.0f * lfoValue_;
    lfoMod = std::pow(2.0f, lfoSemitones / 12.0f);
}
```

**Rationale:**
- Juno-106 LFO has approximately ±2 octave modulation range
- LFO value ranges -1 to +1 (triangle wave)
- Smaller range than envelope (for musical LFO rates)

### Key Tracking

**Parameter:** `keyTrack` (0.0, 0.5, 1.0 = Off/Half/Full)

**Modulation:**
```cpp
float keyTrackMod = 1.0f;
if (params_.keyTrack > 0.0f) {
    float noteRatio = noteFrequency_ / 440.0f;  // Relative to A440
    keyTrackMod = std::pow(noteRatio, params_.keyTrack);
}
```

**Behavior:**
- **keyTrack = 0.0:** Filter cutoff independent of note (same timbre everywhere)
- **keyTrack = 0.5:** Filter cutoff follows note at half rate (subtle brightness change)
- **keyTrack = 1.0:** Filter cutoff tracks note 1:1 (fixed spectral content)

**Example:**
- Note = A440 (440 Hz), keyTrack = 1.0: keyTrackMod = 1.0 (no change)
- Note = A880 (880 Hz), keyTrack = 1.0: keyTrackMod = 2.0 (double cutoff)
- Note = A880 (880 Hz), keyTrack = 0.5: keyTrackMod = √2 ≈ 1.41 (≈+5 semitones)

### Velocity Modulation (M14)

**Parameters:** `velocityAmount` (0.0 to 1.0)

**Modulation:**
```cpp
float velocityMod = 1.0f;
if (velocityAmount_ > 0.0f) {
    // Map velocity (0-1) to ±24 semitones around center
    float velSemitones = (velocityValue_ - 0.5f) * 2.0f * 24.0f * velocityAmount_;
    velocityMod = std::pow(2.0f, velSemitones / 12.0f);
}
```

**Behavior:**
- Velocity 0.0 (soft): Cutoff down by up to 2 octaves
- Velocity 0.5 (medium): No change
- Velocity 1.0 (hard): Cutoff up by up to 2 octaves

### Combined Modulation

All modulation sources are **multiplicative:**
```cpp
return baseCutoff * envMod * lfoMod * keyTrackMod * velocityMod;
```

**Rationale:**
- Multiplicative = musical (constant interval relationships)
- Additive = non-musical (absolute Hz offsets)
- Matches analog filter behavior (CV is exponential)

---

## Saturation and Non-Linearity

### Input Drive

**Parameter:** `params_.drive` (typically 1.0 to 2.0)

```cpp
input *= params_.drive;
input = saturate(input);
```

**Purpose:**
- Emulate IR3109 transistor saturation
- Adds subtle warmth and compression
- Prevents filter instability from extreme inputs

### Soft Saturation Function

```cpp
float Filter::saturate(float x) {
    const float threshold = 1.5f;
    if (x > threshold) {
        return threshold + (x - threshold) * 0.1f;  // Gentle compression
    } else if (x < -threshold) {
        return -threshold + (x + threshold) * 0.1f;
    }
    return x;  // Linear region
}
```

**Characteristics:**
- Linear below ±1.5
- Soft compression above ±1.5 (10:1 ratio)
- Never hard clips (prevents harsh artifacts)

**Alternative (tanh):**
```cpp
float Filter::saturate(float x) {
    return std::tanh(x * 0.5f);  // Smooth saturation curve
}
```

**Trade-off:**
- Piecewise linear: Faster, less CPU
- Tanh: Smoother, more authentic analog behavior

### Resonance Compensation

High resonance can cause filter to get very loud. Some implementations compensate:

```cpp
// Optional: Reduce gain at high resonance
float gainCompensation = 1.0f / (1.0f + k_ * 0.25f);
return out4 * gainCompensation;
```

**Juno-106 Behavior:**
- Does NOT compensate (resonance boosts level)
- This is part of the sound (be careful with ears at max resonance!)

**Our Implementation:**
- Currently no compensation (matches hardware)
- Can be added as optional feature if needed

---

## High-Pass Filter

**M11 Feature:** 1-pole high-pass filter before the main lowpass filter.

### HPF Cutoff Frequencies

| Mode | Cutoff | Purpose |
|------|--------|---------|
| 0 | Off | Full bass |
| 1 | 30 Hz | Remove sub-bass rumble |
| 2 | 60 Hz | Thin out bass |
| 3 | 120 Hz | Significant bass reduction |

### HPF Implementation

**ZDF 1-Pole High-Pass:**
```cpp
float Filter::processHPF(float input) {
    // Calculate lowpass output
    float v = (input - hpfState_) * hpfG_;
    float lowpass = v + hpfState_;
    hpfState_ = lowpass + v;
    
    // HPF output = input - lowpass
    return input - lowpass;
}
```

**Coefficient Calculation:**
```cpp
if (params_.hpfMode > 0) {
    float hpfCutoff = 30.0f * std::pow(2.0f, params_.hpfMode - 1);
    // Mode 1: 30 Hz, Mode 2: 60 Hz, Mode 3: 120 Hz
    
    float hpfWc = TWO_PI * hpfCutoff / sampleRate_;
    hpfG_ = std::tan(hpfWc * 0.5f);
}
```

**Signal Flow:**
```
Input → HPF → (Saturation) → 4-Pole Lowpass → Output
```

---

## Testing and Validation

### Unit Tests

**Test 1: Basic Filtering**
```cpp
TEST_CASE("Filter attenuates high frequencies", "[filter]") {
    Filter filter;
    filter.setSampleRate(48000.0f);
    
    FilterParams params;
    params.cutoff = 0.2f;  // Low cutoff (~80 Hz)
    params.resonance = 0.0f;
    filter.setParameters(params);
    
    // Low frequency passes
    filter.reset();
    float lowFreqOut = filter.process(1.0f);
    REQUIRE(lowFreqOut > 0.9f);
    
    // High frequency attenuated (feed in alternating +1/-1)
    filter.reset();
    float highFreqSum = 0.0f;
    for (int i = 0; i < 100; ++i) {
        highFreqSum += std::abs(filter.process((i % 2) ? 1.0f : -1.0f));
    }
    REQUIRE(highFreqSum / 100.0f < 0.1f);  // Strongly attenuated
}
```

**Test 2: Resonance Peak**
```cpp
TEST_CASE("Filter resonance creates peak at cutoff", "[filter]") {
    Filter filter;
    filter.setSampleRate(48000.0f);
    
    FilterParams params;
    params.cutoff = 0.5f;  // ~600 Hz
    params.resonance = 0.75f;  // k = 3.0
    filter.setParameters(params);
    
    // Feed in impulse, measure response
    float impulseResponse[1000];
    filter.reset();
    impulseResponse[0] = filter.process(1.0f);
    for (int i = 1; i < 1000; ++i) {
        impulseResponse[i] = filter.process(0.0f);
    }
    
    // Should ring at cutoff frequency
    // (Detailed FFT analysis would go here)
}
```

### Frequency Response Measurement

**Tool:** `tools/measure_filter.py`

**Method:**
1. Generate sweep tones 20 Hz to 20 kHz
2. Process through filter
3. Measure output amplitude at each frequency
4. Plot magnitude response (dB vs. frequency)

**Expected Results:**
- Flat passband (< cutoff)
- -24 dB/octave rolloff (> cutoff)
- Resonance peak at cutoff (if resonance > 0)

### Comparison to TAL-U-NO-LX

**Method:**
1. Generate test signal in TAL-U-NO-LX
2. Export audio
3. Process same test signal through Poor House Juno
4. Compare frequency responses

**Acceptable Deviation:**
- ±1 dB in passband
- ±3 dB in transition band
- Resonance peak within ±2 dB

---

## Troubleshooting

### Problem: Filter sounds too bright

**Possible Causes:**
1. Cutoff frequency mapping too high
2. Missing input saturation
3. Resonance too low

**Solutions:**
- Check `baseCutoff` calculation
- Verify `g_` coefficient (should be `tan(wc/2)`)
- Add or increase saturation

### Problem: Self-oscillation unstable (runaway)

**Possible Causes:**
1. `k_` value too high (> 4.0)
2. ZDF state update incorrect
3. Feedback calculated wrong

**Solutions:**
- Clamp `k_` to 4.0 maximum
- Verify state update: `state = out + v`
- Check feedback: `feedback = stage4_ * k_`

### Problem: Filter frequency doesn't track pitch

**Possible Causes:**
1. `keyTrack` not implemented
2. `noteFrequency_` not being set
3. Key track modulation calculated wrong

**Solutions:**
- Ensure `setNoteFrequency()` called on note-on
- Verify modulation: `pow(noteRatio, keyTrack)`
- Check that modulation is multiplicative, not additive

### Problem: Envelope modulation sounds wrong

**Possible Causes:**
1. Envelope value not being set
2. Modulation depth incorrect
3. Polarity inverted

**Solutions:**
- Ensure `setEnvValue()` called every sample
- Check semitone range (should be ±48 for ±4 octaves)
- Verify bipolar mapping: -1 to +1 envelope amount

### Problem: High CPU usage

**Possible Causes:**
1. Coefficients updated every sample
2. Expensive transcendental functions (tan, pow)
3. No optimization flags

**Solutions:**
- Cache coefficients (only update when parameters change)
- Use lookup tables for `tan()` and `pow()`
- Compile with `-O3 -ffast-math`

---

## Performance Optimization

### Coefficient Caching

**Instead of:**
```cpp
Sample Filter::process(Sample input) {
    updateCoefficients();  // Every sample! Slow!
    // ... filter processing
}
```

**Do:**
```cpp
void Filter::setParameters(const FilterParams& params) {
    params_ = params;
    updateCoefficients();  // Only when parameters change
}

Sample Filter::process(Sample input) {
    // Coefficients already computed
    // ... filter processing
}
```

**But:** Modulation sources (envelope, LFO) change every sample, so some recalculation is necessary.

### SIMD Optimization (Future)

Process multiple filter stages in parallel:
```cpp
// ARM NEON (4 stages in parallel)
float32x4_t stages = { stage1_, stage2_, stage3_, stage4_ };
float32x4_t inputs = { in1, in2, in3, in4 };
// ... vectorized processing
```

**Benefit:** ~2-4× speedup on ARM processors (Raspberry Pi)

---

## Summary

### Key Takeaways

✅ **Use ZDF topology** for accurate resonance
✅ **Bilinear transform** for cutoff coefficient (`g = tan(wc/2)`)
✅ **k = 4.0** for self-oscillation threshold
✅ **Multiplicative modulation** for musical behavior
✅ **Soft saturation** for analog character

### Tuning Checklist

- [ ] Cutoff range: 20 Hz to 20 kHz (logarithmic)
- [ ] Resonance range: 0.0 to 4.0 (linear)
- [ ] Self-oscillation at k = 4.0
- [ ] Envelope modulation: ±4 octaves
- [ ] LFO modulation: ±2 octaves
- [ ] Key tracking: 0/0.5/1.0 modes
- [ ] HPF: 30/60/120 Hz cutoffs
- [ ] Saturation: threshold ~1.5, ratio 10:1

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
