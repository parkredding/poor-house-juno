# M16 Testing Guide

**Date:** January 10, 2026  
**Milestone:** M16 - Final Refinement  
**Status:** Code Complete - Ready for Testing

---

## Overview

All M16 features have been implemented in code. This guide provides comprehensive testing procedures to verify functionality once the web build is completed.

---

## Prerequisites

### Building the Web Interface

1. **Install Emscripten SDK** (if not already installed):
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

2. **Build the web version**:
   ```bash
   cd poor-house-juno
   ./scripts/build_web.sh
   ```
   
   Or manually:
   ```bash
   mkdir build-web
   cd build-web
   emcmake cmake .. -DPLATFORM=web -DCMAKE_BUILD_TYPE=Release
   emmake make -j$(nproc)
   ```

3. **Serve the web interface**:
   ```bash
   cd web
   python3 -m http.server 8000
   ```

4. **Open in browser**: http://localhost:8000

---

## Test Plan

### Test 1: Voice Allocation Mode - UI Verification

**Objective:** Verify the voice allocation dropdown exists and has correct options

**Steps:**
1. Open http://localhost:8000
2. Click "Start Audio"
3. Scroll to "Performance (M11/M13/M14/M16)" section
4. Locate "Voice Allocation (M16)" dropdown

**Expected Results:**
- ✅ Dropdown is visible and labeled "Voice Allocation (M16)"
- ✅ Four options are available:
  - Oldest (Default)
  - Newest (Last-Note)
  - Low-Note Priority
  - High-Note Priority
- ✅ Default selection is "Oldest (Default)"

---

### Test 2: Voice Allocation Mode - Oldest (Default)

**Objective:** Verify round-robin voice stealing behavior

**Steps:**
1. Select "Oldest (Default)" mode
2. Play 6 notes simultaneously (e.g., C3, D3, E3, F3, G3, A3)
3. Without releasing, play a 7th note (B3)
4. Observe which voice is stolen

**Expected Results:**
- ✅ First note played (C3) is stolen
- ✅ Voice count stays at 6/6
- ✅ Remaining notes continue playing

**Repeat:**
- Play an 8th note → 2nd note (D3) should be stolen
- Play a 9th note → 3rd note (E3) should be stolen

---

### Test 3: Voice Allocation Mode - Newest (Last-Note)

**Objective:** Verify most recent note has priority

**Steps:**
1. Select "Newest (Last-Note)" mode
2. Play 6 notes: C3, D3, E3, F3, G3, A3
3. Play a 7th note (B3)
4. Observe behavior

**Expected Results:**
- ✅ Oldest active note is stolen (C3 first, then D3, etc.)
- ✅ Most recently played note (B3) always has priority
- ✅ Good for legato lead lines

---

### Test 4: Voice Allocation Mode - Low-Note Priority

**Objective:** Verify bass notes are protected

**Steps:**
1. Select "Low-Note Priority" mode
2. Play a low bass note: C2
3. Play 5 more notes above it: C3, D3, E3, F3, G3
4. Play a 7th note: A3
5. Observe which note is stolen

**Expected Results:**
- ✅ Highest note is stolen first (G3)
- ✅ Bass note (C2) remains playing
- ✅ Lower notes are protected over higher notes

**Test Case 2:**
1. Play: C2, C3, D3, E3, F3, G3 (6 voices)
2. Play A1 (lower than C2)
3. Expected: A1 should replace C2 (A1 is lower priority)

---

### Test 5: Voice Allocation Mode - High-Note Priority

**Objective:** Verify treble notes are protected

**Steps:**
1. Select "High-Note Priority" mode
2. Play a high lead note: C5
3. Play 5 more notes below it: C4, D4, E4, F4, G4
4. Play a 7th note: A4
5. Observe which note is stolen

**Expected Results:**
- ✅ Lowest note is stolen first (C4)
- ✅ High lead note (C5) remains playing
- ✅ Higher notes are protected over lower notes

