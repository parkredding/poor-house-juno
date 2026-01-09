# Poor House Juno - Juno-106 Faithfulness Evaluation

**Date:** January 9, 2026
**Evaluator:** Claude Code
**Project Version:** M10 Complete (Post-Pi Integration)

---

## Executive Summary

**Poor House Juno** is a work-in-progress Juno-106 emulator targeting Raspberry Pi 4 with a web-based development environment. The project has completed ~10 out of ~11 planned milestones and demonstrates strong core DSP implementation. However, several critical Juno-106 features are currently missing.

**Project Stats:**
- **Lines of Code:** ~1,753 (C++ DSP + platform code)
- **Development Timeline:** January 8-9, 2026 (1-2 days)
- **Commits:** 24 total
- **Current Status:** M10 Complete (Pi Integration), M11 Pending (Final Polish)
- **Faithfulness Score:** ~70-75%

---

## Faithfulness Analysis

### ✅ What's Been Implemented (Core DSP)

#### Oscillator Section (DCO)
- ✅ Sawtooth waveform with polyBLEP anti-aliasing
- ✅ Pulse waveform with variable pulse width (PWM)
- ✅ Sub-oscillator (square wave, -1 octave)
- ✅ White noise generator
- ✅ LFO modulation routing (Pitch, PWM, Both, Off)
- ✅ Per-voice detuning
- ✅ Pitch drift emulation

#### Filter Section
- ✅ IR3109 4-pole ladder filter emulation (24dB/octave lowpass)
- ✅ Resonance with self-oscillation
- ✅ Envelope modulation (bipolar)
- ✅ Key tracking (Off, Half, Full)
- ✅ Subtle saturation for IR3109 character

#### Envelopes
- ✅ Dual ADSR envelopes (Filter and Amplitude)
- ✅ Exponential curves
- ✅ Correct time ranges (Attack: 1.5ms-3s, Decay/Release: 1.5ms-12s)

#### LFO
- ✅ Triangle waveform
- ✅ Rate control (0.1-30 Hz)

#### Chorus
- ✅ BBD (Bucket Brigade Device) emulation
- ✅ Three modes: I, II, and I+II
- ✅ Stereo output with independent modulation

#### System
- ✅ 6-voice polyphony
- ✅ Voice stealing algorithm
- ✅ MIDI input (Web MIDI API and ALSA)
- ✅ Real-time audio processing
- ✅ Web-based test interface
- ✅ Raspberry Pi 4 support

---

### ❌ What's Missing (Critical Juno-106 Features)

#### 1. High-Pass Filter (HPF)
**Status:** Not implemented
**Juno-106 Spec:** 4-position switch (0/1/2/3) controlling cutoff frequency from ~5 Hz to 2.4 kHz
**Impact:** CRITICAL - The HPF is essential for tone shaping and removing low-frequency content

#### 2. VCA Control Mode
**Status:** Not implemented
**Juno-106 Spec:** ENV/GATE switch - allows VCA to be controlled by envelope OR gate signal
**Impact:** MAJOR - Gate mode enables organ-style sounds without envelope shaping

#### 3. VCA Level Control
**Status:** Not implemented (only has master volume concept)
**Juno-106 Spec:** Dedicated VCA Level slider controlling overall output before effects
**Impact:** MODERATE - Affects signal flow and gain staging

#### 4. Envelope Polarity Switch
**Status:** Not implemented
**Juno-106 Spec:** Normal/Inverse switch for filter envelope modulation
**Impact:** MAJOR - Inverse polarity creates different timbral effects (closing filter instead of opening)

#### 5. LFO Delay
**Status:** Not implemented
**Juno-106 Spec:** 0-3 second delay before LFO modulation begins
**Impact:** MAJOR - Essential for performance techniques (delayed vibrato/tremolo)

#### 6. LFO Filter Modulation
**Status:** Partially implemented
**Current:** Filter has `lfoAmount` parameter but it's not exposed in web UI
**Impact:** MODERATE - Important modulation destination is missing from UI

#### 7. Portamento (Glide)
**Status:** Not implemented
**Juno-106 Spec:** Adjustable glide time between notes
**Impact:** MAJOR - Key performance feature for lead sounds

