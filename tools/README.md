# Poor House Juno - Analysis and Comparison Tools

This directory contains Python tools for analyzing, measuring, and comparing the Poor House Juno synthesizer against TAL-U-NO-LX and the original Juno-106 hardware.

## Purpose

These tools help validate the "reverse-engineered from TAL-U-NO-LX" claim by providing:
- Objective accuracy measurements
- Frequency response comparisons
- Timing and modulation analysis
- Reference recordings for A/B testing

## Requirements

```bash
pip install numpy scipy matplotlib soundfile librosa
```

**Optional (for VST analysis):**
- TAL-U-NO-LX VST plugin
- RenderMan or similar VST host for automated rendering
- REAPER with ReaScript (alternative)

## Tools Overview

### 1. `generate_reference.py` - Reference Recording Generator

Generates test audio files from Poor House Juno (and optionally TAL-U-NO-LX) for comparison.

**Purpose:**
- Create sweep tones, filter sweeps, LFO modulation tests
- Generate identical test signals from both synths
- Export preset comparison recordings

**Usage:**
```bash
# Generate test signals from Poor House Juno web build
python generate_reference.py --synth phj --output references/phj/

# Generate from TAL-U-NO-LX (requires VST setup)
python generate_reference.py --synth tal --vst-path /path/to/TAL-U-NO-LX.vst --output references/tal/

# Generate specific test
python generate_reference.py --synth phj --test filter_sweep --output filter_test.wav
```

**Tests generated:**
- Frequency sweep (20Hz - 20kHz)
- Filter cutoff sweep (0.0 - 1.0, fixed resonance)
- Filter resonance sweep (0.0 - 1.0, fixed cutoff)
- LFO rate sweep (0.1 - 30 Hz)
- Envelope timing tests (attack/decay/release curves)
- Chorus mode comparisons (I, II, I+II)
- Preset bank comparison

### 2. `measure_filter.py` - Filter Frequency Response Analyzer

Measures and compares filter frequency response curves.

**Purpose:**
- Plot magnitude response vs frequency
- Measure resonance peak height and Q factor
- Verify cutoff frequency accuracy
- Compare key tracking behavior

**Usage:**
```bash
# Analyze Poor House Juno filter
python measure_filter.py --input references/phj/filter_sweep.wav --output plots/phj_filter.png

# Compare PHJ vs TAL
python measure_filter.py --phj references/phj/filter_sweep.wav --tal references/tal/filter_sweep.wav --output plots/filter_comparison.png

# Measure resonance behavior
python measure_filter.py --input references/phj/resonance_sweep.wav --mode resonance --output plots/resonance.png
```

**Output:**
- Frequency response plots (magnitude vs frequency)
- Resonance peak measurements
- Cutoff frequency accuracy (expected vs measured)
- Key tracking verification
- Difference plots (PHJ - TAL)

### 3. `measure_chorus.py` - Chorus Characteristics Analyzer

Analyzes BBD chorus delay times, modulation depth, and stereo imaging.

**Purpose:**
- Measure actual delay times for each mode
- Analyze modulation depth and rate
- Compare stereo spread characteristics
- Verify BBD emulation accuracy

**Usage:**
```bash
# Analyze chorus Mode I
python measure_chorus.py --input references/phj/chorus_mode1.wav --mode 1 --output plots/chorus_mode1.png

# Compare all modes
python measure_chorus.py --input references/phj/ --compare-modes --output plots/chorus_comparison.png

# PHJ vs TAL comparison
python measure_chorus.py --phj references/phj/chorus_mode1.wav --tal references/tal/chorus_mode1.wav --output plots/chorus_tal_comparison.png
```

**Output:**
- Delay time measurements (expected: Mode I=2.5ms, II=4.0ms)
- Modulation depth measurements (expected: Mode I=0.5ms, II=0.8ms)
- Modulation rate measurements (expected: Mode I=0.65Hz, II=0.50Hz)
- Stereo correlation plots
- Spectrogram comparisons

### 4. `analyze_tal.py` - TAL Parameter Behavior Analyzer

Analyzes TAL-U-NO-LX parameter behavior and extracts DSP characteristics.

**Purpose:**
- Reverse-engineer parameter curves (exponential, logarithmic, linear)
- Extract filter cutoff frequency mapping (0.0-1.0 → Hz)
- Measure envelope timing curves (ADSR stages)
- Analyze LFO characteristics

