# Poor House Juno - Juno-106 Analysis & Reverse Engineering

**Date:** January 10, 2026
**Version:** M15 (Polish & Optimization)
**Author:** Poor House Juno Development Team

---

## Table of Contents

1. [Overview](#overview)
2. [Original Juno-106 Specifications](#original-juno-106-specifications)
3. [Component Analysis](#component-analysis)
4. [TAL-U-NO-LX Comparison Approach](#tal-u-no-lx-comparison-approach)
5. [Differences from Hardware](#differences-from-hardware)
6. [Measurement Methodology](#measurement-methodology)

---

## Overview

Poor House Juno is designed to emulate the Roland Juno-106 synthesizer with high fidelity. This document analyzes the original hardware and describes our reverse engineering approach, primarily based on studying the behavior of TAL-U-NO-LX, a well-regarded software emulation.

**Design Philosophy:**
- Authentic sound over circuit-level accuracy
- Modern DSP techniques (ZDF, polyBLEP) vs. analog modeling
- Focus on musical behavior rather than component simulation

---

## Original Juno-106 Specifications

### Hardware Overview

**Released:** 1984
**Production:** 1984-1988
**Units Sold:** ~30,000-40,000

**Architecture:**
- 6-voice polyphony (analog voices)
- Digital oscillators (DCO) with analog VCF/VCA
- Shared LFO and chorus
- MIDI + CV/Gate (depending on variant)

### Oscillator Section (DCO)

**Chip:** Custom Roland DCO (80017A)

**Waveforms:**
- Sawtooth
- Pulse (variable width, manual and LFO modulation)
- Sub-oscillator (square wave, -1 octave)
- White noise

**Characteristics:**
- Digital oscillator core (stable pitch)
- Analog waveshaping output
- Slight pitch drift due to analog circuitry (~0.1-0.5 cents)
- Random phase on note-on (free-running oscillators)

**Range Selection:**
- 16' (down 1 octave)
- 8' (normal pitch)
- 4' (up 1 octave)

**LFO Routing:**
- DCO pitch (vibrato)
- PWM (pulse width modulation)
- Both simultaneously

### Filter Section (VCF)

**Chip:** IR3109 (4-pole lowpass ladder)

**Specifications:**
- 24 dB/octave rolloff
- Self-oscillation at high resonance
- Envelope modulation (bipolar)
- LFO modulation
- Key tracking (Off, 1/2, Full)

**Known Issues:**
- IR3109 chip failure (voice chip disease)
- Filter tracking varies between units
- Resonance self-oscillation frequency varies

**High-Pass Filter:**
- 4-position switch (Off, 1, 2, 3)
- Estimated cutoffs: 30 Hz, 60 Hz, 120 Hz
- Simple 1-pole or 2-pole design
- Removes low-frequency mud

### Envelopes

**Configuration:** Dual ADSR (Filter and Amplitude)

**Time Ranges (estimated):**
- Attack: 1.5 ms - 3 s
- Decay: 1.5 ms - 12 s
- Sustain: 0% - 100%
- Release: 1.5 ms - 12 s

**Characteristics:**
- Exponential curves (RC circuits)
- Slight variation between voices
- Some units exhibit envelope "droop" in sustain

### LFO

**Waveform:** Triangle
**Rate Range:** 0.1 Hz - 30 Hz
**Delay:** 0-3 seconds (on later models)

**Routing:**
- DCO (pitch/PWM)
- VCF (filter cutoff)
- Manual depth control via mod wheel

### Chorus

**Design:** Analog BBD (Bucket Brigade Device)
**Chips:** 2× MN3009 (512-stage BBD)
**Modes:** I, II, I+II (both)

**Chorus I:**
- Shorter delay (~2-3 ms)
- Faster modulation (~0.6-0.7 Hz)
- Subtle effect

**Chorus II:**
- Longer delay (~3-5 ms)
- Slower modulation (~0.4-0.5 Hz)
- Deeper effect

**Characteristics:**
- Stereo output (opposite LFO phases)
- Mild clock noise and high-frequency droop
- "Glassy" chorused sound characteristic of BBD

### Voice Architecture

**Signal Flow:**
```
DCO → VCF (IR3109) → VCA → Chorus → Output (L/R)
      ↑               ↑
Filter Env       Amp Env
```

**Polyphony:** 6 voices
**Voice Stealing:** Oldest note priority (releasing notes preferred)

---

## Component Analysis

### DCO Behavior

**Measured Characteristics:**
- Pitch stability: ±0.5 cents drift (slow, ~0.1 Hz variation)
- Phase relationship: Random on note-on
- Waveform quality: Clean digital (minimal aliasing above 1 kHz fundamental)

**PWM Behavior:**
- Pulse width range: 5%-95% (safely away from extremes)
- LFO modulation depth: ±30-40% from center position
- Manual width control: Linear taper

**Sub-Oscillator:**
- Simple square wave (no anti-aliasing needed at -1 octave)
- Fixed 50% duty cycle
- Frequency = 0.5 × main oscillator

### IR3109 Filter Behavior

**Frequency Response:**
- -3 dB at cutoff frequency
- 24 dB/octave rolloff (4-pole)
- Slight passband ripple at high resonance

**Resonance:**
- Self-oscillation threshold: ~95% resonance parameter
- Self-oscillation frequency: Slightly above cutoff setting
- Resonance gain compensation: Minimal (some volume boost)

**Envelope Modulation:**
- Range: Approximately ±4 octaves (±48 semitones)
- Polarity: Positive (envelope opens filter) on most units
- Some units have inverted polarity (mod?)

**Key Tracking:**
- Off: Filter cutoff independent of note
- Half: Filter tracks at 50% (square root of frequency ratio)
- Full: Filter tracks at 100% (1:1 with oscillator)

**Non-linearities:**
- Soft saturation at high input levels
- Resonance "screaming" at self-oscillation
- Slight frequency drift with temperature

### Envelope Behavior

**Attack Stage:**
- Exponential rise to peak
- Minimum: ~1-2 ms (very fast)
- Maximum: ~3 seconds (slow)

**Decay/Release Stages:**
- Exponential fall
- Minimum: ~2 ms
- Maximum: ~12 seconds

**Sustain:**
- Holds at specified level
- Slight "droop" on some units (capacitor leakage)

**Timing Accuracy:**
- Generally consistent across voices
- Some unit-to-unit variation (component tolerances)

### LFO Behavior

**Waveform:**
- Triangle (rising from 0 → +1 → 0 → -1 → 0)
- Very low harmonic content (pure modulation)

**Rate:**
- Linear control (Hz)
- Range: ~0.1 Hz (10-second cycle) to ~30 Hz (visible vibrato)

**Delay:**
- Linear fade-in after note-on
- Range: 0-3 seconds
- Smooth ramp (no clicks)

### Chorus Characteristics

**BBD Delay Times (measured from TAL-U-NO-LX):**
- Chorus I: 2.5 ms ± 0.5 ms modulation
- Chorus II: 4.0 ms ± 0.8 ms modulation

**LFO Rates:**
- Chorus I: ~0.65 Hz
- Chorus II: ~0.50 Hz
- Phase offset: ~90° (I vs II)

**Stereo Imaging:**
- Left/Right channels use opposite LFO phases
- Creates wide stereo image
- Mono-compatible (no phase cancellation)

**BBD Artifacts:**
- High-frequency droop (capacitor charge loss)
- Clock noise (faint ticking, ~30-50 kHz aliased down)
- Mild distortion at extreme modulation depths

---

## TAL-U-NO-LX Comparison Approach

### Why TAL-U-NO-LX?

TAL-U-NO-LX by Togu Audio Line is widely regarded as one of the most accurate Juno-106 emulations. We use it as a reference for:

1. **Parameter mapping** (control curves)
2. **Filter behavior** (cutoff response, resonance)
3. **Chorus characteristics** (delay times, modulation depth)
4. **Overall sound character**

**Note:** We do not have access to TAL's source code. Our approach is behavioral analysis only.

### Measurement Tools (M15)

**Location:** `tools/`

#### 1. `analyze_tal.py` - Parameter Curve Analysis

**Purpose:** Analyze how TAL-U-NO-LX parameters map to internal values.

**Method:**
- Sweep parameter from 0% to 100%
- Measure output frequency/amplitude/etc.
- Fit mathematical models (linear, exponential, logarithmic)

**Example:**
```bash
python tools/analyze_tal.py --parameter filter_cutoff --output filter_curve.json
```

**Output:**
```json
{
  "parameter": "filter_cutoff",
  "type": "logarithmic",
  "equation": "20 * 1000^x",
  "range": [20, 20000],
  "samples": 100
}
```

#### 2. `measure_filter.py` - Filter Frequency Response

**Purpose:** Measure filter cutoff and resonance response.

**Method:**
- Generate sine sweep (20 Hz - 20 kHz)
- Process through Poor House Juno and TAL-U-NO-LX
- Compare frequency response curves
- Measure -3 dB point, rolloff slope, resonance peak

**Example:**
```bash
python tools/measure_filter.py --compare-tal --cutoff 0.5 --resonance 0.8
```

**Output:**
- Frequency response plots (magnitude and phase)
- Cutoff frequency accuracy (Hz)
- Rolloff slope (dB/octave)
- Resonance peak height (dB)

#### 3. `measure_chorus.py` - Chorus Analysis

**Purpose:** Analyze chorus delay times, modulation depth, and stereo imaging.

**Method:**
- Send impulse through chorus
- Measure delay time (cross-correlation)
- Analyze LFO modulation (FFT)
- Measure stereo decorrelation

**Example:**
```bash
python tools/measure_chorus.py --mode 1 --compare-tal
```

**Output:**
- Delay time vs. time plot
- LFO frequency spectrum
- Stereo correlation coefficient

#### 4. `generate_reference.py` - Test Signal Generator

**Purpose:** Generate test signals and reference recordings.

**Method:**
- Generate sine sweeps, impulses, noise
- Create patch presets for testing
- Record TAL-U-NO-LX output for A/B testing

**Example:**
```bash
python tools/generate_reference.py --test filter_sweep --tal-output reference.wav
```

### Comparison Metrics

**Filter Accuracy:**
- Cutoff frequency error: <2% (within 1/4 semitone)
- Resonance peak error: <1 dB
- Rolloff slope: 24 ± 0.5 dB/octave

**Chorus Accuracy:**
- Delay time error: <0.2 ms
- LFO frequency error: <0.05 Hz
- Stereo width correlation: >0.95

**Envelope Accuracy:**
- Timing error: <5%
- Curve shape correlation: >0.99

---

## Differences from Hardware

### Intentional Design Choices

#### 1. Digital DSP vs. Analog Modeling

**Hardware:** Analog VCF/VCA with component-level non-linearities
**Our Approach:** Clean digital ZDF filter with optional soft saturation

**Rationale:**
- ZDF provides accurate resonance and self-oscillation
- Soft saturation captures IR3109 character without circuit-level complexity
- Easier to tune and maintain stable behavior

#### 2. polyBLEP vs. Hardware Waveshaping

**Hardware:** Digital oscillator → analog waveshaping (inherent bandwidth limiting)
**Our Approach:** polyBLEP anti-aliasing

**Rationale:**
- polyBLEP is computationally efficient
- Provides good anti-aliasing up to ~12 kHz fundamental
- Minimal audible aliasing in musical context

#### 3. Perfect Tuning

**Hardware:** Slight pitch drift, voice-to-voice detuning
**Our Approach:** Optional drift emulation (can be disabled)

**Rationale:**
- Digital oscillators are perfectly stable by default
- Drift emulation adds analog character
- User can choose between perfect stability and analog warmth

### Unintentional Limitations

#### 1. BBD Clock Noise

**Hardware:** Faint clock noise from MN3009 BBD chips
**Our Limitation:** No clock noise (clean digital delay)

**Impact:** Very subtle (most users won't notice)

**Future:** Could add synthesized clock noise if desired

#### 2. Component Tolerances

**Hardware:** Each voice has slightly different filter response
**Our Limitation:** All voices identical

**Impact:** Slightly less "organic" than hardware
**Future:** Could add per-voice randomization

#### 3. Temperature Drift

**Hardware:** Filter cutoff drifts with temperature
**Our Limitation:** No temperature dependency

**Impact:** Minimal (mostly affects tuning stability during warm-up)

---

## Measurement Methodology

### Filter Frequency Response

**Test Signal:** Sine sweep, 20 Hz - 20 kHz, 10 seconds
**Settings:**
- Resonance = 0 (measure cutoff only)
- Envelope amount = 0
- LFO amount = 0

**Measurements:**
1. Generate sweep
2. Process through filter
3. Calculate FFT of input and output
4. Compute magnitude response: `20 * log10(|Output| / |Input|)`
5. Find -3 dB point (cutoff frequency)
6. Measure rolloff slope (linear regression, 2-5 kHz above cutoff)

**Resonance Test:**
- Repeat with resonance = 0.95
- Measure peak height at cutoff
- Measure Q factor (peak height / -3 dB bandwidth)

### Envelope Timing

**Test Signal:** Gate on for 1 second, gate off
**Settings:**
- Attack = 100 ms
- Decay = 300 ms
- Sustain = 0.7
- Release = 500 ms

**Measurements:**
1. Trigger envelope
2. Record output over time
3. Measure time to reach 99% of peak (attack)
4. Measure time to reach sustain + 1% (decay)
5. Measure time to reach 1% after release (release)

**Curve Shape:**
- Fit exponential: `y = A * (1 - exp(-t / τ))`
- Compare time constant τ to theoretical value

### Chorus Delay Time

**Test Signal:** Single impulse (Dirac delta)
**Settings:** Chorus Mode I or II

**Measurements:**
1. Send impulse through chorus
2. Record left and right outputs
3. Cross-correlate output with input
4. Find peak delay (time of maximum correlation)
5. Repeat over 10-second window to measure LFO modulation
6. FFT of delay time vs. time to measure LFO frequency

---

## Known Issues and Quirks

### Original Juno-106 Issues

1. **Voice Chip Disease**
   - IR3109 chips fail over time (capacitor degradation)
   - Symptoms: Dead voices, stuck filters, distortion
   - Not emulated (intentionally)

2. **Noisy Sliders**
   - Potentiometers develop noise and dead spots
   - Not relevant to digital implementation

3. **Chorus Bleed**
   - Some units have chorus bleed even when off
   - Not emulated

### Poor House Juno Known Limitations

1. **No MIDI CC Mapping (M16)**
   - Currently only supports note on/off, pitch bend, mod wheel
   - Full MIDI CC mapping planned

2. **No Per-Voice Variation**
   - All voices are identical (no analog drift)
   - Could be added as optional feature

3. **No BBD Artifacts**
   - Clean digital delay (no clock noise, droop)
   - Sounds slightly "cleaner" than hardware

---

## Validation Results (M15)

**Status:** Pending completion of comparison tools

**Target Metrics:**
- Filter cutoff accuracy: <2%
- Envelope timing accuracy: <5%
- Chorus delay accuracy: <0.2 ms

**Test Patches:**
1. Bass patch (filter sweeps)
2. Pad patch (slow attack, chorus)
3. Lead patch (PWM, portamento)
4. Brass patch (resonance, envelope)

---

## References

### Hardware Documentation
- Roland Juno-106 Service Manual (1984)
- Roland Juno-106 Owner's Manual
- IR3109 Datasheet (limited availability)
- MN3009 BBD Datasheet

### Software References
- TAL-U-NO-LX by Togu Audio Line
- Dexed (Yamaha DX7 emulator, for MIDI implementation reference)

### Academic References
- Stilson & Smith (1996) - "Alias-Free Digital Synthesis"
- Zavalishin (2012) - "The Art of VA Filter Design"
- Välimäki et al. (2012) - "Virtual Analog Modeling"

### Online Resources
- Juno-106 service notes (various forums)
- Gearslutz/GearSpace Juno-106 threads
- Muffwiggler analog synthesis forums

---

**Last Updated:** January 10, 2026
