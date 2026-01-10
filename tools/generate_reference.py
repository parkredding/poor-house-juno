#!/usr/bin/env python3
"""
Poor House Juno - Reference Recording Generator

Generates test audio files for analyzing and comparing synthesizer behavior.
These reference files can be compared against actual Poor House Juno or TAL-U-NO-LX output.

Usage:
    python generate_reference.py --output references/ --all
    python generate_reference.py --test filter_sweep --output filter_sweep.wav
"""

import argparse
import numpy as np
import soundfile as sf
from pathlib import Path
from typing import Tuple, Optional
import sys


class ReferenceGenerator:
    """Generates reference audio signals for testing and analysis."""

    def __init__(self, sample_rate: int = 48000):
        self.sample_rate = sample_rate

    def generate_sine_sweep(self,
                           duration: float = 5.0,
                           start_freq: float = 20.0,
                           end_freq: float = 20000.0,
                           amplitude: float = 0.5) -> np.ndarray:
        """
        Generate a logarithmic sine sweep from start_freq to end_freq.

        Args:
            duration: Duration in seconds
            start_freq: Starting frequency in Hz
            end_freq: Ending frequency in Hz
            amplitude: Peak amplitude (0.0-1.0)

        Returns:
            Mono audio signal
        """
        num_samples = int(duration * self.sample_rate)
        t = np.linspace(0, duration, num_samples)

        # Logarithmic sweep
        k = (end_freq / start_freq) ** (1.0 / duration)
        phase = 2.0 * np.pi * start_freq * (k ** t - 1.0) / np.log(k)

        signal = amplitude * np.sin(phase)
        return signal.astype(np.float32)

    def generate_saw_wave(self,
                         frequency: float,
                         duration: float = 2.0,
                         amplitude: float = 0.5) -> np.ndarray:
        """
        Generate a naive sawtooth wave (will have aliasing).
        This is useful for filter testing.

        Args:
            frequency: Frequency in Hz
            duration: Duration in seconds
            amplitude: Peak amplitude

        Returns:
            Mono audio signal
        """
        num_samples = int(duration * self.sample_rate)
        t = np.linspace(0, duration, num_samples)

        # Sawtooth: ramp from -1 to 1
        phase = (frequency * t) % 1.0
        signal = 2.0 * phase - 1.0

        return (amplitude * signal).astype(np.float32)

    def generate_pulse_wave(self,
                           frequency: float,
                           pulse_width: float = 0.5,
                           duration: float = 2.0,
                           amplitude: float = 0.5) -> np.ndarray:
        """
        Generate a pulse wave with specified pulse width.

        Args:
            frequency: Frequency in Hz
            pulse_width: Duty cycle (0.0-1.0)
            duration: Duration in seconds
            amplitude: Peak amplitude

        Returns:
            Mono audio signal
        """
        num_samples = int(duration * self.sample_rate)
        t = np.linspace(0, duration, num_samples)

        phase = (frequency * t) % 1.0
        signal = np.where(phase < pulse_width, 1.0, -1.0)

        return (amplitude * signal).astype(np.float32)

    def generate_filter_sweep_test(self, output_path: Path):
        """
        Generate a test file for analyzing filter cutoff sweep.

        This generates a sawtooth wave while documenting the expected
        filter cutoff sweep. The actual filtering must be done by the synth.

        Output includes:
        - test.wav: Unfiltered sawtooth at A4 (440 Hz)
        - test_metadata.txt: Instructions for manual testing
        """
        output_path.mkdir(parents=True, exist_ok=True)

        # Generate 10-second sawtooth at A4
        saw = self.generate_saw_wave(440.0, duration=10.0, amplitude=0.3)

        # Save test signal
        test_file = output_path / "filter_sweep_input.wav"
        sf.write(test_file, saw, self.sample_rate)

        # Save metadata
        metadata_file = output_path / "filter_sweep_test.txt"
        with open(metadata_file, 'w') as f:
            f.write("Filter Cutoff Sweep Test\n")
            f.write("=" * 50 + "\n\n")
            f.write("Input file: filter_sweep_input.wav\n")
            f.write("  - Sawtooth wave at 440 Hz (A4)\n")
            f.write("  - Duration: 10 seconds\n")
            f.write("  - Amplitude: 0.3\n\n")
            f.write("Manual test procedure:\n")
            f.write("1. Load Poor House Juno web interface\n")
            f.write("2. Set filter resonance to 0.5 (50%)\n")
            f.write("3. Set filter envelope amount to 0.0\n")
            f.write("4. Set LFO rate to 0.0 (no modulation)\n")
            f.write("5. Play A4 (440 Hz) via MIDI or virtual keyboard\n")
            f.write("6. Slowly sweep filter cutoff from 0.0 to 1.0 over 10 seconds\n")
            f.write("7. Record output using browser audio recorder\n")
            f.write("8. Save as 'filter_sweep_output_phj.wav'\n\n")
            f.write("Analysis:\n")
            f.write("  Use measure_filter.py to analyze the output and determine\n")
            f.write("  actual cutoff frequency curve (parameter value → Hz)\n")

        print(f"✓ Generated filter sweep test files in {output_path}/")
        print(f"  - {test_file.name}: Sawtooth input signal")
        print(f"  - {metadata_file.name}: Test instructions")

    def generate_resonance_sweep_test(self, output_path: Path):
        """Generate test file for analyzing filter resonance sweep."""
        output_path.mkdir(parents=True, exist_ok=True)

        # Generate sawtooth at A4
        saw = self.generate_saw_wave(440.0, duration=10.0, amplitude=0.3)

        test_file = output_path / "resonance_sweep_input.wav"
        sf.write(test_file, saw, self.sample_rate)

        metadata_file = output_path / "resonance_sweep_test.txt"
        with open(metadata_file, 'w') as f:
            f.write("Filter Resonance Sweep Test\n")
            f.write("=" * 50 + "\n\n")
            f.write("Input file: resonance_sweep_input.wav\n")
            f.write("  - Sawtooth wave at 440 Hz (A4)\n")
            f.write("  - Duration: 10 seconds\n\n")
            f.write("Manual test procedure:\n")
            f.write("1. Set filter cutoff to 0.5 (around 1-2 kHz)\n")
            f.write("2. Set filter envelope amount to 0.0\n")
            f.write("3. Play A4 (440 Hz)\n")
            f.write("4. Slowly sweep resonance from 0.0 to 1.0 over 10 seconds\n")
            f.write("5. Record output\n")
            f.write("6. Save as 'resonance_sweep_output_phj.wav'\n\n")
            f.write("Analysis:\n")
            f.write("  Use measure_filter.py to measure resonance peak height\n")
            f.write("  and Q factor as a function of resonance parameter.\n")

        print(f"✓ Generated resonance sweep test files in {output_path}/")

    def generate_chorus_test(self, output_path: Path):
        """Generate test files for analyzing chorus characteristics."""
        output_path.mkdir(parents=True, exist_ok=True)

        # Generate sustained chord for chorus analysis
        # Root at 220 Hz (A3), fifth at 330 Hz (E4), octave at 440 Hz (A4)
        duration = 8.0
        chord = (
            self.generate_saw_wave(220.0, duration, 0.2) +
            self.generate_saw_wave(330.0, duration, 0.2) +
            self.generate_saw_wave(440.0, duration, 0.2)
        )

        test_file = output_path / "chorus_test_input.wav"
        sf.write(test_file, chord, self.sample_rate)

        metadata_file = output_path / "chorus_test.txt"
        with open(metadata_file, 'w') as f:
            f.write("Chorus Characteristics Test\n")
            f.write("=" * 50 + "\n\n")
            f.write("Input file: chorus_test_input.wav\n")
            f.write("  - Chord: A3 (220 Hz) + E4 (330 Hz) + A4 (440 Hz)\n")
            f.write("  - Duration: 8 seconds\n\n")
            f.write("Manual test procedure:\n")
            f.write("1. Set filter cutoff to 1.0 (fully open)\n")
            f.write("2. Set resonance to 0.0\n")
            f.write("3. Play the chord (A3+E4+A4 simultaneously)\n")
            f.write("4. Test each chorus mode:\n")
            f.write("   a) Chorus OFF - record as 'chorus_off.wav'\n")
            f.write("   b) Chorus Mode I - record as 'chorus_mode1.wav'\n")
            f.write("   c) Chorus Mode II - record as 'chorus_mode2.wav'\n")
            f.write("   d) Chorus Mode I+II - record as 'chorus_both.wav'\n\n")
            f.write("Expected characteristics:\n")
            f.write("  Mode I: 2.5ms delay, 0.5ms depth, 0.65 Hz rate\n")
            f.write("  Mode II: 4.0ms delay, 0.8ms depth, 0.50 Hz rate\n\n")
            f.write("Analysis:\n")
            f.write("  Use measure_chorus.py to analyze delay times,\n")
            f.write("  modulation depth/rate, and stereo imaging.\n")

        print(f"✓ Generated chorus test files in {output_path}/")

    def generate_envelope_test(self, output_path: Path):
        """Generate test files for analyzing envelope timing."""
        output_path.mkdir(parents=True, exist_ok=True)

        metadata_file = output_path / "envelope_test.txt"
        with open(metadata_file, 'w') as f:
            f.write("Envelope Timing Test\n")
            f.write("=" * 50 + "\n\n")
            f.write("This test measures ADSR envelope timing accuracy.\n\n")
            f.write("Manual test procedure:\n")
            f.write("1. Set filter envelope to control VCA (or use Amp Envelope)\n")
            f.write("2. Set envelope parameters:\n")
            f.write("   - Attack: various (0.001s, 0.01s, 0.1s, 0.5s, 1.0s)\n")
            f.write("   - Decay: various (0.01s, 0.1s, 0.5s, 1.0s, 5.0s)\n")
            f.write("   - Sustain: 0.5 (50%)\n")
            f.write("   - Release: various (0.01s, 0.1s, 0.5s, 1.0s, 5.0s)\n")
            f.write("3. Trigger 3-second note (hold for 3 seconds)\n")
            f.write("4. Record output for each parameter combination\n")
            f.write("5. Save as 'envelope_A{attack}_D{decay}_R{release}.wav'\n\n")
            f.write("Analysis:\n")
            f.write("  Measure actual attack/decay/release times from recordings.\n")
            f.write("  Compare to expected exponential curves.\n")
            f.write("  Verify timing accuracy (should be within 5%).\n")

        print(f"✓ Generated envelope test instructions in {output_path}/")

    def generate_lfo_test(self, output_path: Path):
        """Generate test files for analyzing LFO characteristics."""
        output_path.mkdir(parents=True, exist_ok=True)

        metadata_file = output_path / "lfo_test.txt"
        with open(metadata_file, 'w') as f:
            f.write("LFO Characteristics Test\n")
            f.write("=" * 50 + "\n\n")
            f.write("This test measures LFO waveform, rate, and modulation depth.\n\n")
            f.write("Manual test procedure:\n")
            f.write("1. Set LFO to modulate DCO pitch\n")
            f.write("2. Set DCO to pure sine wave (for clearest FM)\n")
            f.write("3. Play A4 (440 Hz) continuously\n")
            f.write("4. Test various LFO rates:\n")
            f.write("   - 0.1 Hz (very slow)\n")
            f.write("   - 1.0 Hz (moderate)\n")
            f.write("   - 5.0 Hz (fast)\n")
            f.write("   - 10.0 Hz (very fast)\n")
            f.write("5. Set LFO depth to maximum\n")
            f.write("6. Record 10 seconds for each rate\n")
            f.write("7. Save as 'lfo_rate_{rate}Hz.wav'\n\n")
            f.write("Analysis:\n")
            f.write("  Measure actual modulation rate via FFT or zero-crossing.\n")
            f.write("  Verify triangle waveform shape.\n")
            f.write("  Measure modulation depth (cents or Hz deviation).\n")

        print(f"✓ Generated LFO test instructions in {output_path}/")

    def generate_frequency_response_reference(self, output_path: Path):
        """
        Generate a perfect frequency sweep for comparing against filtered output.
        This is the theoretical reference for filter frequency response analysis.
        """
        output_path.mkdir(parents=True, exist_ok=True)

        # Generate logarithmic sine sweep (20 Hz - 20 kHz)
        sweep = self.generate_sine_sweep(
            duration=10.0,
            start_freq=20.0,
            end_freq=20000.0,
            amplitude=0.5
        )

        output_file = output_path / "frequency_sweep_reference.wav"
        sf.write(output_file, sweep, self.sample_rate)

        print(f"✓ Generated frequency sweep reference: {output_file}")
        print("  Use this to measure filter frequency response.")
        print("  Play through synth with varying filter settings,")
        print("  then analyze with measure_filter.py")

    def generate_all(self, output_dir: Path):
        """Generate all test files."""
        print("\nGenerating all reference test files...")
        print("=" * 60)

        self.generate_frequency_response_reference(output_dir / "frequency_response")
        self.generate_filter_sweep_test(output_dir / "filter_sweep")
        self.generate_resonance_sweep_test(output_dir / "resonance_sweep")
        self.generate_chorus_test(output_dir / "chorus")
        self.generate_envelope_test(output_dir / "envelope")
        self.generate_lfo_test(output_dir / "lfo")

        print("\n" + "=" * 60)
        print("✓ All reference files generated successfully!")
        print(f"\nOutput directory: {output_dir.absolute()}")
        print("\nNext steps:")
        print("1. Follow the instructions in each test's .txt file")
        print("2. Manually record Poor House Juno output (web interface)")
        print("3. Run analysis tools (measure_filter.py, measure_chorus.py, etc.)")
        print("4. Compare results to expected values")


