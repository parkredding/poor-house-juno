# Poor House Juno - Unmet Milestones Punchlist

**Date:** January 9, 2026
**Current Status:** M13 Complete (13/16 milestones done)
**Faithfulness:** ~89%

---

## Summary

**Remaining Work:** 3 milestones (M14-M16)
**Total Estimated Time:** 91-147 hours (~115 hours median)
**Priority:** M14 (HIGH) → M15 (MEDIUM) → M16 (LOW)

---

## 🔴 M14: Range & Voice Control (8-12 hours) - HIGH PRIORITY

### 1. DCO Range Selection (2-3 hours)
**What:** Add 16'/8'/4' octave transpose switches
**Why:** Users cannot easily shift the synth's pitch range
**Files:**
- `src/dsp/parameters.h` - Add octave transpose parameter
- `src/dsp/voice.cpp` - Apply octave shift to frequency
- `web/index.html` + `web/js/app.js` - Add UI control

**Implementation:**
```cpp
// In parameters.h
enum DcoRange {
    DCO_RANGE_16 = -12,  // -1 octave
    DCO_RANGE_8 = 0,     // Normal
    DCO_RANGE_4 = 12     // +1 octave
};
int dcoRange;
```

---

### 2. VCA Level Control (2-3 hours)
**What:** Add dedicated VCA level slider (separate from master volume)
**Why:** Authentic signal flow, better gain staging
**Files:**
- `src/dsp/parameters.h` - Add vcaLevel parameter
- `src/dsp/voice.cpp` or `synth.cpp` - Apply before chorus
- `web/index.html` + `web/js/app.js` - Add UI slider

**Implementation:**
```cpp
// Apply in Synth::processStereo() before chorus
mixedVoices *= vcaLevel_;  // 0.0-1.0, default 0.75
```

---

### 3. Velocity Sensitivity Options (3-4 hours)
**What:** Add per-parameter velocity depth controls
**Why:** More flexible velocity response (currently velocity affects filter/amp equally)
**Files:**
- `src/dsp/parameters.h` - Add velocityToFilter, velocityToAmp
- `src/dsp/voice.cpp` - Apply velocity curves separately
- `web/index.html` + `web/js/app.js` - Add UI controls

**Implementation:**
```cpp
// In Voice::process()
float filterVelocity = 1.0 + (velocity_ - 1.0) * velocityToFilter_;
float ampVelocity = 1.0 + (velocity_ - 1.0) * velocityToAmp_;
```

---

### 4. Master Tune (1-2 hours)
**What:** Add ±50 cent tuning control
**Why:** Tune synth to other instruments
**Files:**
- `src/dsp/synth.cpp` - Apply master tune to all voices
- `web/index.html` + `web/js/app.js` - Add UI control

**Implementation:**
```cpp
// In Synth class
float masterTuneRatio = std::pow(2.0f, masterTune_ / 1200.0f);
// Apply to all voice frequencies
```

---

## 🟡 M15: Polish & Optimization (60-100 hours) - MEDIUM PRIORITY

### 5. Unit Test Suite (15-25 hours)
**What:** Create automated tests for DSP components
**Why:** Verify DSP correctness, prevent regressions
**Files to create:**
- `tests/test_oscillator.cpp` - Waveform quality, polyBLEP
- `tests/test_filter.cpp` - Frequency response, resonance
- `tests/test_envelope.cpp` - ADSR curves, timing
- `tests/test_lfo.cpp` - Triangle wave, delay
- `tests/test_voice.cpp` - Voice stealing, portamento
- `tests/CMakeLists.txt` - Build configuration

**Tools:** Catch2, Google Test, or doctest

---