#### 8. Hold Function
**Status:** Not implemented
**Juno-106 Spec:** Sustains notes after key release
**Impact:** MODERATE - Useful performance feature

#### 9. Pitch Bend / Modulation Wheel
**Status:** Not implemented
**Juno-106 Spec:** Pitch bend wheel (±2 semitones typical), modulation wheel for real-time LFO depth control
**Impact:** CRITICAL - Essential for expressive MIDI performance

#### 10. DCO Range Selection
**Status:** Not implemented
**Juno-106 Spec:** 16', 8', 4' footage switches for octave selection
**Impact:** MODERATE - Limits available pitch range

#### 11. Preset Management (Juno-106 Style)
**Status:** Partially implemented
**Current:** Web interface has save/load/delete, but no 128-patch bank structure
**Juno-106 Spec:** 128 preset memory locations with bank/group organization
**Impact:** MINOR - Current system works, just different organization

#### 12. Bender Range / Bender Target
**Status:** Not implemented
**Juno-106 Spec:** Configurable pitch bend range and routing
**Impact:** MODERATE - Affects pitch bend behavior

---

## Differences from Original Juno-106

### Architectural Differences

1. **Digital vs Analog**
   - **Juno-106:** Analog VCF, VCA, and envelopes with digital DCO
   - **Poor House Juno:** Fully digital DSP emulation
   - **Impact:** Sound will be "cleaner" without analog component drift and imperfections (though pitch drift is emulated)

2. **Sample Rate**
   - **Juno-106:** Continuous-time analog processing
   - **Poor House Juno:** 48 kHz digital processing
   - **Impact:** Nyquist limit at 24 kHz vs unlimited analog bandwidth

3. **Filter Implementation**
   - **Juno-106:** IR3109 analog chip with component tolerances
   - **Poor House Juno:** Zero-Delay Feedback (ZDF) digital model
   - **Impact:** Very close approximation but may lack subtle analog non-linearities

4. **No TAL-U-NO-LX Comparison**
   - **Issue:** README claims to "reverse-engineer TAL-U-NO-LX behavior" but no actual comparison tools or reference recordings exist in the codebase
   - **Impact:** Cannot verify accuracy claims

### Design Decisions

1. **Web Platform**
   - **Addition:** Full web-based test environment (not in original Juno-106)
   - **Impact:** Excellent for development/testing

2. **Simplified Chorus**
   - **Juno-106:** Uses MN3009 BBD chips with analog circuitry
   - **Poor House Juno:** Digital BBD emulation with fixed parameters
   - **Impact:** Chorus is functional but may lack subtle analog BBD artifacts (clock noise, droop, etc.)

3. **No Hardware Controls**
   - **Juno-106:** Panel-mounted sliders, buttons, switches
   - **Poor House Juno:** MIDI CC or web interface only (planned)
   - **Impact:** Less immediate than hardware controls

---

## Comprehensive Punchlist

### 🔴 CRITICAL (Must-Have for Juno-106 Faithfulness)

1. **Implement High-Pass Filter (HPF)**
   - Add 4-position switch (0/1/2/3) or continuous control
   - Implement 6dB/octave HPF in signal chain
   - Cutoff range: ~5 Hz to 2.4 kHz
   - Location: `src/dsp/` - new `hpf.h/.cpp` files

2. **Implement Pitch Bend**
   - Add pitch bend wheel MIDI CC handler
   - Configurable bend range (±2 semitones default)
   - Apply to all active voices
   - Location: `src/dsp/synth.cpp`, `parameters.h`

3. **Implement LFO Delay**
   - Add 0-3 second delay parameter
   - Delay only affects modulation depth, not LFO phase
   - Location: `src/dsp/lfo.h/.cpp`

4. **Implement Portamento**
   - Add glide time parameter (0-10 seconds typical)
   - Smooth pitch transitions between notes
   - Mode options: always, legato-only
   - Location: `src/dsp/voice.h/.cpp`

5. **Expose Filter LFO Modulation in UI**
   - Parameter exists in code but not in web interface
   - Add slider to web UI for `FILTER_LFO_AMOUNT`
   - Location: `web/index.html`, `web/js/app.js`

### 🟡 MAJOR (Important for Authentic Sound)

