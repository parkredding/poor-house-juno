# Poor House Juno - Outstanding Features Punchlist

**Date:** January 10, 2026
**Current Status:** M14 Complete, All Tests Passing ✅
**Milestones Complete:** 14/16 (87.5%)
**Faithfulness:** 89%

---

## 🎉 PHASE 1: COMPLETE ✅

**All Critical Bugs Fixed!**

Recent commits 0159f58 and 83d9bc1 fixed all critical DSP bugs:
- ✅ LFO waveform generation
- ✅ LFO → DCO pitch modulation routing
- ✅ Filter envelope modulation
- ✅ Filter LFO modulation
- ✅ Envelope timing
- ✅ Voice release/deactivation
- ✅ DCO Range (16'/8'/4')
- ✅ Filter key tracking
- ✅ Filter velocity modulation
- ✅ LFO phase start

**Test Results:** ✅ All 25 unit tests passing

---

## 📋 PHASE 2: M15 - Testing, Optimization & Documentation

**Priority: MEDIUM** | **Time Estimate: 60-100 hours**

### 1. Chorus Unit Tests (3-5 hours)
**Status:** ⏳ Not started
**Why:** Chorus is the only DSP component without unit tests
**Impact:** MODERATE - Would increase confidence in chorus implementation

**Tasks:**
- [ ] Create `tests/test_chorus.cpp`
- [ ] Test Mode I (2.5ms delay, 0.5ms depth, 0.65 Hz)
- [ ] Test Mode II (4.0ms delay, 0.8ms depth, 0.50 Hz)
- [ ] Test Mode I+II (both combined)
- [ ] Verify stereo output
- [ ] Test BBD delay modulation

**Files to create:**
- `tests/test_chorus.cpp` (new)

**Implementation notes:**
- Test each mode separately
- Verify delay characteristics
- Check stereo spread
- Test switching between modes

---

### 2. TAL-U-NO-LX Comparison Tools (20-30 hours)
**Status:** ⏳ Not started
**Why:** README claims "reverse-engineered from TAL" but no verification exists
**Impact:** HIGH - Objective accuracy verification

**Tasks:**
- [ ] Create `tools/` directory structure
- [ ] Write Python analysis scripts
- [ ] Generate reference recordings from TAL
- [ ] Compare frequency responses
- [ ] Document differences
- [ ] Create A/B testing workflow

**Files to create:**
- `tools/analyze_tal.py` - Analyze TAL parameter behavior
  - Extract filter curves
  - Measure envelope timing
  - Analyze LFO characteristics

- `tools/measure_filter.py` - Measure filter frequency response
  - Sweep test: 20Hz-20kHz
  - Plot magnitude response
  - Compare resonance behavior
  - Verify key tracking

- `tools/measure_chorus.py` - Analyze chorus characteristics
  - Measure delay times
  - Analyze modulation depth
  - Compare stereo spread

- `tools/generate_reference.py` - Generate reference recordings
  - Sweep tones
  - Filter sweeps
  - LFO modulation tests
  - Preset comparison

- `tools/README.md` - Tool usage documentation

**Requirements:**
- Python 3.8+
- NumPy, SciPy, Matplotlib
- librosa or soundfile for audio I/O
- TAL-U-NO-LX VST plugin (for reference) OR pre-recorded reference files

**Expected outcome:**
- Quantifiable accuracy metrics
- Identified areas for improvement
- Confidence in "reverse-engineered from TAL" claim

---

### 3. CPU Profiling & Optimization (10-20 hours)
**Status:** ⏳ Not started
**Why:** Unknown if <50% CPU target is met on Raspberry Pi 4
**Impact:** HIGH - Critical for Pi deployment

**Tasks:**

#### Phase 3a: Profiling (4-6 hours)
- [ ] Set up profiling on Raspberry Pi 4
- [ ] Profile with 6 voices + chorus + max modulation
- [ ] Identify DSP hotspots
- [ ] Measure current CPU usage
- [ ] Document baseline performance

**Tools:** `perf`, `gprof`, or custom timing code

**Test scenarios:**
- 6 voices playing simultaneously
- All modulation active (LFO, envelopes)
- Chorus enabled (all modes)
- High filter resonance
- Fast LFO rates

#### Phase 3b: Optimization (6-14 hours)
- [ ] Optimize identified hotspots
- [ ] Consider SIMD optimizations
- [ ] Implement ARM NEON intrinsics (if needed)
- [ ] Reduce buffer copies
- [ ] Optimize filter coefficient calculations

**Likely optimization targets:**
1. **Filter::process()** - Called most frequently
   - 4-pole ladder = 4 stages per sample
   - Consider NEON SIMD for parallel processing

2. **Dco::polyBlep()** - Called for every discontinuity
   - Pre-compute coefficients where possible
   - Optimize conditional branches

3. **Chorus::process()** - Stereo processing
   - SIMD for parallel L/R processing
   - Optimize interpolation

4. **Envelope::process()** - Exponential calculations
   - Consider lookup tables for exp()
   - Cache coefficient calculations

**Files to modify:**
- `src/dsp/filter.cpp` - Filter optimization
- `src/dsp/dco.cpp` - polyBLEP optimization
- `src/dsp/chorus.cpp` - Chorus optimization
- `src/dsp/envelope.cpp` - Envelope optimization
- `CMakeLists.txt` - Add optimization flags, NEON support

**Success criteria:**
- <50% CPU usage on Raspberry Pi 4
- 128-sample buffer at 48kHz (2.7ms latency)
- No audio dropouts or glitches

---

### 4. Comprehensive Documentation (15-25 hours)
**Status:** ⏳ Not started (docs/ directory doesn't exist)
**Why:** Project needs technical documentation for users and developers
**Impact:** MODERATE - Useful for adoption and contribution

**Tasks:**
- [ ] Create `docs/` directory
- [ ] Write architecture documentation
- [ ] Document DSP design decisions
- [ ] Create user guides
- [ ] API reference documentation

**Files to create:**

#### `docs/architecture.md` (3-4 hours)
- System overview and component diagram
- Build system (CMake structure)
- Platform abstraction (Web vs Pi)
- WASM integration (Emscripten)
- ALSA audio/MIDI setup
- Real-time threading architecture

#### `docs/dsp_design.md` (4-6 hours)
- DCO implementation (polyBLEP anti-aliasing)
- IR3109 filter design (ZDF topology)
- Envelope curve calculations
- LFO implementation
- Chorus BBD emulation
- Signal flow diagram
- Sample rate considerations

#### `docs/juno106_analysis.md` (2-3 hours)
- Juno-106 specifications
- Reverse engineering process
- Component behavior analysis
- TAL-U-NO-LX comparison findings
- Differences from hardware

#### `docs/filter_tuning.md` (2-3 hours)
- IR3109 filter calibration
- Resonance tuning process
- Cutoff frequency mapping
- Key tracking implementation
- Saturation characteristics

#### `docs/chorus_analysis.md` (2-3 hours)
- BBD chorus design
- MN3009 chip characteristics
- Delay time selection
- Modulation depth/rate tuning
- Stereo imaging

#### `docs/pi_setup.md` (2-3 hours)
- Raspberry Pi 4 setup guide
- ALSA audio configuration
- MIDI device setup
- Real-time priority configuration
- Performance tuning tips
- Troubleshooting guide

#### `docs/web_interface.md` (1-2 hours)
- Web UI architecture
- AudioWorklet integration
- WASM memory management
- Preset system (localStorage)
- Virtual keyboard
- MIDI Web API

#### `docs/midi_cc_map.md` (1-2 hours)
- Current MIDI CC implementation
- Planned CC mappings (M16)
- MIDI channel configuration
- Pitch bend range
- Mod wheel behavior

**Total:** ~15-25 hours

---

## 📋 PHASE 3: M16 - Final Refinement

**Priority: LOW** | **Time Estimate: 23-35 hours**

### 5. Full MIDI CC Mapping (8-12 hours)
**Status:** ⏳ Not started
**Current:** Only CC #1 (Mod Wheel) and Pitch Bend
**Impact:** MODERATE - Enables hardware controller integration

**Suggested MIDI CC Mapping:**

| CC # | Parameter | Range | Notes |
|------|-----------|-------|-------|
| 1 | Mod Wheel (LFO Depth) | 0-127 | ✅ Already implemented |
| 64 | Sustain Pedal | 0/127 | See item #6 |
| 71 | Filter Resonance | 0-127 | Maps to 0.0-1.0 |
| 73 | Filter Env Amount | 0-127 | Maps to -1.0 to +1.0 |
| 74 | Filter Cutoff | 0-127 | Maps to 0.0-1.0 (log) |
| 75 | LFO Rate | 0-127 | Maps to 0.1-30 Hz (log) |
| 76 | LFO Delay | 0-127 | Maps to 0.0-3.0 seconds |
| 77 | DCO Pulse Width | 0-127 | Maps to 0.05-0.95 |
| 78 | DCO PWM Depth | 0-127 | Maps to 0.0-1.0 |
| 79 | Filter Env Attack | 0-127 | Maps to 0.001-3.0s (log) |
| 80 | Filter Env Decay | 0-127 | Maps to 0.002-12.0s (log) |
| 81 | Filter Env Sustain | 0-127 | Maps to 0.0-1.0 |
| 82 | Filter Env Release | 0-127 | Maps to 0.002-12.0s (log) |
| 83 | Amp Env Attack | 0-127 | Maps to 0.001-3.0s (log) |
| 84 | Amp Env Decay | 0-127 | Maps to 0.002-12.0s (log) |
| 85 | Amp Env Sustain | 0-127 | Maps to 0.0-1.0 |
| 86 | Amp Env Release | 0-127 | Maps to 0.002-12.0s (log) |
| 91 | Chorus Mode | 0-127 | 0-31=Off, 32-63=I, 64-95=II, 96-127=I+II |
| 102 | Portamento Time | 0-127 | Maps to 0.0-10.0s |
| 103 | Pitch Bend Range | 0-127 | Maps to 2-12 semitones |

**Tasks:**
- [ ] Add `handleControlChange(int cc, int value)` to Synth class
- [ ] Map all CCs to parameter updates
- [ ] Update web MIDI handler
- [ ] Update Pi MIDI driver
- [ ] Document mappings in `docs/midi_cc_map.md`
- [ ] Add MIDI learn functionality (optional)

**Files to modify:**
- `src/dsp/synth.h` - Add CC handler declaration
- `src/dsp/synth.cpp` - Implement CC → parameter mapping
- `src/platform/web/main.cpp` - Route web MIDI CCs
- `src/platform/pi/midi_driver.cpp` - Route ALSA MIDI CCs
- `docs/midi_cc_map.md` - Document CC assignments

**Implementation notes:**
- Use exponential scaling for time-based parameters
- Use logarithmic scaling for frequency-based parameters
- Clamp all values to valid ranges
- Consider MIDI learn mode for custom mappings

---

### 6. Sustain Pedal (MIDI CC #64) (3-5 hours)
**Status:** ⏳ Not started
**Why:** Essential performance feature for piano-style playing
**Impact:** MINOR - Nice to have, not critical

**Tasks:**
- [ ] Add sustain pedal state to Synth class
- [ ] Buffer note-off events when pedal is down
- [ ] Release all sustained notes when pedal released
- [ ] Handle edge cases (sustain during note-on, multiple pedal presses)
- [ ] Add UI toggle for testing (web interface)

**Implementation approach:**

```cpp
// In synth.h
bool sustainPedalDown_;
std::vector<int> sustainedNotes_;

// In synth.cpp
void Synth::handleControlChange(int cc, int value) {
    if (cc == 64) {  // Sustain pedal
        bool pedalDown = (value >= 64);
        handleSustainPedal(pedalDown);
    }
    // ... other CCs
}

void Synth::handleSustainPedal(bool down) {
    sustainPedalDown_ = down;

    if (!down) {
        // Pedal released: release all sustained notes
        for (int note : sustainedNotes_) {
            handleNoteOff(note);
        }
        sustainedNotes_.clear();
    }
}

void Synth::handleNoteOff(int midiNote) {
    if (sustainPedalDown_) {
        // Buffer note-off for later
        sustainedNotes_.push_back(midiNote);
    } else {
        // Release immediately
        // ... existing note-off logic
    }
}
```

**Files to modify:**
- `src/dsp/synth.h` - Add sustain state members
- `src/dsp/synth.cpp` - Implement sustain logic
- `src/platform/web/main.cpp` - Handle CC #64
- `src/platform/pi/midi_driver.cpp` - Handle CC #64
- `web/index.html` - Add Hold toggle button

---

### 7. 128-Patch Bank System (8-12 hours)
**Status:** ⏳ Not started
**Current:** Web localStorage with dynamic preset list (functional)
**Why:** Authentic Juno-106 organization, MIDI Program Change support
**Impact:** MINOR - Current system works, just different organization

**Tasks:**
- [ ] Reorganize preset data structure into banks
- [ ] Create bank select UI (8 banks × 16 patches)
- [ ] Implement MIDI Program Change (0-127)
- [ ] Migrate existing presets to bank structure
- [ ] Add bank copy/swap functions
- [ ] Style bank/patch selector

**Implementation:**

#### Bank Structure
```javascript
// Current: flat preset list
presets = [
    { name: "Preset 1", params: {...} },
    { name: "Preset 2", params: {...} },
    // ...
];

// New: organized banks
banks = {
    "A": [
        { name: "Brass 1", params: {...} },  // A-01
        { name: "Brass 2", params: {...} },  // A-02
        // ... 16 patches per bank
    ],
    "B": [ /* 16 patches */ ],
    "C": [ /* 16 patches */ ],
    "D": [ /* 16 patches */ ],
    "E": [ /* 16 patches */ ],
    "F": [ /* 16 patches */ ],
    "G": [ /* 16 patches */ ],
    "H": [ /* 16 patches */ ]
};
```

#### UI Layout
```
[Bank Select: A ▼] [Patch: 01 ▼]
[Previous] [Next] [Copy] [Paste] [Init]
```

#### MIDI Program Change
```javascript
// MIDI Program Change 0-127 → Bank + Patch
function handleProgramChange(program) {
    const bank = Math.floor(program / 16);  // 0-7 → A-H
    const patch = program % 16;             // 0-15 → 1-16
    loadBankPatch(bank, patch);
}
```

**Files to modify:**
- `web/js/presets.js` - Reorganize data structure and logic
- `web/index.html` - Bank/patch selector UI
- `web/css/juno.css` - Style bank/patch controls
- `src/platform/web/main.cpp` - Program Change handler (optional)

**Migration notes:**
- Preserve existing presets during migration
- Provide import/export for individual banks
- Consider preset initialization (factory presets)

---

### 8. Voice Allocation Priority Modes (4-6 hours)
**Status:** ⏳ Not started
**Current:** Basic oldest-voice stealing
**Why:** Different playing styles benefit from different allocation strategies
**Impact:** MINOR - Current algorithm is reasonable for most use cases

**Voice Priority Modes:**

1. **Round-Robin** (current)
   - Steals oldest active voice
   - Good for general playing

2. **Low-Note Priority**
   - Steals highest note first
   - Preserves bass notes
   - Good for bass playing with chord stabs

3. **High-Note Priority**
   - Steals lowest note first
   - Preserves treble notes
   - Good for lead playing with chord backing

4. **Last-Note Priority**
   - Most recently triggered note always plays
   - Good for legato lead lines

**Tasks:**
- [ ] Add priority mode enum to PerformanceParams
- [ ] Implement priority algorithms in voice stealing
- [ ] Add UI selector (dropdown)
- [ ] Test with different playing styles
- [ ] Document behavior differences

**Implementation:**

```cpp
// In parameters.h
enum VoiceAllocationMode {
    VOICE_ALLOC_ROUND_ROBIN = 0,
    VOICE_ALLOC_LOW_NOTE = 1,
    VOICE_ALLOC_HIGH_NOTE = 2,
    VOICE_ALLOC_LAST_NOTE = 3
};
int voiceAllocationMode;

// In synth.cpp
Voice* Synth::findVoiceToSteal() {
    switch (voiceAllocationMode_) {
        case VOICE_ALLOC_LOW_NOTE:
            return findHighestNote();  // Steal highest
        case VOICE_ALLOC_HIGH_NOTE:
            return findLowestNote();   // Steal lowest
        case VOICE_ALLOC_LAST_NOTE:
            return findOldestNote();   // Steal oldest (but recently played notes protected)
        default:
            return findOldestVoice();  // Round-robin (current)
    }
}
```

**Files to modify:**
- `src/dsp/parameters.h` - Add voice allocation mode enum
- `src/dsp/synth.cpp` - Implement priority algorithms
- `web/index.html` - Add mode selector dropdown
- `web/js/app.js` - Wire up parameter

---

## 📊 Summary Table

| Item | Milestone | Priority | Hours | Status |
|------|-----------|----------|-------|--------|
| **PHASE 1: Critical Bugs** | M14 | 🔴 CRITICAL | 0 | ✅ **COMPLETE** |
| 1. Chorus Unit Tests | M15 | 🟡 MEDIUM | 3-5 | ⏳ Pending |
| 2. TAL Comparison Tools | M15 | 🟡 MEDIUM | 20-30 | ⏳ Pending |
| 3. CPU Profiling & Optimization | M15 | 🟡 MEDIUM | 10-20 | ⏳ Pending |
| 4. Documentation | M15 | 🟡 MEDIUM | 15-25 | ⏳ Pending |
| 5. Full MIDI CC Mapping | M16 | 🟢 LOW | 8-12 | ⏳ Pending |
| 6. Sustain Pedal | M16 | 🟢 LOW | 3-5 | ⏳ Pending |
| 7. 128-Patch Bank System | M16 | 🟢 LOW | 8-12 | ⏳ Pending |
| 8. Voice Priority Modes | M16 | 🟢 LOW | 4-6 | ⏳ Pending |
| **TOTAL REMAINING** | | | **71-115 hours** | |

---

## 🎯 Recommended Implementation Order

### Sprint 1: Verification (20-30 hours)
**Goal:** Verify accuracy and add missing tests
1. Chorus unit tests (3-5h)
2. TAL comparison tools (20-30h)

### Sprint 2: Optimization (10-20 hours)
**Goal:** Meet performance targets
3. CPU profiling on Pi 4 (4-6h)
4. DSP optimization (6-14h)

### Sprint 3: Documentation (15-25 hours)
**Goal:** Complete project documentation
5. Write all docs/ files (15-25h)

### Sprint 4: MIDI Enhancement (11-17 hours)
**Goal:** Complete MIDI support
6. Full MIDI CC mapping (8-12h)
7. Sustain pedal (3-5h)

### Sprint 5: Final Polish (12-18 hours)
**Goal:** Production-ready refinements
8. 128-patch bank system (8-12h)
9. Voice priority modes (4-6h)

---

## 🏁 Completion Milestones

### After Sprint 1-2 (M15 Testing & Optimization)
- Faithfulness: **92%**
- All DSP verified and optimized
- Performance targets met
- Comprehensive test coverage

### After Sprint 3 (M15 Documentation)
- Faithfulness: **92%**
- Fully documented project
- Easy onboarding for contributors
- Clear user guides

### After Sprint 4-5 (M16 Final Refinement)
- Faithfulness: **95%**
- Complete MIDI support
- Production-ready feature set
- Ready for public release

---

## ⏱️ Timeline Estimates

| Pace | M15 (Sprints 1-3) | M16 (Sprints 4-5) | Total |
|------|------------------|------------------|-------|
| **Aggressive (full-time)** | 1-2 weeks | 1 week | 2-3 weeks |
| **Moderate (part-time)** | 1-2 months | 2-3 weeks | 2-3 months |
| **Relaxed (hobby pace)** | 2-3 months | 1-2 months | 4-5 months |

---

## ✅ Success Criteria

### M15 Complete When:
- ✅ All DSP components have unit tests (including chorus)
- ✅ TAL comparison tools created and analysis complete
- ✅ CPU usage <50% on Raspberry Pi 4
- ✅ Comprehensive documentation in docs/ directory
- ✅ Performance targets met and verified

### M16 Complete When:
- ✅ Full MIDI CC mapping implemented
- ✅ Sustain pedal working
- ✅ 128-patch bank system organized
- ✅ Voice allocation modes implemented
- ✅ Production-ready feature set

### Project Complete (95% Faithfulness) When:
- ✅ All M15 criteria met
- ✅ All M16 criteria met
- ✅ Real-world testing successful
- ✅ No critical bugs or performance issues
- ✅ Documentation complete and accurate

---

**Last Updated:** January 10, 2026
**Next Review:** After completing Sprint 1 (Verification)