**Usage:**
```bash
# Analyze filter cutoff mapping
python analyze_tal.py --parameter filter_cutoff --output data/filter_cutoff_curve.csv

# Analyze envelope timing
python analyze_tal.py --parameter envelope_attack --output data/envelope_attack_curve.csv

# Full parameter analysis
python analyze_tal.py --all --output data/
```

**Output:**
- Parameter curve plots (parameter value vs measured result)
- CSV data files for curve fitting
- Curve fit equations (exponential constants, log bases, etc.)
- Comparison to Poor House Juno implementation

**Note:** This tool requires either:
1. Pre-recorded reference files from TAL-U-NO-LX at various parameter settings
2. VST automation capability (RenderMan, REAPER ReaScript, etc.)

## Typical Workflow

### 1. Generate Reference Recordings

```bash
# Build Poor House Juno web version first
cd /home/user/poor-house-juno
make web

# Generate PHJ test signals (TODO: requires web automation or manual export)
# For now, manually export from web interface at http://localhost:8000
# Future: automate via headless browser (Puppeteer/Playwright)

# If you have TAL-U-NO-LX, generate TAL references
# (Manual process: load TAL in DAW, render test signals)
```

### 2. Analyze Filter Response

```bash
# Measure PHJ filter
python measure_filter.py --input references/phj/filter_sweep.wav --output plots/phj_filter.png

# Compare to TAL (if available)
python measure_filter.py --phj references/phj/filter_sweep.wav --tal references/tal/filter_sweep.wav --output plots/filter_comparison.png
```

### 3. Analyze Chorus

```bash
# Measure chorus characteristics
python measure_chorus.py --input references/phj/ --compare-modes --output plots/chorus_analysis.png
```

### 4. Parameter Analysis (Advanced)

```bash
# Analyze TAL parameter curves (requires TAL reference data)
python analyze_tal.py --all --output data/

# Compare to PHJ implementation
# (Generates recommendations for curve adjustments)
```

## Current Limitations

**Web Build Export:**
Currently, Poor House Juno web build doesn't have automated audio export. Options:
1. **Manual export:** Use browser DevTools to record Web Audio output
2. **Future enhancement:** Add "Export Test Signals" button to web interface
3. **Headless automation:** Use Puppeteer/Playwright to automate web interface

**TAL-U-NO-LX Analysis:**
Requires either:
1. Pre-recorded reference files (included in repo, if available)
2. Manual VST rendering in DAW
3. VST automation tools (RenderMan, REAPER ReaScript)

## Expected Accuracy Metrics

Based on the punchlist, Poor House Juno targets ~89-95% faithfulness to TAL-U-NO-LX:

### Critical Accuracy (>95%)
- Filter cutoff frequency mapping
- Filter resonance behavior
- Envelope timing
- Polyphony and voice stealing

### High Accuracy (85-95%)
- Oscillator waveform quality (polyBLEP)
- LFO modulation depth/rate
- Chorus delay times
- Overall sound character

### Moderate Accuracy (70-85%)
- Chorus stereo imaging (subjective)
- Filter saturation/nonlinearity
- Envelope curve shapes (exponential approximations)

### Known Differences
- Platform-specific: Web Audio API quantization vs native
- CPU-dependent: Chorus interpolation quality
- Intentional: Simplified parameter ranges

## Future Enhancements

- [ ] Automated PHJ web export via Puppeteer
- [ ] VST plugin wrapper for automated TAL rendering
- [ ] Spectral difference analyzer (FFT-based comparison)
- [ ] MIDI file-based preset comparison
- [ ] Perceptual difference metrics (loudness, brightness)
- [ ] Automated regression testing (compare PHJ versions)

## Contributing

When adding new tools:
1. Follow the naming convention: `<verb>_<noun>.py`
2. Include `--help` documentation
3. Output both plots (PNG) and data (CSV)
4. Add usage examples to this README

## References

- TAL-U-NO-LX: https://tal-software.com/products/tal-u-no-lx
- Juno-106 Service Manual: [Roland Corporation]
- IR3109 Filter Analysis: [Various sources]
- BBD Chorus Design: [Boss CE-1, Roland Juno-106 schematics]

---

**Last Updated:** 2026-01-10
**Maintainer:** Poor House Juno Project