6. **Implement VCA Control Mode (ENV/GATE)**
   - Add VCA mode parameter (ENV or GATE)
   - In GATE mode, bypass amplitude envelope (instant on/off)
   - Location: `src/dsp/voice.cpp`, `parameters.h`

7. **Implement Filter Envelope Polarity Switch**
   - Add Normal/Inverse parameter to `FilterParams`
   - Invert envelope value when in Inverse mode
   - Location: `src/dsp/filter.h/.cpp`, `parameters.h`

8. **Implement Modulation Wheel**
   - Add MIDI CC #1 handler
   - Control LFO depth in real-time
   - Location: `src/dsp/synth.cpp`, `parameters.h`

9. **Implement DCO Range Selection**
   - Add 16'/8'/4' switches (or global transpose)
   - Apply octave shift to all voices
   - Location: `src/dsp/parameters.h`, `synth.cpp`

10. **Implement VCA Level Control**
    - Add dedicated VCA level parameter (separate from master volume)
    - Apply before chorus effect
    - Location: `src/dsp/voice.cpp`, `parameters.h`

### 🟢 MINOR (Nice to Have)

11. **Implement Hold Function**
    - Add Hold toggle (sustain all notes)
    - MIDI CC #64 (Sustain Pedal) support
    - Location: `src/dsp/synth.cpp`

12. **Implement 128-Patch Bank System**
    - Match Juno-106's bank/group structure
    - Add bank select UI
    - Location: `web/js/app.js`, preset management

13. **Add MIDI CC Mapping for All Parameters**
    - Map all synth parameters to MIDI CC messages
    - Create CC map matching Juno-106 (if applicable)
    - Location: `src/dsp/synth.cpp`, `src/platform/*/midi_driver.cpp`

14. **Add Master Tune Control**
    - Parameter ID exists (`MASTER_TUNE`) but not implemented
    - Add ±50 cent tuning control
    - Location: `src/dsp/synth.cpp`

15. **Create TAL-U-NO-LX Comparison Tools**
    - README claims this, but no tools exist
    - Implement `tools/analyze_tal.py`, `tools/measure_filter.py`, etc.
    - Generate reference recordings for A/B testing
    - Location: `tools/` directory

### 🔧 TECHNICAL DEBT / POLISH

16. **Add Velocity Sensitivity Options**
    - Current: velocity affects both filter and amplitude
    - Add per-parameter velocity amount controls
    - Location: `src/dsp/voice.cpp`

17. **Implement CPU Usage Optimization**
    - Target: <50% on Raspberry Pi 4 (README claims this but not verified)
    - Profile and optimize DSP hotspots
    - Consider NEON SIMD optimizations for ARM
    - Location: All DSP files

18. **Add Unit Tests**
    - README mentions tests but none exist
    - Test oscillator waveforms, filter response, envelope curves
    - Location: `tests/` directory (create)

19. **Create Documentation**
    - `docs/architecture.md` (planned, not created)
    - `docs/dsp_design.md` (planned, not created)
    - `docs/juno106_analysis.md` (planned, not created)
    - Filter tuning, chorus analysis, Pi setup guides

20. **Implement Voice Allocation Modes**
    - Current: basic voice stealing (oldest or released voice)
    - Add priority modes: low-note, high-note, last-note
    - Location: `src/dsp/synth.cpp`

---

## Development Cost Estimation (Human-Only)

Based on the project's current state and industry-standard rates, here's an estimate for human-only development:

### Time Breakdown