### 6. TAL-U-NO-LX Comparison Tools (20-30 hours)
**What:** Create tools to verify accuracy vs TAL-U-NO-LX
**Why:** README claims "reverse-engineered from TAL" but no verification exists
**Files to create:**
- `tools/analyze_tal.py` - Parameter behavior analysis
- `tools/measure_filter.py` - Filter frequency response
- `tools/measure_chorus.py` - Chorus characteristics
- `tools/export_preset.py` - Preset conversion
- `tools/generate_reference.py` - Reference recordings

**Requirements:** TAL-U-NO-LX VST or reference recordings

---

### 7. Documentation (15-25 hours)
**What:** Create comprehensive technical documentation
**Why:** Explain architecture, DSP design, usage
**Files to create:**
- `docs/architecture.md` - System design, build process
- `docs/dsp_design.md` - DSP algorithms, filter/oscillator design
- `docs/juno106_analysis.md` - Reverse engineering notes
- `docs/filter_tuning.md` - IR3109 calibration
- `docs/chorus_analysis.md` - BBD chorus design
- `docs/pi_setup.md` - Raspberry Pi setup guide
- `docs/web_interface.md` - WASM architecture
- `docs/midi_cc_map.md` - MIDI CC reference

**Include:** Diagrams, code examples, troubleshooting

---

### 8. CPU Profiling & Optimization (10-20 hours)
**What:** Profile and optimize for Raspberry Pi 4
**Why:** Verify <50% CPU target, ensure real-time performance
**Tasks:**
- Profile on Pi 4 with 6 voices + chorus
- Identify hotspots (filter, polyBLEP, chorus)
- Optimize (SIMD, loop unrolling, ARM NEON)
- Verify performance targets

**Files to modify:**
- `src/dsp/filter.cpp`
- `src/dsp/chorus.cpp`
- `src/dsp/oscillator.cpp`
- `CMakeLists.txt` - Add optimization flags

**Tools:** `perf`, `gprof`

---

## 🟢 M16: Final Refinement (23-35 hours) - LOW PRIORITY

### 9. Full MIDI CC Mapping (8-12 hours)
**What:** Map all synth parameters to MIDI CC messages
**Why:** Enable hardware controller integration
**Files:**
- `src/dsp/synth.cpp` - Add CC handlers
- `src/platform/web/main.cpp` - Route CCs
- `src/platform/pi/midi_driver.cpp` - Route CCs
- `docs/midi_cc_map.md` - Document mappings

**Suggested CC Mapping:**
- CC #1: Mod Wheel (✓ done)
- CC #64: Sustain Pedal (see #10)
- CC #71: Filter Resonance
- CC #73: Filter Env Amount
- CC #74: Filter Cutoff
- CC #75: LFO Rate
- Etc.

---

### 10. Sustain Pedal (Hold Function) (3-5 hours)
**What:** Implement MIDI CC #64 sustain pedal support
**Why:** Essential performance feature
**Files:**
- `src/dsp/synth.h/.cpp` - Add sustain state, buffer note-offs
- `src/platform/web/main.cpp` - Handle CC #64
- `src/platform/pi/midi_driver.cpp` - Handle CC #64
- `web/index.html` - Add Hold toggle for testing

**Implementation:**
```cpp
// In Synth class
bool sustainPedalDown_;
std::vector<int> sustainedNotes_;

void handleSustainPedal(bool down) {
    sustainPedalDown_ = down;
    if (!down) {
        // Release all sustained notes
        for (int note : sustainedNotes_) {
            handleNoteOff(note);
        }
        sustainedNotes_.clear();
    }
}
```

---

### 11. 128-Patch Bank System (8-12 hours)
**What:** Organize presets into 8 banks of 16 patches (Juno-106 style)
**Why:** Authentic preset organization, MIDI Program Change support
**Files:**
- `web/js/presets.js` - Reorganize into bank structure
- `web/index.html` - Add bank select UI
- `web/css/juno.css` - Style bank/patch selector
- `src/platform/web/main.cpp` - Bank select logic (optional)

**Structure:**
- 8 Banks (A-H)
- 16 Patches per bank (1-16)
- MIDI Program Change 0-127