---

### Test 6: M14 Controls - DCO Range

**Objective:** Verify octave shifting works

**Steps:**
1. Play middle C (C4 / MIDI note 60)
2. Listen to pitch
3. Change "DCO Range (M14)" dropdown:
   - 16' → One octave down (C3)
   - 8' → Normal pitch (C4)
   - 4' → One octave up (C5)

**Expected Results:**
- ✅ 16' selection: Pitch drops one octave
- ✅ 8' selection: Normal pitch (default)
- ✅ 4' selection: Pitch rises one octave
- ✅ All playing notes update immediately

---

### Test 7: M14 Controls - VCA Level

**Objective:** Verify output gain control works

**Steps:**
1. Play a sustained note
2. Adjust "VCA Level (M14)" slider from 0% to 100%
3. Observe volume changes

**Expected Results:**
- ✅ 0% → Complete silence
- ✅ 50% → Half volume
- ✅ 100% → Full volume
- ✅ Smooth volume changes with no clicks/pops

---

### Test 8: M14 Controls - Master Tune

**Objective:** Verify fine tuning works

**Steps:**
1. Play A4 (440 Hz reference)
2. Adjust "Master Tune (M14)" slider:
   - Set to +50 cents
   - Set to -50 cents
   - Return to 0

**Expected Results:**
- ✅ +50 cents: Pitch rises (quarter-tone sharp)
- ✅ -50 cents: Pitch drops (quarter-tone flat)
- ✅ 0 cents: Standard tuning (A4 = 440 Hz)
- ✅ Smooth pitch changes

---

### Test 9: M14 Controls - Velocity Sensitivity (Filter)

**Objective:** Verify velocity modulates filter cutoff

**Steps:**
1. Set filter cutoff to 50%
2. Set "Velocity → Filter (M14)" to 50%
3. Play notes with varying velocity:
   - Soft (velocity ~30)
   - Medium (velocity ~64)
   - Hard (velocity ~120)

**Expected Results:**
- ✅ Soft notes: Darker/duller sound (lower cutoff)
- ✅ Medium notes: Normal brightness
- ✅ Hard notes: Brighter sound (higher cutoff)
- ✅ At 0%: No velocity effect on filter
- ✅ At 100%: Maximum velocity effect

---

### Test 10: M14 Controls - Velocity Sensitivity (Amplitude)

**Objective:** Verify velocity modulates volume

**Steps:**
1. Set "Velocity → Amp (M14)" to 100% (default)
2. Play notes with varying velocity:
   - Soft (velocity ~30)
   - Medium (velocity ~64)
   - Hard (velocity ~120)

**Expected Results:**
- ✅ Soft notes: Quieter volume
- ✅ Medium notes: Medium volume
- ✅ Hard notes: Louder volume
- ✅ At 0%: All notes same volume (velocity ignored)
- ✅ At 100%: Full velocity response

---

### Test 11: Preset System - Save with M14/M16 Parameters

**Objective:** Verify new parameters are saved in presets

**Steps:**
1. Configure synth with specific settings:
   - DCO Range: 4' (octave up)
   - VCA Level: 75%
   - Master Tune: +10 cents
   - Velocity → Filter: 30%
   - Velocity → Amp: 80%
   - Voice Allocation: High-Note Priority
2. Save as new preset: "Test M14/M16"
3. Change all settings to different values
4. Load "Test M14/M16" preset

**Expected Results:**
- ✅ All M14 parameters restore correctly
- ✅ Voice allocation mode restores to High-Note Priority
- ✅ UI controls show correct values
- ✅ Audio output matches expected configuration

---

### Test 12: Preset System - Default Presets

**Objective:** Verify enhanced default presets work correctly

**Steps:**
1. Load each default preset and verify behavior:

**"Init" Preset:**
- Voice Allocation: Oldest (Default)
- DCO Range: 8' (normal)
- Velocity → Amp: 100%