| Phase | Tasks | Estimated Hours | Hourly Rate | Cost Range |
|-------|-------|-----------------|-------------|------------|
| **Phase 1: Core DSP** | Oscillator, Filter, Envelopes, LFO, Voice, Polyphony | 60-80 hours | $100-200/hr | $6,000-16,000 |
| **Phase 2: Effects** | BBD Chorus implementation | 15-25 hours | $100-200/hr | $1,500-5,000 |
| **Phase 3: Platform Integration** | Web Audio API, Emscripten, AudioWorklet | 25-35 hours | $100-200/hr | $2,500-7,000 |
| **Phase 4: Raspberry Pi** | ALSA audio/MIDI, real-time threads, optimization | 30-45 hours | $100-200/hr | $3,000-9,000 |
| **Phase 5: Web UI** | HTML/CSS/JS interface, controls, keyboard, presets | 25-40 hours | $75-150/hr | $1,875-6,000 |
| **Phase 6: Missing Features** | HPF, portamento, pitch bend, LFO delay, etc. | 40-60 hours | $100-200/hr | $4,000-12,000 |
| **Phase 7: Testing & Refinement** | DSP tuning, debugging, TAL comparison, optimization | 30-50 hours | $100-200/hr | $3,000-10,000 |
| **Phase 8: Documentation** | Architecture docs, user guide, API docs | 15-25 hours | $75-150/hr | $1,125-3,750 |
| **Phase 9: Project Management** | Planning, coordination, reviews (15% overhead) | 35-55 hours | $100-200/hr | $3,500-11,000 |

### Total Estimate: $26,500 - $79,750

**Median Estimate: ~$50,000-55,000** (assuming mid-range rates and hours)

### Cost Factors

**Lower End ($26-35K):**
- Junior developer or contractor in lower-cost region
- Minimal documentation and testing
- Basic feature implementation without extensive tuning
- Solo developer (no PM overhead)

**Higher End ($60-80K):**
- Senior audio DSP engineer in high-cost region (SF Bay Area, NYC)
- Comprehensive testing against reference hardware
- Extensive documentation and code review
- Full project management and QA

**Realistic Mid-Range ($45-55K):**
- Experienced DSP developer with synthesizer background
- Good documentation and reasonable testing
- Most critical features implemented
- Light project management

### Additional Considerations

1. **Hardware Costs:**
   - Juno-106 or TAL-U-NO-LX license for reference: $200-3,000
   - Raspberry Pi 4 + audio interface + MIDI controller: $200-500
   - Test equipment (oscilloscope, audio analyzer): $500-2,000

2. **Software/Tools:**
   - Development tools (mostly free: Emscripten, CMake, GCC)
   - Audio analysis software: $0-500

3. **Ongoing Costs:**
   - Maintenance and bug fixes: $5-10K/year
   - Feature additions: $10-20K/year
   - Community support: $5-15K/year

### Time Estimate

**Human-only development time:**
- **Sprint development:** 6-10 weeks full-time (240-400 hours)
- **Part-time development:** 4-8 months (10-20 hrs/week)
- **Current project:** Completed in 1-2 days (clearly AI-assisted)

---

## Recommendations

### Priority 1 (Next 2-4 weeks)
1. Implement High-Pass Filter
2. Add Pitch Bend support
3. Implement Portamento
4. Add LFO Delay
5. Expose Filter LFO modulation in UI

### Priority 2 (Next 1-2 months)
6. Add VCA control mode (ENV/GATE)
7. Implement Filter Envelope Polarity
8. Add Modulation Wheel support
9. Implement DCO Range selection
10. Create TAL-U-NO-LX comparison tools

### Priority 3 (Future)
11. Complete documentation
12. Add comprehensive unit tests
13. Optimize for Raspberry Pi 4 (verify <50% CPU target)
14. Implement full MIDI CC mapping
15. Add velocity sensitivity options

---

## Conclusion

**Poor House Juno** demonstrates solid foundational DSP work with excellent architecture (shared DSP core, dual-platform support). However, it's currently **~70-75% faithful** to the Juno-106 specification due to missing critical features like HPF, portamento, pitch bend, and LFO delay.

**The project is functional as a basic 6-voice polysynth**, but lacks several features essential for authentic Juno-106 emulation and expressive performance.

**Estimated remaining work:** 80-120 hours to reach 95% faithfulness.

**Human-only development cost estimate:** $26,500-79,750 (median ~$50-55K)

---

## References

- [Juno-106: Technical Specifications – Roland Corporation](https://support.roland.com/hc/en-us/articles/201966419-Juno-106-Technical-Specifications)
- [Roland Juno-106 - Wikipedia](https://en.wikipedia.org/wiki/Roland_Juno-106)
- [Roland Juno-106 | Vintage Synth Explorer](https://www.vintagesynth.com/roland/juno106)
- Poor House Juno codebase analysis (January 2026)
