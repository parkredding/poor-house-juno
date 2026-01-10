#!/usr/bin/env python3
"""
Poor House Juno - Chorus Characteristics Analyzer

Measures BBD chorus delay times, modulation depth/rate, and stereo imaging.

Usage:
    python measure_chorus.py --input chorus_mode1.wav --mode 1
    python measure_chorus.py --phj phj_chorus.wav --tal tal_chorus.wav --compare
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.fft import fft, fftfreq
import soundfile as sf
from pathlib import Path
from typing import Tuple, Optional, Dict
import sys


class ChorusAnalyzer:
    """Analyzes chorus effect characteristics from audio recordings."""

    def __init__(self, sample_rate: int = 48000):
        self.sample_rate = sample_rate

    def measure_delay_time(self,
                          dry_signal: np.ndarray,
                          wet_signal: np.ndarray,
                          max_delay_samples: int = 1000) -> float:
        """
        Measure delay time using cross-correlation.

        Args:
            dry_signal: Original (dry) signal
            wet_signal: Chorus-processed signal
            max_delay_samples: Maximum delay to search (in samples)

        Returns:
            Delay time in milliseconds
        """
        # Compute cross-correlation
        correlation = signal.correlate(wet_signal, dry_signal, mode='full')

        # Find peak in expected delay range
        center = len(correlation) // 2
        search_start = center
        search_end = center + max_delay_samples

        if search_end > len(correlation):
            search_end = len(correlation)

        peak_idx = np.argmax(correlation[search_start:search_end]) + search_start
        delay_samples = peak_idx - center

        delay_ms = (delay_samples / self.sample_rate) * 1000.0

        return delay_ms

    def measure_modulation_rate(self,
                                stereo_audio: np.ndarray,
                                expected_rate_hz: float = 0.65,
                                search_range_hz: Tuple[float, float] = (0.1, 2.0)) -> float:
        """
        Measure LFO modulation rate by analyzing stereo difference signal.

        Args:
            stereo_audio: Stereo chorus output [samples, 2]
            expected_rate_hz: Expected modulation rate (for validation)
            search_range_hz: Frequency range to search for modulation

        Returns:
            Measured modulation rate in Hz
        """
        if stereo_audio.ndim == 1:
            print("Warning: Mono signal provided, cannot measure modulation rate")
            return 0.0

        # Compute difference between L and R channels
        # The chorus modulation will be prominent in the difference signal
        diff_signal = stereo_audio[:, 0] - stereo_audio[:, 1]

        # Compute FFT of difference signal
        fft_result = fft(diff_signal)
        freqs = fftfreq(len(diff_signal), 1.0 / self.sample_rate)

        # Only look at positive frequencies in search range
        mask = (freqs >= search_range_hz[0]) & (freqs <= search_range_hz[1])
        search_freqs = freqs[mask]
        search_magnitude = np.abs(fft_result[mask])

        # Find peak in search range
        peak_idx = np.argmax(search_magnitude)
        measured_rate = search_freqs[peak_idx]

        return measured_rate

    def measure_stereo_correlation(self,
                                   stereo_audio: np.ndarray) -> float:
        """
        Measure stereo correlation (stereo width).

        Returns:
            Correlation coefficient (-1 to 1)
            -1: fully out of phase
             0: uncorrelated (maximum width)
             1: fully correlated (mono)
        """
        if stereo_audio.ndim == 1:
            return 1.0  # Mono = fully correlated

        left = stereo_audio[:, 0]
        right = stereo_audio[:, 1]

        # Normalize
        left = left / (np.std(left) + 1e-10)
        right = right / (np.std(right) + 1e-10)

        # Compute correlation
        correlation = np.corrcoef(left, right)[0, 1]

        return correlation

    def analyze_chorus_mode(self,
                           audio: np.ndarray,
                           mode_name: str,
                           expected: Dict[str, float]) -> Dict[str, float]:
        """
        Analyze chorus characteristics for a specific mode.

        Args:
            audio: Stereo audio recording
            mode_name: Mode name (e.g., "Mode I", "Mode II")
            expected: Expected characteristics (delay_ms, depth_ms, rate_hz)

        Returns:
            Dictionary of measured characteristics
        """
        results = {}

        # Measure stereo correlation
        correlation = self.measure_stereo_correlation(audio)
        results['stereo_correlation'] = correlation

        # Measure modulation rate
        if audio.ndim > 1:
            rate = self.measure_modulation_rate(audio, expected.get('rate_hz', 0.65))
            results['modulation_rate_hz'] = rate
        else:
            results['modulation_rate_hz'] = 0.0

        # Report
        print(f"\n{mode_name} Analysis:")
        print(f"  Stereo Correlation: {correlation:.3f}")
        if audio.ndim > 1:
            print(f"  Modulation Rate: {rate:.2f} Hz (expected: {expected.get('rate_hz', 0.0):.2f} Hz)")

        return results

    def plot_chorus_analysis(self,
                            audio: np.ndarray,
                            mode_name: str,
                            output_path: Optional[Path] = None):
        """
        Plot chorus analysis (waveform, stereo field, spectrum).

        Args:
            audio: Stereo audio
            mode_name: Mode name for title
            output_path: Output file path
        """
        if audio.ndim == 1:
            print("Warning: Mono signal, limited analysis available")
            audio = np.stack([audio, audio], axis=1)

        fig, axes = plt.subplots(3, 1, figsize=(14, 10))

        # Time axis
        time = np.arange(len(audio)) / self.sample_rate

        # Plot 1: Waveform (first 0.1 seconds)
        plot_samples = int(0.1 * self.sample_rate)
        axes[0].plot(time[:plot_samples], audio[:plot_samples, 0], label='Left', alpha=0.7)
        axes[0].plot(time[:plot_samples], audio[:plot_samples, 1], label='Right', alpha=0.7)
        axes[0].set_xlabel('Time (s)')
        axes[0].set_ylabel('Amplitude')
        axes[0].set_title(f'{mode_name} - Waveform (first 0.1s)')
        axes[0].legend()
        axes[0].grid(True, alpha=0.3)

        # Plot 2: Stereo difference signal (shows modulation)
        diff_signal = audio[:, 0] - audio[:, 1]
        axes[1].plot(time[:plot_samples], diff_signal[:plot_samples], color='purple')
        axes[1].set_xlabel('Time (s)')
        axes[1].set_ylabel('L - R')
        axes[1].set_title(f'{mode_name} - Stereo Difference (L-R)')
        axes[1].grid(True, alpha=0.3)

        # Plot 3: Spectrum of difference signal (shows modulation rate)
        fft_result = fft(diff_signal)
        freqs = fftfreq(len(diff_signal), 1.0 / self.sample_rate)
        magnitude = np.abs(fft_result)

        # Only plot low frequencies (where LFO modulation appears)
        mask = (freqs > 0) & (freqs < 5.0)
        axes[2].plot(freqs[mask], magnitude[mask], color='purple')
        axes[2].set_xlabel('Frequency (Hz)')
        axes[2].set_ylabel('Magnitude')
        axes[2].set_title(f'{mode_name} - Modulation Spectrum (0-5 Hz)')
        axes[2].grid(True, alpha=0.3)

        plt.tight_layout()

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"✓ Saved plot to {output_path}")
        else:
            plt.show()

        plt.close()

    def compare_chorus_modes(self,
                            mode1_audio: np.ndarray,
                            mode2_audio: np.ndarray,
                            mode_both_audio: np.ndarray,
                            output_path: Optional[Path] = None):
        """
        Compare all three chorus modes.

        Args:
            mode1_audio: Mode I recording
            mode2_audio: Mode II recording
            mode_both_audio: Mode I+II recording
            output_path: Output plot path
        """
        fig, axes = plt.subplots(3, 2, figsize=(16, 12))

        modes = [
            (mode1_audio, "Mode I (2.5ms, 0.65Hz)"),
            (mode2_audio, "Mode II (4.0ms, 0.50Hz)"),
            (mode_both_audio, "Mode I+II (Both)")
        ]

        for i, (audio, name) in enumerate(modes):
            if audio.ndim == 1:
                audio = np.stack([audio, audio], axis=1)

            time = np.arange(len(audio)) / self.sample_rate
            plot_samples = min(int(0.05 * self.sample_rate), len(audio))

            # Left: Waveform
            axes[i, 0].plot(time[:plot_samples], audio[:plot_samples, 0],
                          label='Left', alpha=0.7)
            axes[i, 0].plot(time[:plot_samples], audio[:plot_samples, 1],
                          label='Right', alpha=0.7)
            axes[i, 0].set_xlabel('Time (s)')
            axes[i, 0].set_ylabel('Amplitude')
            axes[i, 0].set_title(f'{name} - Waveform')
            axes[i, 0].legend()
            axes[i, 0].grid(True, alpha=0.3)

            # Right: Stereo difference spectrum
            diff = audio[:, 0] - audio[:, 1]
            fft_result = fft(diff)
            freqs = fftfreq(len(diff), 1.0 / self.sample_rate)
            magnitude = np.abs(fft_result)

            mask = (freqs > 0) & (freqs < 3.0)
            axes[i, 1].plot(freqs[mask], magnitude[mask], color='purple')
            axes[i, 1].set_xlabel('Frequency (Hz)')
            axes[i, 1].set_ylabel('Magnitude')
            axes[i, 1].set_title(f'{name} - Modulation Spectrum')
            axes[i, 1].grid(True, alpha=0.3)

        plt.tight_layout()

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"✓ Saved comparison plot to {output_path}")
        else:
            plt.show()

        plt.close()


def main():
    parser = argparse.ArgumentParser(
        description="Analyze chorus effect characteristics",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze single chorus mode
  python measure_chorus.py --input chorus_mode1.wav --mode 1

  # Compare all three modes
  python measure_chorus.py --mode1 mode1.wav --mode2 mode2.wav --both both.wav --compare-modes

  # Compare PHJ vs TAL
  python measure_chorus.py --phj phj_chorus.wav --tal tal_chorus.wav --compare
        """
    )

    parser.add_argument('--input', '-i', type=str,
                       help='Input chorus recording')
    parser.add_argument('--mode', type=int, choices=[1, 2],
                       help='Chorus mode (1 or 2)')
    parser.add_argument('--mode1', type=str,
                       help='Mode I recording (for comparison)')
    parser.add_argument('--mode2', type=str,
                       help='Mode II recording (for comparison)')
    parser.add_argument('--both', type=str,
                       help='Mode I+II recording (for comparison)')
    parser.add_argument('--phj', type=str,
                       help='Poor House Juno recording')
    parser.add_argument('--tal', type=str,
                       help='TAL-U-NO-LX recording')
    parser.add_argument('--output', '-o', type=str,
                       help='Output plot path (PNG)')
    parser.add_argument('--compare-modes', action='store_true',
                       help='Compare all chorus modes')
    parser.add_argument('--compare', action='store_true',
                       help='Compare PHJ vs TAL')

    args = parser.parse_args()

    # Expected chorus characteristics
    expected_params = {
        1: {'delay_ms': 2.5, 'depth_ms': 0.5, 'rate_hz': 0.65},
        2: {'delay_ms': 4.0, 'depth_ms': 0.8, 'rate_hz': 0.50}
    }

    try:
        if args.compare_modes:
            # Compare all three modes
            if not all([args.mode1, args.mode2, args.both]):
                print("Error: --compare-modes requires --mode1, --mode2, and --both",
                     file=sys.stderr)
                return 1

            mode1, sr = sf.read(args.mode1)
            mode2, _ = sf.read(args.mode2)
            both, _ = sf.read(args.both)

            analyzer = ChorusAnalyzer(sample_rate=sr)

            # Analyze each mode
            print("\n" + "=" * 60)
            print("CHORUS MODE COMPARISON")
            print("=" * 60)

            analyzer.analyze_chorus_mode(mode1, "Mode I", expected_params[1])
            analyzer.analyze_chorus_mode(mode2, "Mode II", expected_params[2])
            analyzer.analyze_chorus_mode(both, "Mode I+II", {})

            output_path = Path(args.output) if args.output else None
            analyzer.compare_chorus_modes(mode1, mode2, both, output_path)

        elif args.input and args.mode:
            # Analyze single mode
            audio, sr = sf.read(args.input)
            analyzer = ChorusAnalyzer(sample_rate=sr)

            mode_name = f"Mode {['I', 'II'][args.mode - 1]}"
            expected = expected_params[args.mode]

            results = analyzer.analyze_chorus_mode(audio, mode_name, expected)

            output_path = Path(args.output) if args.output else None
            analyzer.plot_chorus_analysis(audio, mode_name, output_path)

        else:
            print("Error: Must specify either --input + --mode, or --compare-modes, or --compare",
                 file=sys.stderr)
            parser.print_help()
            return 1

    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