**"Bass" Preset:**
- Voice Allocation: Low-Note Priority ✨
- DCO Range: 8'
- VCA Level: 85%
- Velocity → Filter: 20%
- Velocity → Amp: 90%

**"Lead" Preset:**
- Voice Allocation: High-Note Priority ✨
- DCO Range: 8'
- VCA Level: 90%
- Velocity → Filter: 30%
- Velocity → Amp: 100%

**"Pad" Preset:**
- Voice Allocation: Oldest
- VCA Level: 75%
- Velocity → Filter: 15%
- Velocity → Amp: 70%

**"Classic Juno" Preset:**
- Voice Allocation: Oldest
- All M14 parameters at default

**Expected Results:**
- ✅ Bass preset: Low notes protected when playing chords + bass line
- ✅ Lead preset: High notes protected when playing chords + lead
- ✅ All presets load and sound as expected

---

### Test 13: MIDI CC Control (M16 Backend)

**Objective:** Verify MIDI CC messages control new parameters

**Prerequisites:** MIDI controller connected

**Steps:**
1. Send MIDI CC messages via external controller or MIDI monitor

**MIDI CC Mapping (M16):**
- CC #19: DCO Range (0-127 → 16'/8'/4')
- CC #25: VCA Level (0-127 → 0-100%)
- CC #26: Master Tune (0-127 → -50 to +50 cents)
- CC #27: Velocity → Filter (0-127 → 0-100%)
- CC #28: Velocity → Amp (0-127 → 0-100%)
- CC #29: Voice Allocation Mode (0-127 → 0-3)
- CC #64: Sustain Pedal (0-63=off, 64-127=on)

**Expected Results:**
- ✅ Each CC message updates the corresponding parameter
- ✅ UI controls reflect CC changes
- ✅ Audio output changes appropriately
- ✅ Sustain pedal holds notes when pressed

---

### Test 14: Integration Test - Complex Scenario

**Objective:** Test all M16 features together

**Scenario:** Bass + Lead Performance

**Steps:**
1. Load "Bass" preset (Low-Note Priority)
2. Play sustained bass notes in left hand: C2, E2
3. Play melody in right hand: C4, D4, E4, F4, G4, A4, B4
4. Observe voice stealing behavior

**Expected:** 
- ✅ Bass notes (C2, E2) are protected
- ✅ High melody notes are stolen if >6 total voices
- ✅ Bass remains solid throughout

**Switch to Lead:**
1. Load "Lead" preset (High-Note Priority)
2. Play chord in left hand: C3, E3, G3
3. Play high lead line: C5, D5, E5, F5, G5

**Expected:**
- ✅ High lead notes (C5+) are protected
- ✅ Low chord notes are stolen if >6 total voices
- ✅ Lead line remains prominent

---

## Known Limitations

### Build Requirements
- **Emscripten SDK required**: Web build currently requires Emscripten, which is not available on Windows without WSL or Docker
- **Alternative**: Deploy to GitHub Pages or test on Linux/macOS system

### Future Enhancements (Deferred)
- 128-patch bank system (optional, deferred to future milestone)
- Hardware build and Pi testing pending

---

## Success Criteria

M16 is considered **fully complete** when:
- ✅ All 14 test cases pass
- ✅ Voice allocation modes work correctly for different playing styles
- ✅ M14 controls function properly and integrate with presets
- ✅ Preset save/load includes all M14/M16 parameters
- ✅ MIDI CC mapping works for all new parameters
- ✅ No regressions in existing M1-M15 features

---

## Reporting Issues

If any test fails, document:
1. Test number and name
2. Steps to reproduce
3. Expected vs actual behavior
4. Browser/OS information
5. Console errors (if any)

---

**Testing Status:** ⏳ Pending Web Build  
**Code Status:** ✅ Complete  
**Next Step:** Build web interface with Emscripten