def main():
    parser = argparse.ArgumentParser(
        description="Generate reference audio files for Poor House Juno analysis",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate all test files
  python generate_reference.py --all --output references/

  # Generate specific test
  python generate_reference.py --test filter_sweep --output references/

  # Generate with custom sample rate
  python generate_reference.py --all --output references/ --sample-rate 44100
        """
    )

    parser.add_argument('--output', '-o', type=str, required=True,
                       help='Output directory for reference files')
    parser.add_argument('--test', '-t', type=str,
                       choices=['frequency_response', 'filter_sweep', 'resonance_sweep',
                               'chorus', 'envelope', 'lfo', 'all'],
                       default='all',
                       help='Which test to generate (default: all)')
    parser.add_argument('--sample-rate', '-sr', type=int, default=48000,
                       help='Sample rate in Hz (default: 48000)')

    args = parser.parse_args()

    output_dir = Path(args.output)
    generator = ReferenceGenerator(sample_rate=args.sample_rate)

    test_map = {
        'frequency_response': lambda: generator.generate_frequency_response_reference(output_dir / "frequency_response"),
        'filter_sweep': lambda: generator.generate_filter_sweep_test(output_dir / "filter_sweep"),
        'resonance_sweep': lambda: generator.generate_resonance_sweep_test(output_dir / "resonance_sweep"),
        'chorus': lambda: generator.generate_chorus_test(output_dir / "chorus"),
        'envelope': lambda: generator.generate_envelope_test(output_dir / "envelope"),
        'lfo': lambda: generator.generate_lfo_test(output_dir / "lfo"),
        'all': lambda: generator.generate_all(output_dir)
    }

    try:
        test_map[args.test]()
    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
