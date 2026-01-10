#!/usr/bin/env python3
"""
Poor House Juno - Filter Frequency Response Analyzer

Measures filter frequency response from sweep recordings and compares
against expected behavior.

Usage:
    python measure_filter.py --input filter_sweep.wav --output filter_response.png
    python measure_filter.py --phj phj_sweep.wav --tal tal_sweep.wav --compare
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.fft import fft, fftfreq
import soundfile as sf
from pathlib import Path
from typing import Tuple, Optional
import sys


class FilterAnalyzer:
    """Analyzes filter frequency response from audio recordings."""

    def __init__(self, sample_rate: int = 48000):
        self.sample_rate = sample_rate

    def analyze_sweep_response(self,
                               input_sweep: np.ndarray,
                               output_sweep: np.ndarray,
                               start_freq: float = 20.0,
                               end_freq: float = 20000.0) -> Tuple[np.ndarray, np.ndarray]:
        """
        Analyze frequency response by comparing input sweep to output sweep.

        Args:
            input_sweep: Original unfiltered sweep
            output_sweep: Filtered sweep from synth
            start_freq: Sweep start frequency (Hz)
            end_freq: Sweep end frequency (Hz)

        Returns:
            Tuple of (frequencies, magnitude_db)
        """
        # Ensure same length
        min_len = min(len(input_sweep), len(output_sweep))
        input_sweep = input_sweep[:min_len]
        output_sweep = output_sweep[:min_len]

        # Compute FFT of both signals
        input_fft = fft(input_sweep)
        output_fft = fft(output_sweep)

        # Compute frequency response (output / input)
        # Avoid division by zero
        epsilon = 1e-10
        freq_response = output_fft / (input_fft + epsilon)

        # Compute magnitude in dB
        magnitude = np.abs(freq_response)
        magnitude_db = 20 * np.log10(magnitude + epsilon)

        # Compute frequency bins
        freqs = fftfreq(len(input_sweep), 1.0 / self.sample_rate)

        # Only keep positive frequencies up to Nyquist
        mask = (freqs > 0) & (freqs < self.sample_rate / 2)
        freqs = freqs[mask]
        magnitude_db = magnitude_db[mask]

        return freqs, magnitude_db

    def find_cutoff_frequency(self,
                             freqs: np.ndarray,
                             magnitude_db: np.ndarray,
                             threshold_db: float = -3.0) -> float:
        """
        Find the -3dB cutoff frequency.

        Args:
            freqs: Frequency bins
            magnitude_db: Magnitude response in dB
            threshold_db: Cutoff threshold (default: -3 dB)

        Returns:
            Cutoff frequency in Hz
        """
        # Normalize to 0 dB at passband
        passband_level = np.max(magnitude_db[:len(magnitude_db)//10])  # First 10% of spectrum
        normalized_db = magnitude_db - passband_level

        # Find first frequency below threshold
        below_threshold = np.where(normalized_db < threshold_db)[0]

        if len(below_threshold) > 0:
            idx = below_threshold[0]
            return freqs[idx]
        else:
            return freqs[-1]  # Cutoff beyond measurement range

    def measure_resonance_peak(self,
                              freqs: np.ndarray,
                              magnitude_db: np.ndarray) -> Tuple[float, float]:
        """
        Measure resonance peak frequency and height.

        Returns:
            Tuple of (peak_freq_hz, peak_height_db)
        """
        # Find peak in magnitude response
        peak_idx = np.argmax(magnitude_db)
        peak_freq = freqs[peak_idx]
        peak_height = magnitude_db[peak_idx]

        return peak_freq, peak_height

    def plot_frequency_response(self,
                               freqs: np.ndarray,
                               magnitude_db: np.ndarray,
                               title: str = "Filter Frequency Response",
                               output_path: Optional[Path] = None,
                               cutoff_freq: Optional[float] = None,
                               resonance_peak: Optional[Tuple[float, float]] = None):
        """
        Plot frequency response.

        Args:
            freqs: Frequency bins
            magnitude_db: Magnitude in dB
            title: Plot title
            output_path: Path to save plot (if None, display only)
            cutoff_freq: Optional cutoff frequency to mark
            resonance_peak: Optional (peak_freq, peak_height) to mark
        """
        plt.figure(figsize=(12, 6))

        # Plot magnitude response
        plt.semilogx(freqs, magnitude_db, linewidth=2, label='Magnitude Response')

        # Mark cutoff frequency
        if cutoff_freq is not None:
            plt.axvline(cutoff_freq, color='r', linestyle='--',
                       label=f'Cutoff: {cutoff_freq:.1f} Hz')
            plt.axhline(-3, color='r', linestyle=':', alpha=0.5,
                       label='-3 dB')

        # Mark resonance peak
        if resonance_peak is not None:
            peak_freq, peak_height = resonance_peak
            plt.plot(peak_freq, peak_height, 'ro', markersize=10,
                    label=f'Peak: {peak_freq:.1f} Hz, {peak_height:.1f} dB')

        plt.xlabel('Frequency (Hz)')
        plt.ylabel('Magnitude (dB)')
        plt.title(title)
        plt.grid(True, which='both', alpha=0.3)
        plt.legend()
        plt.xlim(20, 20000)

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"✓ Saved plot to {output_path}")
        else:
            plt.show()

        plt.close()

    def compare_responses(self,
                         freqs1: np.ndarray,
                         mag_db1: np.ndarray,
                         freqs2: np.ndarray,
                         mag_db2: np.ndarray,
                         label1: str = "PHJ",
                         label2: str = "TAL",
                         output_path: Optional[Path] = None):
        """
        Compare two frequency responses.

        Args:
            freqs1, mag_db1: First response (e.g., Poor House Juno)
            freqs2, mag_db2: Second response (e.g., TAL-U-NO-LX)
            label1, label2: Labels for legend
            output_path: Path to save plot
        """
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

        # Top plot: Both responses
        ax1.semilogx(freqs1, mag_db1, linewidth=2, label=label1, alpha=0.8)
        ax1.semilogx(freqs2, mag_db2, linewidth=2, label=label2, alpha=0.8)
        ax1.set_xlabel('Frequency (Hz)')
        ax1.set_ylabel('Magnitude (dB)')
        ax1.set_title('Filter Frequency Response Comparison')
        ax1.grid(True, which='both', alpha=0.3)
        ax1.legend()
        ax1.set_xlim(20, 20000)

        # Bottom plot: Difference (PHJ - TAL)
        # Interpolate to common frequency grid
        from scipy.interpolate import interp1d
        common_freqs = freqs1
        interp_func = interp1d(freqs2, mag_db2, kind='linear',
                              bounds_error=False, fill_value='extrapolate')
        mag_db2_interp = interp_func(common_freqs)

        difference = mag_db1 - mag_db2_interp

        ax2.semilogx(common_freqs, difference, linewidth=2, color='purple')
        ax2.axhline(0, color='k', linestyle='--', alpha=0.5)
        ax2.fill_between(common_freqs, -1, 1, alpha=0.2, color='green',
                        label='±1 dB tolerance')
        ax2.set_xlabel('Frequency (Hz)')
        ax2.set_ylabel('Difference (dB)')
        ax2.set_title(f'Difference: {label1} - {label2}')
        ax2.grid(True, which='both', alpha=0.3)
        ax2.legend()
        ax2.set_xlim(20, 20000)

        plt.tight_layout()

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"✓ Saved comparison plot to {output_path}")
        else:
            plt.show()

        plt.close()

        # Compute RMS error
        rms_error = np.sqrt(np.mean(difference ** 2))
        print(f"\nComparison metrics:")
        print(f"  RMS Error: {rms_error:.2f} dB")
        print(f"  Max Error: {np.max(np.abs(difference)):.2f} dB")

        return rms_error


def main():
    parser = argparse.ArgumentParser(
        description="Analyze filter frequency response from audio recordings",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze single sweep recording
  python measure_filter.py --input filtered_sweep.wav --reference unfiltered_sweep.wav

  # Compare PHJ vs TAL
  python measure_filter.py --phj phj_sweep.wav --tal tal_sweep.wav --compare

  # Analyze and find cutoff frequency
  python measure_filter.py --input sweep.wav --reference ref.wav --find-cutoff
        """
    )

    parser.add_argument('--input', '-i', type=str,
                       help='Input filtered sweep recording')
    parser.add_argument('--reference', '-r', type=str,
                       help='Reference unfiltered sweep')
    parser.add_argument('--phj', type=str,
                       help='Poor House Juno sweep (for comparison)')
    parser.add_argument('--tal', type=str,
                       help='TAL-U-NO-LX sweep (for comparison)')
    parser.add_argument('--output', '-o', type=str,
                       help='Output plot path (PNG)')
    parser.add_argument('--find-cutoff', action='store_true',
                       help='Find and report -3dB cutoff frequency')
    parser.add_argument('--find-resonance', action='store_true',
                       help='Find and report resonance peak')
    parser.add_argument('--compare', action='store_true',
                       help='Compare PHJ vs TAL (requires --phj and --tal)')

    args = parser.parse_args()

    # Determine sample rate from first input file
    if args.input:
        audio, sr = sf.read(args.input)
    elif args.phj:
        audio, sr = sf.read(args.phj)
    else:
        print("Error: Must specify either --input or --phj", file=sys.stderr)
        return 1

    analyzer = FilterAnalyzer(sample_rate=sr)

    try:
        if args.compare:
            # Comparison mode
            if not args.phj or not args.tal:
                print("Error: --compare requires both --phj and --tal", file=sys.stderr)
                return 1

            phj_audio, phj_sr = sf.read(args.phj)
            tal_audio, tal_sr = sf.read(args.tal)

            if phj_audio.ndim > 1:
                phj_audio = phj_audio[:, 0]  # Use left channel
            if tal_audio.ndim > 1:
                tal_audio = tal_audio[:, 0]

            # For comparison, we need the reference sweep
            # Generate it on the fly
            duration = len(phj_audio) / phj_sr
            t = np.linspace(0, duration, len(phj_audio))
            k = (20000.0 / 20.0) ** (1.0 / duration)
            phase = 2.0 * np.pi * 20.0 * (k ** t - 1.0) / np.log(k)
            reference = 0.5 * np.sin(phase)

            freqs_phj, mag_phj = analyzer.analyze_sweep_response(
                reference, phj_audio, 20.0, 20000.0)

            # Adjust reference for TAL (might be different length)
            t_tal = np.linspace(0, duration, len(tal_audio))
            phase_tal = 2.0 * np.pi * 20.0 * (k ** t_tal - 1.0) / np.log(k)
            reference_tal = 0.5 * np.sin(phase_tal)

            freqs_tal, mag_tal = analyzer.analyze_sweep_response(
                reference_tal, tal_audio, 20.0, 20000.0)

            output_path = Path(args.output) if args.output else None
            analyzer.compare_responses(freqs_phj, mag_phj, freqs_tal, mag_tal,
                                      "Poor House Juno", "TAL-U-NO-LX",
                                      output_path)

        else:
            # Single analysis mode
            if not args.reference:
                print("Error: --input requires --reference (unfiltered sweep)", file=sys.stderr)
                return 1

            input_audio, input_sr = sf.read(args.input)
            ref_audio, ref_sr = sf.read(args.reference)

            if input_audio.ndim > 1:
                input_audio = input_audio[:, 0]
            if ref_audio.ndim > 1:
                ref_audio = ref_audio[:, 0]

            freqs, mag_db = analyzer.analyze_sweep_response(
                ref_audio, input_audio, 20.0, 20000.0)

            cutoff_freq = None
            resonance_peak = None

            if args.find_cutoff:
                cutoff_freq = analyzer.find_cutoff_frequency(freqs, mag_db)
                print(f"✓ Cutoff frequency (-3 dB): {cutoff_freq:.1f} Hz")

            if args.find_resonance:
                resonance_peak = analyzer.measure_resonance_peak(freqs, mag_db)
                print(f"✓ Resonance peak: {resonance_peak[0]:.1f} Hz, {resonance_peak[1]:.1f} dB")

            output_path = Path(args.output) if args.output else None
            analyzer.plot_frequency_response(freqs, mag_db,
                                           "Filter Frequency Response",
                                           output_path,
                                           cutoff_freq, resonance_peak)

    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
