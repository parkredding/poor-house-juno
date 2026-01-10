# Poor House Juno - DSP Design Documentation

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Signal Flow](#signal-flow)
3. [DCO (Digitally Controlled Oscillator)](#dco-digitally-controlled-oscillator)
4. [IR3109 4-Pole Ladder Filter](#ir3109-4-pole-ladder-filter)
5. [ADSR Envelope Generators](#adsr-envelope-generators)
6. [Triangle LFO](#triangle-lfo)
7. [BBD Chorus Emulation](#bbd-chorus-emulation)
8. [Voice Architecture](#voice-architecture)
9. [Polyphony and Voice Management](#polyphony-and-voice-management)
10. [Sample Rate Considerations](#sample-rate-considerations)
11. [Performance Optimizations](#performance-optimizations)

---

## Overview

Poor House Juno implements a high-fidelity digital emulation of the Roland Juno-106 synthesizer's DSP chain. The implementation uses modern digital signal processing techniques while maintaining authentic analog character.

**Key Technical Specifications:**
- **Sample Rate:** 48 kHz (primary), 44.1 kHz supported
- **Buffer Size:** 128 samples (2.67 ms @ 48 kHz)
- **Polyphony:** 6 voices
- **Bit Depth:** 32-bit float internal processing
- **Latency:** ~2.7 ms (Pi), ~5-10 ms (Web)

**DSP Algorithm Summary:**
- **Oscillator:** polyBLEP anti-aliasing
- **Filter:** Zero-Delay Feedback (ZDF) topology
- **Envelopes:** Exponential RC curves
- **Chorus:** Digital BBD emulation with linear interpolation

---

## Signal Flow

```
MIDI Note On
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│                    VOICE (x6)                            │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────┐      ┌──────────┐      ┌──────────┐      │
│  │   DCO    │──▶   │  Filter  │──▶   │   VCA    │──▶   │
│  │          │      │ (IR3109) │      │          │      │
│  └──────────┘      └──────────┘      └──────────┘      │
│       │                  ▲                  ▲           │
│       │                  │                  │           │
│       │         ┌────────┴────────┐         │           │
│       │         │  Filter Env     │    ┌────┴─────┐    │
│       │         │  (ADSR)         │    │  Amp Env │    │
│       │         └─────────────────┘    │  (ADSR)  │    │
│       │                                └──────────┘    │
│       │         ┌─────────────────┐                    │
│       └────────▶│  LFO (Global)   │                    │
│                 │  Triangle Wave  │                    │
│                 └─────────────────┘                    │
│                                                          │
└──────────────────────────────┬───────────────────────────┘
                               │
                               ▼
                        ┌─────────────┐
                        │   Mixer     │ (Voice Sum)
                        └──────┬──────┘
                               │
                               ▼
                        ┌─────────────┐
                        │   Chorus    │ (Stereo)
                        │   BBD I/II  │
                        └──────┬──────┘
                               │
                               ▼
                          L/R Output
```

---

## DCO (Digitally Controlled Oscillator)

### Overview

The DCO generates four waveforms that can be mixed:
1. **Sawtooth** - Band-limited with polyBLEP
2. **Pulse** - Variable width (PWM), band-limited
3. **Sub-oscillator** - Square wave, -1 octave
4. **White noise** - Random number generator

**File:** `src/dsp/dco.cpp`, `src/dsp/dco.h`

### polyBLEP Anti-Aliasing

To prevent aliasing artifacts, we use **Polynomial Bandlimited Step (polyBLEP)**, a simple but effective anti-aliasing technique.

**Algorithm:**

```cpp
float Dco::polyBlep(float t, float dt) {
    // t: phase position (0.0 - 1.0)
    // dt: phase increment per sample

    // Handle discontinuity at t = 0
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    // Handle discontinuity at t = 1
    else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}
```

**Sawtooth Generation:**

```cpp
Sample Dco::generateSaw(float phase, float phaseInc) {
    // Naive sawtooth: ramp from -1 to +1
    Sample saw = 2.0f * phase - 1.0f;

    // Apply polyBLEP anti-aliasing at discontinuity
    saw -= polyBlep(phase, phaseInc);

    return saw;
}
```

**Pulse Generation:**

```cpp
Sample Dco::generatePulse(float phase, float phaseInc, float pulseWidth) {
    // Naive pulse wave
    Sample pulse = (phase < pulseWidth) ? 1.0f : -1.0f;

    // Apply polyBLEP at both discontinuities
    pulse += polyBlep(phase, phaseInc);                        // Rising edge at 0
    pulse -= polyBlep(std::fmod(phase + (1.0f - pulseWidth), 1.0f), phaseInc); // Falling edge

    return pulse;
}
```

### Pitch Modulation

The DCO supports multiple pitch modulation sources:

**Frequency Calculation:**

```cpp
void Dco::updatePhaseIncrements() {
    // Range factor (16'/8'/4' footage)
    float rangeFactor = octaveMultipliers[params_.range];  // 0.5, 1.0, 2.0

    // Detune (±cents)
    float detuneFactor = pow(2.0f, params_.detune / 1200.0f);

    // Drift (analog instability emulation)
    float driftFactor = pow(2.0f, driftAmount_ / 1200.0f);

    // LFO pitch modulation (±1 semitone)
    float pitchMod = 1.0f;
    if (params_.lfoTarget == LFO_PITCH || params_.lfoTarget == LFO_BOTH) {
        pitchMod = pow(2.0f, lfoValue_ / 12.0f);
    }

    currentFrequency_ = baseFrequency_ * rangeFactor * detuneFactor * driftFactor * pitchMod;
    mainPhaseInc_ = currentFrequency_ / sampleRate_;
    subPhaseInc_ = (currentFrequency_ * 0.5f) / sampleRate_;  // -1 octave
}
```

### Pitch Drift Emulation

The Juno-106 DCO exhibits subtle pitch instability. We emulate this with a slow random walk:

```cpp
void Dco::updateDrift() {
    if (!params_.enableDrift) return;

    driftCounter_++;

    // Update drift target every ~100ms (4800 samples @ 48kHz)
    if (driftCounter_ >= DRIFT_UPDATE_SAMPLES) {
        driftTarget_ = driftDist_(rng_);  // Normal distribution, ±0.5 cents
        driftCounter_ = 0;
    }

    // Smoothly move towards target (simple low-pass filter)
    float alpha = 0.0001f;  // Very slow drift
    driftAmount_ += alpha * (driftTarget_ - driftAmount_);

    updatePhaseIncrements();
}
```

**Drift Characteristics:**
- **Distribution:** Normal (Gaussian), σ = 0.5 cents
- **Update Rate:** ~10 Hz (every 100 ms)
- **Slew Rate:** α = 0.0001 (very slow)

### PWM (Pulse Width Modulation)

Pulse width can be modulated by the LFO:

```cpp
float pulseWidth = params_.pulseWidth;
if (params_.lfoTarget == LFO_PWM || params_.lfoTarget == LFO_BOTH) {
    pulseWidth += lfoValue_ * params_.pwmDepth * 0.4f;  // ±40% range
    pulseWidth = clamp(pulseWidth, 0.05f, 0.95f);
}
```

---

## IR3109 4-Pole Ladder Filter

### Overview

The IR3109 is a 4-pole (24 dB/octave) lowpass ladder filter, similar to the Moog ladder but with subtle differences. We implement it using **Zero-Delay Feedback (ZDF)** topology for accurate resonance and self-oscillation.

**File:** `src/dsp/filter.cpp`, `src/dsp/filter.h`

### ZDF Topology

Traditional digital ladder filters suffer from delay in the feedback path, causing resonance inaccuracy. ZDF topology solves this using implicit equations.

**Algorithm (per sample):**

```cpp
Sample Filter::process(Sample input) {
    // Apply HPF first (if enabled)
    if (params_.hpfMode > 0) {
        input = processHPF(input);
    }

    // Apply input drive/saturation
    input *= params_.drive;
    input = saturate(input);

    // Calculate feedback amount
    float feedback = stage4_ * k_;

    // Input with feedback subtraction
    float inputWithFeedback = input - feedback;

    // Process through 4 stages (ZDF integration)
    float v1 = (inputWithFeedback - stage1_) * g_;
    float out1 = v1 + stage1_;
    stage1_ = out1 + v1;

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
// Calculate cutoff frequency in Hz
float cutoffHz = calculateCutoffHz();

// Calculate g coefficient (normalized cutoff)
// Uses bilinear transform: g = tan(π * fc / fs)
float wc = TWO_PI * cutoffHz / sampleRate_;
g_ = std::tan(wc * 0.5f);
```

**Resonance Coefficient (k):**

```cpp
// Map resonance parameter (0-1) to k (0-4)
// k = 4.0 gives self-oscillation
k_ = params_.resonance * 4.0f;
```

### Cutoff Frequency Mapping

The filter cutoff is controlled by multiple modulation sources:

```cpp
float Filter::calculateCutoffHz() {
    // Base cutoff (logarithmic: 20 Hz to 20 kHz)
    float baseCutoff = 20.0f * pow(1000.0f, params_.cutoff);

    // Envelope modulation (bipolar: ±48 semitones = 4 octaves)
    float envMod = 1.0f;
    if (params_.envAmount != 0.0f) {
        float envSemitones = params_.envAmount * 48.0f * envValue_;
        envMod = pow(2.0f, envSemitones / 12.0f);
    }

    // LFO modulation (±24 semitones = 2 octaves)
    float lfoMod = 1.0f;
    if (params_.lfoAmount > 0.0f) {
        float lfoSemitones = lfoValue_ * params_.lfoAmount * 24.0f;
        lfoMod = pow(2.0f, lfoSemitones / 12.0f);
    }

    // Key tracking (Off, Half, Full)
    float keyTrackMod = 1.0f;
    if (params_.keyTrack != KEY_TRACK_OFF) {
        float ratio = noteFrequency_ / 440.0f;
        if (params_.keyTrack == KEY_TRACK_HALF) {
            keyTrackMod = sqrt(ratio);  // 50% tracking
        } else {
            keyTrackMod = ratio;  // 100% tracking
        }
    }

    // Velocity modulation (±24 semitones)
    float velocityMod = 1.0f;
    if (velocityAmount_ > 0.0f) {
        float velocitySemitones = (velocityValue_ - 0.5f) * 2.0f * velocityAmount_ * 24.0f;
        velocityMod = pow(2.0f, velocitySemitones / 12.0f);
    }

    // Combine all modulations (multiplicative)
    return baseCutoff * envMod * lfoMod * keyTrackMod * velocityMod;
}
```

### Saturation Characteristics

The IR3109 exhibits subtle soft saturation at high input levels:

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

### High-Pass Filter (M11)

A 1-pole high-pass filter is available with 4 cutoff frequencies:

```cpp
float Filter::processHPF(float input) {
    // 1-pole high-pass filter using ZDF topology
    // HPF(s) = s / (s + wc)
    float v = (input - hpfState_) * hpfG_;
    float lp = v + hpfState_;
    hpfState_ = lp + v;

    // High-pass output = input - lowpass
    return input - lp;
}
```

**HPF Cutoff Frequencies:**
- Mode 0: Off
- Mode 1: 30 Hz
- Mode 2: 60 Hz
- Mode 3: 120 Hz

### Performance Optimization (M15)

To reduce CPU usage, filter coefficients are only recalculated when modulation values change:

```cpp
void Filter::updateCoefficientsIfNeeded() {
    const float epsilon = 0.0001f;

    bool envChanged = abs(envValue_ - cachedEnvValue_) > epsilon;
    bool lfoChanged = abs(lfoValue_ - cachedLfoValue_) > epsilon;
    bool velocityChanged = abs(velocityValue_ - cachedVelocityValue_) > epsilon;

    if (coefficientsNeedUpdate_ || envChanged || lfoChanged || velocityChanged) {
        updateCoefficients();
        cachedEnvValue_ = envValue_;
        cachedLfoValue_ = lfoValue_;
        cachedVelocityValue_ = velocityValue_;
        coefficientsNeedUpdate_ = false;
    }
}
```

**Impact:** Reduces expensive `tan()` and `pow()` calls from 48,000/sec to ~hundreds/sec.

---

## ADSR Envelope Generators

### Overview

Two ADSR (Attack-Decay-Sustain-Release) envelopes control filter cutoff and amplitude. They use exponential curves matching analog RC circuits.

**File:** `src/dsp/envelope.cpp`, `src/dsp/envelope.h`

### Exponential Curve Algorithm

```cpp
float Envelope::process() {
    switch (stage_) {
        case ATTACK:
            // Exponential rise to 1.0
            value_ += (targetValue_ - value_) * attackCoeff_;
            if (value_ >= 0.999f) {
                value_ = 1.0f;
                stage_ = DECAY;
                targetValue_ = params_.sustain;
            }
            break;

        case DECAY:
            // Exponential fall to sustain level
            value_ += (targetValue_ - value_) * decayCoeff_;
            if (abs(value_ - targetValue_) < 0.001f) {
                value_ = targetValue_;
                stage_ = SUSTAIN;
            }
            break;

        case SUSTAIN:
            // Hold at sustain level
            value_ = params_.sustain;
            break;

        case RELEASE:
            // Exponential fall to 0.0
            value_ += (targetValue_ - value_) * releaseCoeff_;
            if (value_ <= 0.001f) {
                value_ = 0.0f;
                stage_ = IDLE;
            }
            break;
    }

    return clamp(value_, 0.0f, 1.0f);
}
```

### Coefficient Calculation

Each stage uses a coefficient derived from the desired time constant:

```cpp
float Envelope::calculateCoefficient(float timeSeconds) {
    // Formula: coeff = 1 - exp(-4.6 / (time * sampleRate))
    // This reaches approximately 99% of target in the specified time

    if (timeSeconds <= 0.0f) {
        return 1.0f;  // Instant
    }

    float samples = timeSeconds * sampleRate_;
    // Use 4.6 time constants to reach 99% in the specified time
    return 1.0f - std::exp(-4.6f / samples);
}
```

**Time Constant Explanation:**

For an RC circuit, voltage reaches 99% of target in ~4.6 time constants:
- 1τ: 63.2%
- 2τ: 86.5%
- 3τ: 95.0%
- 4τ: 98.2%
- 4.6τ: 99.0%

### Parameter Ranges

| Stage   | Min Time | Max Time | Default |
|---------|----------|----------|---------|
| Attack  | 1 ms     | 3 s      | 10 ms   |
| Decay   | 2 ms     | 12 s     | 300 ms  |
| Sustain | 0.0      | 1.0      | 0.7     |
| Release | 2 ms     | 12 s     | 500 ms  |

---

## Triangle LFO

### Overview

The LFO generates a triangle wave for modulating pitch, PWM, and filter cutoff. It includes a delay feature (M12) that fades in the modulation after note-on.

**File:** `src/dsp/lfo.cpp`, `src/dsp/lfo.h`

### Triangle Wave Generation

```cpp
float Lfo::process() {
    // Update delay timer and scale
    updateDelayScale();

    // Generate triangle wave from phase
    float value;
    if (phase_ < 0.25f) {
        // Rising: 0.0 -> 0.25 maps to 0.0 -> +1.0
        value = phase_ * 4.0f;
    } else if (phase_ < 0.75f) {
        // Falling: 0.25 -> 0.75 maps to +1.0 -> -1.0
        value = 2.0f - (phase_ * 4.0f);
    } else {
        // Rising: 0.75 -> 1.0 maps to -1.0 -> 0.0
        value = -4.0f + (phase_ * 4.0f);
    }

    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }

    // Apply delay scale (fade in LFO after delay)
    return clamp(value * delayScale_, -1.0f, 1.0f);
}
```

### LFO Delay Feature (M12)

The delay feature fades in the LFO amplitude after a specified time:

```cpp
void Lfo::updateDelayScale() {
    if (delaySeconds_ <= 0.0f) {
        delayScale_ = 1.0f;
        return;
    }

    if (delayTimer_ < delaySeconds_) {
        // During delay period, fade in from 0 to 1
        delayTimer_ += 1.0f / sampleRate_;
        if (delayTimer_ >= delaySeconds_) {
            delayScale_ = 1.0f;
            delayTimer_ = delaySeconds_;
        } else {
            // Linear fade-in
            delayScale_ = delayTimer_ / delaySeconds_;
        }
    } else {
        delayScale_ = 1.0f;
    }
}
```

**Triggering:**

The delay timer is triggered on note-on events:

```cpp
void Lfo::trigger() {
    delayTimer_ = 0.0f;
    delayScale_ = (delaySeconds_ > 0.0f) ? 0.0f : 1.0f;
}
```

---

## BBD Chorus Emulation

### Overview

The Juno-106 uses two MN3009 Bucket Brigade Device (BBD) chips to create its signature stereo chorus. We emulate this with digital delay lines and triangle LFO modulation.

**File:** `src/dsp/chorus.cpp`, `src/dsp/chorus.h`

### BBD Parameters

Two chorus modes with different characteristics:

**Chorus I (Shorter, Faster):**
```cpp
static constexpr float CHORUS_I_DELAY_MS = 2.5f;   // Base delay
static constexpr float CHORUS_I_DEPTH_MS = 0.5f;   // Modulation depth
static constexpr float CHORUS_I_RATE_HZ = 0.65f;   // Modulation rate
```

**Chorus II (Longer, Slower):**
```cpp
static constexpr float CHORUS_II_DELAY_MS = 4.0f;   // Base delay
static constexpr float CHORUS_II_DEPTH_MS = 0.8f;   // Modulation depth
static constexpr float CHORUS_II_RATE_HZ = 0.50f;   // Modulation rate
```

### Delay Line with Linear Interpolation

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

### Stereo Processing

Each chorus mode uses opposite LFO phases for left and right channels:

```cpp
void Chorus::process(Sample input, Sample& leftOut, Sample& rightOut) {
    // Write input to delay lines
    delayBuffer1_[delayWritePos_] = input;
    delayBuffer2_[delayWritePos_] = input;
    delayWritePos_ = (delayWritePos_ + 1) % MAX_DELAY_SAMPLES;

    if (mode_ == MODE_I || mode_ == MODE_BOTH) {
        float lfo1 = getLfoValue(lfo1Phase_);
        lfo1Phase_ += CHORUS_I_RATE_HZ / sampleRate_;
        if (lfo1Phase_ >= 1.0f) lfo1Phase_ -= 1.0f;

        float baseDelay1 = CHORUS_I_DELAY_MS * sampleRate_ / 1000.0f;
        float depthSamples1 = CHORUS_I_DEPTH_MS * sampleRate_ / 1000.0f;

        // Opposite phases for stereo width
        float delayLeft1 = baseDelay1 + lfo1 * depthSamples1;
        float delayRight1 = baseDelay1 - lfo1 * depthSamples1;

        chorus1Left = readDelayLine(delayBuffer1_, delayLeft1);
        chorus1Right = readDelayLine(delayBuffer1_, delayRight1);
    }

    // Mix dry and wet signals
    constexpr float dryLevel = 0.8f;
    constexpr float wetLevel = 0.2f;

    leftOut = dryLevel * input + wetLevel * chorus1Left;
    rightOut = dryLevel * input + wetLevel * chorus1Right;
}
```

### Mode Behavior

- **Mode I:** Chorus I only (subtle, faster)
- **Mode II:** Chorus II only (deeper, slower)
- **Mode I+II:** Both circuits active (lush, complex)

---

## Voice Architecture

Each voice combines DCO, filter, and envelopes into a complete signal chain.

**File:** `src/dsp/voice.cpp`, `src/dsp/voice.h`

### Voice Processing

```cpp
Sample Voice::process() {
    // Update portamento glide
    updateGlide();

    // Calculate final frequency with pitch bend and master tune
    float pitchBendSemitones = pitchBend_ * pitchBendRange_;
    float pitchBendRatio = pow(2.0f, pitchBendSemitones / 12.0f);
    float masterTuneRatio = pow(2.0f, masterTune_ / 1200.0f);
    float finalFreq = currentFreq_ * pitchBendRatio * masterTuneRatio;

    dco_.setFrequency(finalFreq);

    // Process envelopes
    float filterEnvValue = filterEnv_.process();
    float ampEnvValue = ampEnv_.process();

    // Apply filter envelope polarity (M13)
    float filterModValue = (filterEnvPolarity_ == 1) ?
        (1.0f - filterEnvValue) : filterEnvValue;

    filter_.setEnvValue(filterModValue);
    filter_.setVelocityValue(velocity_, velocityToFilter_);

    // Generate oscillator output
    Sample dcoOut = dco_.process();

    // Process through filter
    Sample filtered = filter_.process(dcoOut);

    // Apply VCA
    float vcaGain = (vcaMode_ == 1) ?
        (noteActive_ ? 1.0f : 0.0f) : ampEnvValue;

    // Apply velocity to amplitude
    float velocityGain = 1.0f - velocityToAmp_ + (velocityToAmp_ * velocity_);

    return filtered * vcaLevel_ * vcaGain * velocityGain;
}
```

### Portamento (Glide)

```cpp
void Voice::updateGlide() {
    if (glideRate_ != 0.0f) {
        // Check if we've reached the target
        if ((glideRate_ > 0.0f && currentFreq_ >= targetFreq_) ||
            (glideRate_ < 0.0f && currentFreq_ <= targetFreq_)) {
            currentFreq_ = targetFreq_;
            glideRate_ = 0.0f;
        } else {
            currentFreq_ += glideRate_;
        }
    }
}
```

---

## Polyphony and Voice Management

The Synth class manages 6 voices with intelligent voice stealing.

**File:** `src/dsp/synth.cpp`, `src/dsp/synth.h`

### Voice Allocation

```cpp
void Synth::handleNoteOn(int midiNote, float velocity) {
    // Find a free voice
    int voiceIndex = findFreeVoice();

    if (voiceIndex == -1) {
        // No free voice, need to steal one
        voiceIndex = findVoiceToSteal();
    }

    if (voiceIndex != -1) {
        voices_[voiceIndex].noteOn(midiNote, velocity);
        lfo_.trigger();  // Trigger LFO delay
    }
}
```

### Voice Stealing Algorithm

```cpp
int Synth::findVoiceToSteal() const {
    int oldestReleasingVoice = -1;
    float oldestReleasingAge = -1.0f;
    int oldestVoice = -1;
    float oldestAge = -1.0f;

    for (int i = 0; i < NUM_VOICES; ++i) {
        float age = voices_[i].getAge();

        if (voices_[i].isReleasing()) {
            // Prefer releasing voices
            if (age > oldestReleasingAge) {
                oldestReleasingAge = age;
                oldestReleasingVoice = i;
            }
        } else if (voices_[i].isActive()) {
            if (age > oldestAge) {
                oldestAge = age;
                oldestVoice = i;
            }
        }
    }

    // Prefer stealing releasing voices
    return (oldestReleasingVoice != -1) ? oldestReleasingVoice : oldestVoice;
}
```

---

## Sample Rate Considerations

### Anti-Aliasing

- **Oscillators:** polyBLEP provides good anti-aliasing up to ~12 kHz fundamental
- **Filter:** Natural low-pass characteristic prevents aliasing
- **Chorus:** Linear interpolation provides adequate anti-aliasing for modulated delays

### Nyquist Considerations

At 48 kHz sample rate:
- **Nyquist frequency:** 24 kHz
- **Usable bandwidth:** ~20 kHz (leaves headroom)
- **Filter cutoff max:** 20 kHz (clamped in code)

---

## Performance Optimizations

### M15 Optimizations

1. **Filter Coefficient Caching**
   - Only recalculate when modulation changes
   - Saves expensive `tan()` and `pow()` calls
   - ~30-50% reduction in filter CPU usage

2. **Inline Functions**
   - Critical DSP functions marked `inline`
   - Reduces function call overhead

3. **Compiler Optimizations**
   - `-O3` maximum optimization
   - `-ffast-math` for relaxed floating-point
   - `-march=native` for platform-specific SIMD

### Future Optimization Opportunities

1. **SIMD Voice Processing** - Process multiple voices in parallel
2. **NEON Intrinsics** - ARM SIMD for filter and chorus
3. **Lookup Tables** - Pre-compute expensive functions
4. **Fixed-Point** - Consider for embedded platforms

---

## References

- Zavalishin, V. (2012). "The Art of VA Filter Design" - Native Instruments
- Smith, J.O. "Introduction to Digital Filters" - CCRMA, Stanford
- Stilson, T. & Smith, J.O. (1996). "Alias-Free Digital Synthesis of Classic Analog Waveforms"
- Parker, J. et al. (2013). "Efficient dispersion filter structures"
- Roland Juno-106 Service Manual

---

**Last Updated:** January 10, 2026