---

### 12. Voice Allocation Priority Modes (4-6 hours)
**What:** Add voice stealing priority modes
**Why:** Better voice allocation for different playing styles
**Files:**
- `src/dsp/parameters.h` - Add priority mode enum
- `src/dsp/synth.cpp` - Implement priority algorithms
- `web/index.html` + `web/js/app.js` - Add UI selector

**Modes:**
- **Round-robin** (current): Steal oldest voice
- **Low-note priority**: Steal highest note first (bass playing)
- **High-note priority**: Steal lowest note first (lead playing)
- **Last-note priority**: Most recent note has priority

---

## Quick Reference: Unmet Features by Impact

### CRITICAL (blocking authentic Juno-106 emulation)
None! All critical features are implemented in M11-M13.

### MAJOR (important for complete experience)
- ✅ All implemented! (HPF, Pitch Bend, Portamento, LFO Delay, VCA Mode, Filter Polarity - done in M11-M13)

### MODERATE (valuable additions)
- ❌ DCO Range Selection (M14)
- ❌ VCA Level Control (M14)
- ❌ Full MIDI CC Mapping (M16)
- ❌ Unit Tests (M15)
- ❌ TAL Comparison Tools (M15)
- ❌ CPU Profiling (M15)

### MINOR (nice to have)
- ❌ Master Tune (M14)
- ❌ Velocity Sensitivity Options (M14)
- ❌ Sustain Pedal (M16)
- ❌ 128-Patch Banks (M16)
- ❌ Voice Priority Modes (M16)
- ❌ Documentation (M15)

---

## Recommended Implementation Order

### Sprint 1: Quick Wins (1 week, 8-12 hours)
Focus on completing M14 to reach 95% faithfulness:
1. DCO Range Selection (2-3h)
2. VCA Level Control (2-3h)
3. Velocity Sensitivity (3-4h)
4. Master Tune (1-2h)

**Result:** Fully featured Juno-106 sound engine

---

### Sprint 2: Quality Assurance (3 weeks, 40-60 hours)
Focus on verification and testing:
1. Unit Test Suite (15-25h)
2. TAL Comparison Tools (20-30h)
3. CPU Profiling (5-10h)

**Result:** Verified accuracy and performance

---

### Sprint 3: Optimization (1 week, 10-20 hours)
Focus on Raspberry Pi performance:
1. Profile on Pi 4
2. Optimize hotspots
3. Implement SIMD/NEON
4. Verify <50% CPU target

**Result:** Production-ready performance

---

### Sprint 4: Documentation (2 weeks, 15-25 hours)
Focus on comprehensive documentation:
1. Architecture docs
2. DSP design docs
3. User guides
4. API documentation

**Result:** Fully documented project

---

### Sprint 5: Final Polish (1 week, 23-35 hours)
Focus on M16 refinements:
1. Full MIDI CC Mapping (8-12h)
2. Sustain Pedal (3-5h)
3. 128-Patch Banks (8-12h)
4. Voice Priority Modes (4-6h)

**Result:** Production-ready Juno-106 emulator

---

## Total Timeline

**Aggressive (full-time):** 6-8 weeks
**Moderate (part-time):** 3-4 months
**Relaxed (hobby pace):** 6-8 months

---

## Files Most Likely to Need Modification

### High-frequency edits:
- `src/dsp/parameters.h` - New parameters for M14/M16
- `src/dsp/synth.cpp` - MIDI CC mapping, sustain pedal, voice allocation
- `src/dsp/voice.cpp` - DCO range, VCA level, velocity curves
- `web/index.html` - UI controls for all new features
- `web/js/app.js` - Wire up new parameters

### New directories to create:
- `tests/` - Unit test suite
- `tools/` - TAL comparison and analysis tools
- `docs/` - Comprehensive documentation

---

*Generated by Claude Code on January 9, 2026*
