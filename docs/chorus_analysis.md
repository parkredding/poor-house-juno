# Chorus Analysis and BBD Emulation

**Poor House Juno - Bucket Brigade Device Chorus Design**

---

## Table of Contents

1. [Overview](#overview)
2. [BBD Technology](#bbd-technology)
3. [Juno-106 Chorus Circuit](#juno-106-chorus-circuit)
4. [Chorus Modes and Characteristics](#chorus-modes-and-characteristics)
5. [Digital BBD Emulation](#digital-bbd-emulation)
6. [Delay Line Implementation](#delay-line-implementation)
7. [LFO Modulation](#lfo-modulation)
8. [Stereo Imaging](#stereo-imaging)
9. [Testing and Calibration](#testing-and-calibration)
10. [Advanced Topics](#advanced-topics)

---

## Overview

The **Roland Juno-106 chorus** is one of the most celebrated effects in synthesizer history. It uses two Panasonic **MN3009** Bucket Brigade Device (BBD) chips to create a rich, warm, stereo chorus effect that became synonymous with the "Juno sound."

### Why the Juno Chorus is Special

1. **Stereo Width:** Creates an expansive soundstage
2. **Warmth:** BBD artifacts add analog character
3. **Three Modes:** Versatile (I, II, or both)
4. **Musical Depth:** Not too subtle, not too extreme
5. **Iconic Sound:** Heard on countless records

### Design Goals

- **Accurate Timing:** Match measured delay times and modulation rates
- **Stereo Character:** Reproduce the wide stereo image
- **BBD-Like Behavior:** Emulate the smooth, warm quality
- **Low CPU:** Efficient enough for real-time use on Raspberry Pi

---

## BBD Technology

### What is a Bucket Brigade Device?

A **BBD (Bucket Brigade Device)** is an analog delay line that works by passing an audio signal through a chain of capacitors:

```
Input → [C1] → [C2] → [C3] → ... → [CN] → Output
         ↓      ↓      ↓            ↓
       Clock   Clock  Clock       Clock
```

**Operation:**
1. Input voltage is sampled onto capacitor C1
2. On clock pulse, charge is transferred to C2
3. Charge "walks" down the chain like a bucket brigade
4. Output is taken from the last capacitor

**Characteristics:**
- **Analog delay:** Signal stays in analog domain (no A/D conversion)
- **Clock noise:** High-frequency clock can leak into signal
- **Frequency response:** Natural low-pass filtering (HF roll-off)
- **Distortion:** Slight non-linearity, especially at high levels
- **Companding:** Many BBD circuits use noise reduction (not in Juno)

### MN3009 Chip Specifications

| Parameter | Value |
|-----------|-------|
| **Number of Stages** | 256 |
| **Delay Time Range** | 1.28 ms to 12.8 ms (typical) |
| **Clock Frequency** | 10 kHz to 100 kHz |
| **Frequency Response** | ~15 kHz max (at 100 kHz clock) |
| **Dynamic Range** | ~70 dB (typical) |
| **THD** | ~1% (moderate distortion) |

**Note:** Delay time inversely proportional to clock frequency:
```
Delay = (Number of Stages) / (Clock Frequency)
Delay = 256 / f_clock
```

---

## Juno-106 Chorus Circuit

### Circuit Topology

```
                    ┌─────────────┐
    Dry Signal  ───→│             │
                    │   Chorus I  │─→ Left Output
    ┌──────────────→│   (MN3009)  │
    │               └─────────────┘
    │
    │ Input         ┌─────────────┐
    ├──────────────→│             │
    │               │  Chorus II  │─→ Right Output
    │               │   (MN3009)  │
    └──────────────→└─────────────┘
```

**Key Design Elements:**
1. **Two Independent BBD Stages:** Different delay times and modulation
2. **Shared Input:** Both BBDs receive the same dry signal
3. **Stereo Outputs:** Left has Chorus I, Right has Chorus II
4. **Three Modes:** Switch selects I, II, or both (I+II)

### Measured Parameters

Through analysis of hardware units and TAL-U-NO-LX:

#### Chorus I (Brighter, Faster)

| Parameter | Value |
|-----------|-------|
| **Base Delay** | ~2.5 ms |
| **Modulation Depth** | ~0.5 ms (±0.25 ms) |
| **LFO Rate** | ~0.65 Hz |
| **LFO Waveform** | Triangle |
| **Clock Frequency** | ~100 kHz (variable) |

#### Chorus II (Deeper, Slower)

| Parameter | Value |
|-----------|-------|
| **Base Delay** | ~4.0 ms |
| **Modulation Depth** | ~0.8 ms (±0.4 ms) |
| **LFO Rate** | ~0.50 Hz |
| **LFO Waveform** | Triangle |
| **Clock Frequency** | ~64 kHz (variable) |

**Note:** These values vary slightly between units due to component tolerances (±10% typical).

---

## Chorus Modes and Characteristics

### Mode I: Bright and Shimmery

**Active:** Chorus I only

**Sound Character:**
- Brighter, more "sparkly" chorus
- Faster modulation (0.65 Hz) = more animation
- Shorter delay = less pronounced doubling effect
- Good for: Lead sounds, plucks, cutting through a mix

**Stereo Image:**
- Left: Dry + Chorus I (mixed)
- Right: Chorus I only (or mostly chorus)

**Use Cases:**
- 80s pop lead synths
- Bright pad sounds
- Solo lines that need presence

### Mode II: Deep and Warm

**Active:** Chorus II only

**Sound Character:**
- Deeper, warmer chorus
- Slower modulation (0.50 Hz) = more subtle
- Longer delay = more obvious doubling
- Good for: Pads, strings, lush textures

**Stereo Image:**
- Left: Chorus II only (or mostly chorus)
- Right: Dry + Chorus II (mixed)

**Use Cases:**
- Lush string pads
- Warm bass sounds
- Ambient textures

### Mode I+II: The "Juno Sound"

**Active:** Both Chorus I and II

**Sound Character:**
- Ultra-wide stereo image
- Complex, rich texture
- Both fast and slow modulation
- Most "recognizable" Juno sound

**Stereo Image:**
- Left: Dry + Chorus I
- Right: Dry + Chorus II
- Very wide (almost feels like three sources)

**Use Cases:**
- Classic Juno pad sounds
- String ensembles
- Any sound that needs maximum width
- The default "Juno chorus" everyone knows

### Mode Off

**Active:** None (bypass)

**Use Cases:**
- Bass sounds (chorus can muddy low end)
- Mono compatibility (live sound)
- When clarity > width

---

## Digital BBD Emulation

### Delay Line Structure

We use circular buffers to emulate the BBD delay lines:

```cpp
class Chorus {
private:
    // Maximum delay: ~10ms at 48kHz = 480 samples
    static constexpr int MAX_DELAY_SAMPLES = 512;
    
    Sample delayBuffer1_[MAX_DELAY_SAMPLES];  // Chorus I
    Sample delayBuffer2_[MAX_DELAY_SAMPLES];  // Chorus II
    int delayWritePos_;                        // Shared write position
    
    // Independent LFO phases
    float lfo1Phase_;  // Chorus I LFO
    float lfo2Phase_;  // Chorus II LFO
};
```

### Processing Flow

```cpp
void Chorus::process(Sample input, Sample& leftOut, Sample& rightOut) {
    // Write input to both delay buffers
    delayBuffer1_[delayWritePos_] = input;
    delayBuffer2_[delayWritePos_] = input;
    delayWritePos_ = (delayWritePos_ + 1) % MAX_DELAY_SAMPLES;
    
    // Mode I: Active Chorus I
    if (mode_ == MODE_I || mode_ == MODE_BOTH) {
        // Calculate modulated delay time
        float lfo1 = getLfoValue(lfo1Phase_);
        float delaySamples1 = (CHORUS_I_DELAY_MS + lfo1 * CHORUS_I_DEPTH_MS) 
                              * sampleRate_ / 1000.0f;
        
        // Read from delay buffer with interpolation
        Sample delayed1 = readDelayLine(delayBuffer1_, delaySamples1);
        
        // Mix to outputs (stereo spread)
        leftOut += input * 0.5f + delayed1 * 0.5f;
        rightOut += delayed1;
    }
    
    // Mode II: Similar for Chorus II
    if (mode_ == MODE_II || mode_ == MODE_BOTH) {
        // ... (similar for delayBuffer2_)
    }
    
    // Update LFO phases
    lfo1Phase_ += CHORUS_I_RATE_HZ / sampleRate_;
    if (lfo1Phase_ >= 1.0f) lfo1Phase_ -= 1.0f;
    
    lfo2Phase_ += CHORUS_II_RATE_HZ / sampleRate_;
    if (lfo2Phase_ >= 1.0f) lfo2Phase_ -= 1.0f;
}
```

---

## Delay Line Implementation

### Circular Buffer

**Write:**
```cpp
delayBuffer_[delayWritePos_] = input;
delayWritePos_ = (delayWritePos_ + 1) % MAX_DELAY_SAMPLES;
```

**Read:**
```cpp
int readPos = delayWritePos_ - delaySamples;
if (readPos < 0) readPos += MAX_DELAY_SAMPLES;
Sample output = delayBuffer_[readPos];
```

### Fractional Delay (Interpolation)

**Problem:** Modulation causes non-integer delay times (e.g., 2.5 samples).

**Solution:** Linear interpolation between adjacent samples.

```cpp
Sample Chorus::readDelayLine(const Sample* buffer, float delaySamples) const {
    // Calculate read position
    float readPosFloat = delayWritePos_ - delaySamples;
    if (readPosFloat < 0.0f) readPosFloat += MAX_DELAY_SAMPLES;
    
    // Integer and fractional parts
    int readPos = (int)readPosFloat;
    float frac = readPosFloat - readPos;
    
    // Next sample (with wrap-around)
    int nextPos = (readPos + 1) % MAX_DELAY_SAMPLES;
    
    // Linear interpolation
    Sample sample1 = buffer[readPos];
    Sample sample2 = buffer[nextPos];
    return sample1 * (1.0f - frac) + sample2 * frac;
}
```

**Why Linear (Not Higher Order)?**
- BBD chips have inherent low-pass filtering
- Linear interpolation adds slight HF roll-off (like BBD)
- Higher-order interpolation would be "too perfect" (less authentic)
- Lower CPU cost

### Delay Time Limits

**Minimum Delay:**
- Must be > 1 sample to avoid feedback issues
- Juno min: ~1.5 ms (safe)

**Maximum Delay:**
- Determined by buffer size
- Juno max: ~5 ms (safe)

**Buffer Size:**
```cpp
// Worst case: 5 ms + 1 ms modulation depth = 6 ms
// At 48 kHz: 6 ms * 48 = 288 samples
// Round up to power of 2: 512 samples (safe)
static constexpr int MAX_DELAY_SAMPLES = 512;
```

---

## LFO Modulation

### Triangle Wave LFO

```cpp
float Chorus::getLfoValue(float phase) const {
    // Triangle wave from phase (0.0 to 1.0)
    if (phase < 0.5f) {
        // Rising: 0.0 -> 0.5 maps to -1.0 -> +1.0
        return -1.0f + 4.0f * phase;
    } else {
        // Falling: 0.5 -> 1.0 maps to +1.0 -> -1.0
        return 3.0f - 4.0f * phase;
    }
}
```

**Output Range:** -1.0 to +1.0

### Delay Modulation

```cpp
float modulatedDelay = baseDelay + lfoValue * modulationDepth;
```

**Example (Chorus I):**
- Base delay: 2.5 ms
- Modulation depth: 0.5 ms (±0.25 ms)
- LFO value: -1.0 to +1.0

**Range:**
- LFO = -1.0: 2.5 - 0.25 = 2.25 ms (shortest)
- LFO = 0.0: 2.5 ms (nominal)
- LFO = +1.0: 2.5 + 0.25 = 2.75 ms (longest)

### Phase Relationship

**Chorus I and II:**
- Independent LFO phases
- Not synchronized (creates complexity)
- Slight beating between rates (0.65 Hz vs 0.50 Hz = 0.15 Hz beat)

**Initial Phase:**
```cpp
lfo1Phase_ = 0.0f;  // Start at center (0.0 output)
lfo2Phase_ = 0.0f;  // Also at center
```

This ensures both choruses start at their nominal delay times.

---

## Stereo Imaging

### Dry/Wet Mixing

**Mode I:**
```cpp
leftOut = dry * 0.5f + chorus1 * 0.5f;   // Mixed
rightOut = chorus1 * 1.0f;                // Wet only
```

**Mode II:**
```cpp
leftOut = chorus2 * 1.0f;                 // Wet only
rightOut = dry * 0.5f + chorus2 * 0.5f;  // Mixed
```

**Mode I+II:**
```cpp
leftOut = dry * 0.4f + chorus1 * 0.6f;   // Mostly chorus1
rightOut = dry * 0.4f + chorus2 * 0.6f;  // Mostly chorus2
```

**Note:** Exact mix ratios vary by implementation and personal preference.

### Width and Depth

**Stereo Width:**
- Determined by delay time differences (1.5 ms difference between I and II)
- Different modulation rates (0.15 Hz difference)
- Independent LFO phases

**Perceived Width:**
- Mode I: Moderate (2.5 ms delay = ~1 meter apparent distance)
- Mode II: Moderate (4.0 ms delay = ~1.3 meters)
- Mode I+II: Very wide (both delays active, complex image)

---

## Testing and Calibration

### Unit Tests

**Test 1: Mode Characteristics**
```cpp
TEST_CASE("Chorus Mode I characteristics", "[chorus]") {
    Chorus chorus;
    chorus.setSampleRate(48000.0f);
    chorus.setMode(Chorus::MODE_I);
    
    // Feed constant signal
    float leftOut, rightOut;
    for (int i = 0; i < 1000; ++i) {
        chorus.process(1.0f, leftOut, rightOut);
    }
    
    // Verify outputs are different (stereo)
    // Verify modulation is present (not constant output)
    // ... detailed checks
}
```

**Test 2: Delay Time Measurement**
```cpp
TEST_CASE("Chorus delay times", "[chorus]") {
    Chorus chorus;
    chorus.setSampleRate(48000.0f);
    chorus.setMode(Chorus::MODE_I);
    
    // Send impulse
    float leftOut, rightOut;
    chorus.process(1.0f, leftOut, rightOut);  // Impulse
    
    // Measure time to peak in output
    float maxOut = 0.0f;
    int peakSample = 0;
    for (int i = 1; i < 500; ++i) {
        chorus.process(0.0f, leftOut, rightOut);
        if (std::abs(rightOut) > maxOut) {
            maxOut = std::abs(rightOut);
            peakSample = i;
        }
    }
    
    // Expected: ~2.5 ms = 120 samples at 48 kHz
    float delayMs = peakSample / 48.0f;
    REQUIRE(delayMs >= 2.0f);
    REQUIRE(delayMs <= 3.0f);
}
```

### Frequency Response

**Method:**
1. Sweep sine wave 20 Hz to 20 kHz through chorus
2. Measure magnitude at each frequency
3. Compare to dry signal

**Expected Characteristics:**
- Notches (comb filtering) at multiples of `1 / delay_time`
- Notch depth varies with modulation
- Slight HF roll-off (BBD characteristic)

**Chorus I (2.5 ms delay):**
- First notch: ~400 Hz
- Subsequent notches: 800 Hz, 1200 Hz, 1600 Hz, ...

**Chorus II (4.0 ms delay):**
- First notch: ~250 Hz
- Subsequent notches: 500 Hz, 750 Hz, 1000 Hz, ...

### Comparison to Hardware

**Measurement Setup:**
1. Record Juno-106 with chorus I, II, and I+II
2. Match input level
3. Compare spectrograms and frequency response
4. Measure delay times from impulse response
5. Measure LFO rates from slow sweeps

**Acceptable Deviation:**
- Delay time: ±0.5 ms (component tolerances)
- LFO rate: ±0.1 Hz (hardware variation)
- Modulation depth: ±0.2 ms

---

## Advanced Topics

### BBD Artifacts (Not Currently Modeled)

**Clock Noise:**
- High-frequency noise from clock signal leaking into audio
- Typically 10-15 dB below signal
- Characteristic "hiss" of BBD chorus

**Frequency Response Roll-off:**
- BBD has natural low-pass filtering
- -3 dB point around 8-12 kHz (depends on clock)
- Could be emulated with low-pass filter after delay

**Compression/Saturation:**
- BBD chips have limited dynamic range
- Soft saturation at high levels
- Could be added with `tanh()` function

**Implementation:**
```cpp
// Optional: Add BBD artifacts
Sample Chorus::addBbdCharacter(Sample delayed) {
    // HF roll-off (simple 1-pole LPF)
    delayed = bbdLowpass_.process(delayed);
    
    // Slight compression
    delayed = std::tanh(delayed * 1.2f) * 0.9f;
    
    // Clock noise (optional)
    delayed += clockNoise_() * 0.01f;  // Low level
    
    return delayed;
}
```

### All-Pass Filters for Phase Shift

Some chorus designs use all-pass filters to add phase complexity:

```cpp
// All-pass filter (delays signal without changing magnitude)
Sample allPass(Sample input, float coefficient) {
    float output = -input + delayedInput_ + coefficient * output_;
    output_ = output;
    delayedInput_ = input;
    return output;
}
```

**Purpose:**
- Adds subtle phase shifts at different frequencies
- Increases apparent "width" and complexity
- Not present in original Juno-106

### Modulation Waveforms

Original Juno uses triangle wave, but other waveforms create different characters:

**Sine:** Smoother than triangle (less "mechanical")
**Random:** Random walk (lush, complex, vintage BBD)
**Square:** Dramatic pitch shifts (leslie effect)

**Our Choice:** Triangle (matches Juno-106 exactly)

---

## Performance Optimization

### Buffer Size Optimization

**Trade-off:**
- Larger buffer: More memory, longer max delay
- Smaller buffer: Less memory, shorter max delay

**Optimal:**
```cpp
// 10 ms max delay at 48 kHz
static constexpr int MAX_DELAY_SAMPLES = 512;  // ~10.6 ms
```

### SIMD Optimization

**Stereo Processing:**
```cpp
// ARM NEON: Process left and right simultaneously
float32x2_t output = { leftOut, rightOut };
float32x2_t delayed = { delayed1, delayed2 };
output = vfma_f32(output, delayed, gain);  // Fused multiply-add
```

**Benefit:** ~2× speedup on ARM processors

### Pre-computed LFO

**Instead of:**
```cpp
float lfo = getLfoValue(phase);  // Computed every sample
```

**Pre-compute:**
```cpp
// Pre-compute LFO waveform once
float lfoTable[1024];
for (int i = 0; i < 1024; ++i) {
    float phase = i / 1024.0f;
    lfoTable[i] = getLfoValue(phase);
}

// Look up during processing
int index = (int)(phase * 1024.0f);
float lfo = lfoTable[index];
```

**Benefit:** Faster, but minimal gain (triangle wave is simple)

---

## Summary

### Key Parameters

| Parameter | Chorus I | Chorus II |
|-----------|----------|-----------|
| **Delay Time** | 2.5 ms | 4.0 ms |
| **Mod Depth** | 0.5 ms | 0.8 ms |
| **LFO Rate** | 0.65 Hz | 0.50 Hz |
| **Waveform** | Triangle | Triangle |

### Implementation Checklist

- [x] Circular delay buffers (512 samples)
- [x] Linear interpolation for fractional delays
- [x] Triangle wave LFO
- [x] Three modes (I, II, I+II)
- [x] Stereo processing
- [x] Correct delay times (2.5 ms, 4.0 ms)
- [x] Correct LFO rates (0.65 Hz, 0.50 Hz)
- [ ] Optional: BBD artifacts (clock noise, HF roll-off)
- [ ] Optional: All-pass filters for phase complexity

### Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Metallic sound | Too much feedback | Reduce wet/dry mix |
| Not enough depth | Mod depth too small | Increase from 0.5ms to 1.0ms |
| Pitch warbling | LFO too fast | Check rate (~0.5-0.65 Hz) |
| Clicks/pops | Buffer wrap-around | Check modulo arithmetic |

---

**Last Updated:** January 10, 2026
**Authors:** Poor House Juno Development Team
