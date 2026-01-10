# DSP Design Documentation

**Poor House Juno - Digital Signal Processing Architecture**

---

## Table of Contents

1. [Overview](#overview)
2. [DCO (Digitally Controlled Oscillator)](#dco-digitally-controlled-oscillator)
3. [Filter (IR3109 4-Pole Ladder)](#filter-ir3109-4-pole-ladder)
4. [Envelopes (ADSR)](#envelopes-adsr)
5. [LFO (Low-Frequency Oscillator)](#lfo-low-frequency-oscillator)
6. [Chorus (BBD Emulation)](#chorus-bbd-emulation)
7. [Signal Flow](#signal-flow)
8. [Performance Considerations](#performance-considerations)

---

## Overview

Poor House Juno implements a complete Juno-106 synthesis engine using modern DSP techniques. All audio processing operates at **48 kHz** sample rate with **32-bit float** precision. The design prioritizes accuracy over computational efficiency (within reasonable bounds), with optimization as a secondary concern.

### Design Principles

1. **Accuracy First**: Match the Juno-106/TAL-U-NO-LX behavior as closely as possible
2. **Modern Algorithms**: Use state-of-the-art DSP techniques (polyBLEP, ZDF)
3. **Platform Agnostic**: All DSP code is pure C++17 with no platform dependencies
4. **Real-Time Safe**: No memory allocations or blocking operations in audio thread
5. **Modularity**: Each component is independent and testable

---

## DCO (Digitally Controlled Oscillator)

**File:** `src/dsp/dco.cpp`, `src/dsp/dco.h`

The Juno-106 uses digitally controlled oscillators (DCOs) rather than traditional voltage-controlled oscillators (VCOs). Our implementation closely mimics this behavior while using modern anti-aliasing techniques.

### Waveforms

#### 1. Sawtooth Wave (polyBLEP Anti-Aliasing)

The sawtooth is the primary waveform, generated with polyBLEP (Polynomial Bandlimited Step) anti-aliasing to eliminate aliasing artifacts.

**Algorithm:**
```cpp
Sample Dco::generateSaw(float phase, float phaseInc) {
    // Raw sawtooth: ramp from 0 to 1, then discontinuity back to 0
    float saw = 2.0f * phase - 1.0f;  // Convert to -1 to +1
    
    // Apply polyBLEP at discontinuity (when phase wraps)
    saw -= polyBlep(phase, phaseInc);
    
    return saw;
}
```

**PolyBLEP Function:**
```cpp
float Dco::polyBlep(float t, float dt) {
    // t: normalized phase (0-1)
    // dt: phase increment per sample
    
    // Check if we're near a discontinuity
    if (t < dt) {
        // Rising edge
        t = t / dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        // Falling edge
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}
```

**Theory:**
- PolyBLEP adds a polynomial residual at discontinuities to bandlimit the signal
- This effectively removes aliasing without the computational cost of oversampling
- The residual is a parabolic correction that smooths the step discontinuity

#### 2. Pulse Wave (PWM with polyBLEP)

The pulse wave uses the same polyBLEP technique but with two discontinuities (rising and falling edges).

**Algorithm:**
```cpp
Sample Dco::generatePulse(float phase, float phaseInc, float pulseWidth) {
    // Pulse is 1.0 when phase < pulseWidth, -1.0 otherwise
    float pulse = (phase < pulseWidth) ? 1.0f : -1.0f;
    
    // Apply polyBLEP at rising edge (phase = 0)
    pulse += polyBlep(phase, phaseInc);
    
    // Apply polyBLEP at falling edge (phase = pulseWidth)
    float t = phase - pulseWidth;
    if (t < 0.0f) t += 1.0f;
    pulse -= polyBlep(t, phaseInc);
    
    return pulse;
}
```

**Pulse Width Modulation (PWM):**
- Base pulse width: `params_.pulseWidth` (0.05 to 0.95)
- LFO modulation: ±40% range when enabled
- Clamped to prevent extreme values (5% to 95%)

#### 3. Sub-Oscillator (Square Wave)

Simple square wave one octave below the main oscillator. No anti-aliasing needed due to low frequency.

**Algorithm:**
```cpp
Sample Dco::generateSub(float phase) {
    return (phase < 0.5f) ? 1.0f : -1.0f;
}
```

**Frequency:** Half of main oscillator (runs at 0.5× frequency)

#### 4. White Noise

Simple uniform random distribution noise generator.

**Algorithm:**
```cpp
Sample Dco::generateNoise() {
    return noiseDist_(rng_);  // Uniform distribution [-1, 1]
}
```

### Modulation Sources

#### LFO Modulation

**Pitch Modulation:**
```cpp
float pitchMod = 1.0f;
if (params_.lfoTarget == DcoParams::LFO_PITCH || 
    params_.lfoTarget == DcoParams::LFO_BOTH) {
    // ±1 semitone range
    pitchMod = std::pow(2.0f, lfoValue_ / 12.0f);
}
```

**PWM Modulation:**
```cpp
if (params_.lfoTarget == DcoParams::LFO_PWM ||
    params_.lfoTarget == DcoParams::LFO_BOTH) {
    pulseWidth += lfoValue_ * params_.pwmDepth * 0.4f;  // ±40%
    pulseWidth = clamp(pulseWidth, 0.05f, 0.95f);
}
```

### Pitch Drift Emulation

The Juno-106's DCOs exhibit slight pitch instability due to temperature and component aging. We emulate this with slow random drift.

**Algorithm:**
```cpp
void Dco::updateDrift() {
    driftCounter_++;
    if (driftCounter_ >= DRIFT_UPDATE_SAMPLES) {  // ~100ms updates
        driftCounter_ = 0;
        
        // Set new drift target (±0.5 cents Gaussian distribution)
        driftTarget_ = driftDist_(rng_);
    }
    
    // Smoothly interpolate to target
    const float driftSlew = 0.001f;
    driftAmount_ += (driftTarget_ - driftAmount_) * driftSlew;
}
```

**Parameters:**
- Update rate: Every 4800 samples (~100ms at 48kHz)
- Distribution: Gaussian with σ = 0.5 cents
- Slew rate: 0.1% per sample (smooth transitions)

### Phase Management

**Phase Accumulation:**
```cpp
mainPhase_ += mainPhaseInc_;
if (mainPhase_ >= 1.0f) mainPhase_ -= 1.0f;
```

**Phase Increment Calculation:**
```cpp
mainPhaseInc_ = currentFrequency_ / sampleRate_;
```

**Random Phase Start:**
- On note-on, phase is randomized (Juno characteristic)
- Prevents phasing issues in polyphonic contexts

---

## Filter (IR3109 4-Pole Ladder)

**File:** `src/dsp/filter.cpp`, `src/dsp/filter.h`

The Juno-106 uses the Roland IR3109 chip, which is a 4-pole (24 dB/octave) lowpass ladder filter. We use a **Zero-Delay Feedback (ZDF)** implementation for accurate resonance and self-oscillation.

### Zero-Delay Feedback (ZDF) Topology

Traditional digital ladder filters suffer from a one-sample delay in the feedback path, causing tuning errors at high frequencies. ZDF solves this by computing the feedback instantaneously.

**Theory:**
Based on Vadim Zavalishin's "The Art of VA Filter Design", the ZDF topology computes the output of each stage by solving the feedback equation algebraically.

**Implementation:**
```cpp
Sample Filter::process(Sample input) {
    // Calculate feedback from 4th stage
    float feedback = stage4_ * k_;
    
    // Input with feedback subtraction (instantaneous)
    float inputWithFeedback = input - feedback;
    
    // Process through 4 cascaded 1-pole filters
    float v1 = (inputWithFeedback - stage1_) * g_;
    float out1 = v1 + stage1_;
    stage1_ = out1 + v1;  // State update
    
    float v2 = (out1 - stage2_) * g_;
    float out2 = v2 + stage2_;
    stage2_ = out2 + v2;
    
    float v3 = (out2 - stage3_) * g_;
    float out3 = v3 + stage3_;
    stage3_ = out3 + v3;
    
    float v4 = (out3 - stage4_) * g_;
    float out4 = v4 + stage4_;
    stage4_ = out4 + v4;
    
    return out4;  // 4-pole output
}
```

### Coefficient Calculation

**Cutoff Coefficient (g):**
```cpp
float wc = TWO_PI * cutoffHz / sampleRate_;
g_ = std::tan(wc * 0.5f);  // Bilinear transform
```

**Resonance Coefficient (k):**
```cpp
k_ = params_.resonance * 4.0f;  // 0.0 to 4.0
```

- `k = 0.0`: No resonance
- `k = 4.0`: Self-oscillation threshold
- Values > 4.0 cause instability (intentionally not used)

### Cutoff Frequency Calculation

The cutoff frequency is affected by multiple modulation sources:

```cpp
float Filter::calculateCutoffHz() {
    // Base cutoff (logarithmic 20 Hz to 20 kHz)
    float baseCutoff = 20.0f * std::pow(1000.0f, params_.cutoff);
    
    // Envelope modulation (±48 semitones = 4 octaves)
    float envMod = 1.0f;
    if (params_.envAmount != 0.0f) {
        float envSemitones = params_.envAmount * 48.0f * envValue_;
        envMod = std::pow(2.0f, envSemitones / 12.0f);
    }
    
    // LFO modulation (±24 semitones = 2 octaves)
    float lfoMod = 1.0f;
    if (params_.lfoAmount != 0.0f) {
        float lfoSemitones = params_.lfoAmount * 24.0f * lfoValue_;
        lfoMod = std::pow(2.0f, lfoSemitones / 12.0f);
    }
    
    // Key tracking (0%, 50%, 100%)
    float keyTrackMod = 1.0f;
    if (params_.keyTrack > 0.0f) {
        float noteRatio = noteFrequency_ / 440.0f;  // Relative to A440
        keyTrackMod = std::pow(noteRatio, params_.keyTrack);
    }
    
    // Velocity modulation (±24 semitones)
    float velocityMod = 1.0f;
    if (velocityAmount_ > 0.0f) {
        float velSemitones = (velocityValue_ - 0.5f) * 2.0f * 24.0f * velocityAmount_;
        velocityMod = std::pow(2.0f, velSemitones / 12.0f);
    }
    
    // Combine all modulations multiplicatively
    return baseCutoff * envMod * lfoMod * keyTrackMod * velocityMod;
}
```

### High-Pass Filter (HPF)

**M11 Feature:** 4-position high-pass filter (Off, 30Hz, 60Hz, 120Hz)

**Implementation:**
Simple 1-pole high-pass filter applied before the main lowpass filter.

```cpp
float Filter::processHPF(float input) {
    // 1-pole high-pass filter (ZDF)
    float v = (input - hpfState_) * hpfG_;
    float lowpass = v + hpfState_;
    hpfState_ = lowpass + v;
    
    // HPF output = input - lowpass
    return input - lowpass;
}
```

**Cutoff Frequencies:**
- Mode 0: Off (bypass)
- Mode 1: 30 Hz
- Mode 2: 60 Hz
- Mode 3: 120 Hz

### Saturation

Subtle soft saturation for IR3109 character:

```cpp
float Filter::saturate(float x) {
    // Soft tanh-like saturation
    const float threshold = 1.5f;
    if (x > threshold) {
        return threshold + (x - threshold) * 0.1f;
    } else if (x < -threshold) {
        return -threshold + (x + threshold) * 0.1f;
    }
    return x;
}
```

---

## Envelopes (ADSR)

**File:** `src/dsp/envelope.cpp`, `src/dsp/envelope.h`

Classic Attack-Decay-Sustain-Release envelope with **exponential curves** for natural-sounding transitions.

### Stage State Machine

```
IDLE → ATTACK → DECAY → SUSTAIN → RELEASE → IDLE
  ↑                         ↓
  └─────────────────────────┘
       (note off)
```

### Exponential Curve Implementation

**Coefficient Calculation:**
```cpp
float Envelope::calculateCoefficient(float timeSeconds) {
    if (timeSeconds < 0.001f) timeSeconds = 0.001f;
    
    // Exponential decay coefficient
    // Target: reach 99.9% of target in specified time
    float numSamples = timeSeconds * sampleRate_;
    return std::exp(-6.908f / numSamples);  // ln(1000) ≈ 6.908
}
```

**Explanation:**
- We want: `value(t) = initial + (target - initial) * (1 - e^(-t/τ))`
- At 99.9% completion: `e^(-t/τ) = 0.001`
- Therefore: `t/τ = 6.908`
- Coefficient: `coeff = e^(-1/(τ * sample_rate))`

### Stage Processing

**Attack Stage:**
```cpp
case ATTACK:
    value_ += (1.0f - value_) * (1.0f - attackCoeff_);
    if (value_ >= 0.999f) {
        value_ = 1.0f;
        stage_ = DECAY;
    }
    break;
```

**Decay Stage:**
```cpp
case DECAY:
    targetValue_ = params_.sustain;
    value_ += (targetValue_ - value_) * (1.0f - decayCoeff_);
    if (std::abs(value_ - targetValue_) < 0.001f) {
        value_ = targetValue_;
        stage_ = SUSTAIN;
    }
    break;
```

**Sustain Stage:**
```cpp
case SUSTAIN:
    value_ = params_.sustain;  // Hold constant
    // Wait for note-off
    break;
```

**Release Stage:**
```cpp
case RELEASE:
    value_ += (0.0f - value_) * (1.0f - releaseCoeff_);
    if (value_ < 0.001f) {
        value_ = 0.0f;
        stage_ = IDLE;
    }
    break;
```

### Time Ranges

Matching Juno-106 specifications:

- **Attack:** 1.5ms to 3.0s
- **Decay:** 1.5ms to 12.0s
- **Sustain:** 0.0 to 1.0 (level)
- **Release:** 1.5ms to 12.0s

### Filter Envelope Polarity

**M13 Feature:** Normal or Inverse polarity

- **Normal:** Envelope opens the filter (positive modulation)
- **Inverse:** Envelope closes the filter (negative modulation)
- Implementation: Multiply `envAmount` by -1.0 for inverse mode

---

## LFO (Low-Frequency Oscillator)

**File:** `src/dsp/lfo.cpp`, `src/dsp/lfo.h`

Triangle wave LFO matching the Juno-106's single LFO waveform.

### Triangle Wave Generation

```cpp
float Lfo::process() {
    // Generate triangle wave from phase (0.0 to 1.0)
    float triangle;
    if (phase_ < 0.5f) {
        // Rising edge: 0.0 -> 0.5 maps to -1.0 -> +1.0
        triangle = -1.0f + 4.0f * phase_;
    } else {
        // Falling edge: 0.5 -> 1.0 maps to +1.0 -> -1.0
        triangle = 3.0f - 4.0f * phase_;
    }
    
    // Apply delay scaling
    updateDelayScale();
    triangle *= delayScale_;
    
    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) phase_ -= 1.0f;
    
    return triangle;
}
```

### LFO Delay (M12 Feature)

**Implementation:**
```cpp
void Lfo::updateDelayScale() {
    if (delayTimer_ < delaySeconds_) {
        // Delay period: fade in from 0 to 1
        delayTimer_ += 1.0f / sampleRate_;
        if (delayTimer_ >= delaySeconds_) {
            delayScale_ = 1.0f;  // Full scale
        } else {
            // Linear fade-in during delay
            delayScale_ = delayTimer_ / delaySeconds_;
        }
    }
}
```

**Behavior:**
- Delay timer starts at 0 on note-on
- LFO output scales from 0% to 100% during delay period
- After delay completes, LFO runs at full depth

**Range:** 0 to 3 seconds

### Rate Range

- **Minimum:** 0.1 Hz (10-second cycle)
- **Maximum:** 30.0 Hz (visible vibrato)
- **Control:** Logarithmic scaling for even distribution

---

## Chorus (BBD Emulation)

**File:** `src/dsp/chorus.cpp`, `src/dsp/chorus.h`

The Juno-106's famous chorus uses two MN3009 Bucket Brigade Device (BBD) chips. We emulate this with digital delay lines and LFO modulation.

### BBD Delay Line Architecture

**Structure:**
- Two independent delay lines (Chorus I and Chorus II)
- Each with its own LFO modulation
- Stereo output with different characteristics per channel

### Mode Characteristics

Tuned to match the Juno-106:

| Mode | Delay Time | Modulation Depth | LFO Rate |
|------|------------|------------------|----------|
| **I** | 2.5 ms | 0.5 ms | 0.65 Hz |
| **II** | 4.0 ms | 0.8 ms | 0.50 Hz |
| **I+II** | Both active | Both active | Both LFOs |

**Rationale:**
- Chorus I: Shorter delay, faster rate → brighter, more shimmery
- Chorus II: Longer delay, slower rate → deeper, warmer
- I+II: Rich, complex stereo image

### Delay Line Processing

```cpp
void Chorus::process(Sample input, Sample& leftOut, Sample& rightOut) {
    // Write input to both delay buffers
    delayBuffer1_[delayWritePos_] = input;
    delayBuffer2_[delayWritePos_] = input;
    delayWritePos_ = (delayWritePos_ + 1) % MAX_DELAY_SAMPLES;
    
    // Process based on mode
    if (mode_ == MODE_I || mode_ == MODE_BOTH) {
        // Chorus I: modulated delay
        float lfo1 = getLfoValue(lfo1Phase_);
        float delaySamples1 = (CHORUS_I_DELAY_MS + lfo1 * CHORUS_I_DEPTH_MS) 
                              * sampleRate_ / 1000.0f;
        Sample delayed1 = readDelayLine(delayBuffer1_, delaySamples1);
        
        // Stereo: dry + chorus on left, chorus on right
        leftOut += input * 0.5f + delayed1 * 0.5f;
        rightOut += delayed1;
    }
    
    // Similar for Chorus II...
}
```

### LFO Modulation

**Triangle Wave LFO:**
```cpp
float Chorus::getLfoValue(float phase) const {
    return (phase < 0.5f) ? (-1.0f + 4.0f * phase) 
                          : (3.0f - 4.0f * phase);
}
```

**Phase Update:**
```cpp
lfo1Phase_ += (CHORUS_I_RATE_HZ / sampleRate_);
if (lfo1Phase_ >= 1.0f) lfo1Phase_ -= 1.0f;
```

### Delay Line Interpolation

**Linear Interpolation:**
```cpp
Sample Chorus::readDelayLine(const Sample* buffer, float delaySamples) const {
    int readPos = delayWritePos_ - (int)delaySamples;
    if (readPos < 0) readPos += MAX_DELAY_SAMPLES;
    
    int nextPos = (readPos + 1) % MAX_DELAY_SAMPLES;
    float frac = delaySamples - (int)delaySamples;
    
    // Linear interpolation
    return buffer[readPos] * (1.0f - frac) + buffer[nextPos] * frac;
}
```

**Why Linear Interpolation?**
- BBD chips have inherent low-pass filtering
- High-order interpolation would be too clean
- Linear interpolation adds slight HF roll-off, matching BBD character

---

## Signal Flow

### Per-Voice Signal Path

```
DCO (Saw + Pulse + Sub + Noise)
    ↓
[Waveform Mix]
    ↓
High-Pass Filter (M11)
    ↓
IR3109 4-Pole Lowpass Filter
    ← Filter Envelope
    ← Filter LFO
    ← Key Tracking
    ← Velocity
    ↓
VCA (Amplitude Envelope)
    ← VCA Mode (ENV/GATE)
    ← Velocity
    ↓
Voice Output
```

### Polyphonic Mix & Effects

```
Voice 1 ─┐
Voice 2 ─┤
Voice 3 ─┤ [6-Voice Mix]
Voice 4 ─┤
Voice 5 ─┤
Voice 6 ─┘
    ↓
VCA Level Control (M14)
    ↓
BBD Stereo Chorus
    ↓
[Left Output]  [Right Output]
```

### Modulation Routing

```
LFO ──┬──→ DCO Pitch (±1 semitone)
      ├──→ DCO PWM (±40%)
      └──→ Filter Cutoff (±2 octaves)

Filter Env ──→ Filter Cutoff (±4 octaves, bipolar)
Amp Env ─────→ VCA Gain

Mod Wheel ───→ LFO Depth (real-time control)

Key Tracking ─→ Filter Cutoff (0%, 50%, 100%)

Velocity ─────┬──→ Filter Cutoff (amount variable)
              └──→ Amp Gain (amount variable)
```

---

## Performance Considerations

### Computational Complexity

**Per-Voice Processing (per sample):**
- DCO: ~50 ops (including polyBLEP)
- Filter: ~30 ops (ZDF 4-pole + HPF)
- Envelopes: ~10 ops × 2 = 20 ops
- LFO: ~5 ops
- **Total:** ~105 ops/sample/voice

**6 Voices:** ~630 ops/sample
**Chorus:** ~40 ops/sample (stereo)
**Total:** ~670 ops/sample

**At 48 kHz:**
- ~32M ops/second
- Well within Raspberry Pi 4 capability

### Optimization Opportunities

1. **SIMD (NEON):** Filter and chorus processing can be vectorized
2. **Coefficient Caching:** Filter coefficients don't need update every sample
3. **Lookup Tables:** Exponential functions (envelopes) could use LUTs
4. **Voice Stealing:** Inactive voices should skip processing entirely

### Real-Time Safety

**No Allocations:**
- All buffers pre-allocated
- No `malloc()` or `new` in audio thread

**No Blocking:**
- No mutexes in hot path
- Atomic operations for parameter updates

**Fixed Buffer Sizes:**
- `MAX_DELAY_SAMPLES = 512` (10ms at 48kHz)
- No dynamic memory

---

## References

1. **Vadim Zavalishin** - "The Art of VA Filter Design" (2012)
   - ZDF topology
   - Bilinear transform

2. **Eli Brandt** - "Hard Sync Without Aliasing" (2001)
   - PolyBLEP algorithm

3. **Julius O. Smith III** - Digital Filter Design
   - General DSP theory

4. **Roland Juno-106 Service Manual**
   - Original hardware specifications

5. **TAL-U-NO-LX** (Togu Audio Line)
   - Reference implementation for behavior matching

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
