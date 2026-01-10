#!/usr/bin/env python3
"""
Poor House Juno - TAL Parameter Behavior Analyzer

Analyzes TAL-U-NO-LX parameter behavior and extracts DSP characteristics.
This helps verify that Poor House Juno matches TAL's parameter curves.

Usage:
    python analyze_tal.py --parameter filter_cutoff --data data/filter_cutoff.csv
    python analyze_tal.py --analyze-all --input-dir data/
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from pathlib import Path
from typing import Tuple, Optional, Callable, Dict
import csv
import sys


class ParameterAnalyzer:
    """Analyzes parameter curves and fits mathematical models."""

    @staticmethod
    def linear_curve(x: np.ndarray, a: float, b: float) -> np.ndarray:
        """Linear: y = a*x + b"""
        return a * x + b

    @staticmethod
    def exponential_curve(x: np.ndarray, a: float, b: float, c: float) -> np.ndarray:
        """Exponential: y = a * exp(b*x) + c"""
        return a * np.exp(b * x) + c

    @staticmethod
    def logarithmic_curve(x: np.ndarray, a: float, b: float) -> np.ndarray:
        """Logarithmic: y = a * log(x + 1) + b"""
        return a * np.log(x + 1.0) + b

    @staticmethod
    def power_curve(x: np.ndarray, a: float, b: float, c: float) -> np.ndarray:
        """Power: y = a * (x^b) + c"""
        return a * (x ** b) + c

    def fit_curve(self,
                  x_data: np.ndarray,
                  y_data: np.ndarray,
                  curve_type: str = 'auto') -> Tuple[Callable, np.ndarray, str, float]:
        """
        Fit a curve to parameter data.

        Args:
            x_data: Parameter values (0.0 - 1.0)
            y_data: Measured output values
            curve_type: 'linear', 'exponential', 'logarithmic', 'power', or 'auto'

        Returns:
            Tuple of (fitted_function, parameters, curve_name, r_squared)
        """
        curves_to_try = {
            'linear': (self.linear_curve, [1.0, 0.0]),
            'exponential': (self.exponential_curve, [1.0, 1.0, 0.0]),
            'logarithmic': (self.logarithmic_curve, [1.0, 0.0]),
            'power': (self.power_curve, [1.0, 2.0, 0.0])
        }

        if curve_type != 'auto':
            curves_to_try = {curve_type: curves_to_try[curve_type]}

        best_fit = None
        best_r_squared = -np.inf
        best_name = None
        best_params = None

        for name, (func, initial_guess) in curves_to_try.items():
            try:
                params, _ = curve_fit(func, x_data, y_data, p0=initial_guess, maxfev=10000)

                # Compute R-squared
                y_pred = func(x_data, *params)
                ss_res = np.sum((y_data - y_pred) ** 2)
                ss_tot = np.sum((y_data - np.mean(y_data)) ** 2)
                r_squared = 1.0 - (ss_res / ss_tot)

                if r_squared > best_r_squared:
                    best_r_squared = r_squared
                    best_fit = lambda x, f=func, p=params: f(x, *p)
                    best_name = name
                    best_params = params

            except Exception as e:
                print(f"  Warning: Failed to fit {name} curve: {e}")
                continue

        if best_fit is None:
            raise ValueError("Could not fit any curve to the data")

        return best_fit, best_params, best_name, best_r_squared

    def analyze_filter_cutoff(self,
                             param_values: np.ndarray,
                             cutoff_hz: np.ndarray,
                             output_path: Optional[Path] = None) -> Dict:
        """
        Analyze filter cutoff frequency parameter curve.

        Args:
            param_values: Parameter values (0.0 - 1.0)
            cutoff_hz: Measured cutoff frequencies in Hz
            output_path: Path to save plot

        Returns:
            Analysis results dictionary
        """
        print("\n" + "=" * 60)
        print("FILTER CUTOFF ANALYSIS")
        print("=" * 60)

        # Fit curve
        fitted_func, params, curve_name, r_squared = self.fit_curve(
            param_values, cutoff_hz, 'auto')

        print(f"\nBest fit: {curve_name}")
        print(f"Parameters: {params}")
        print(f"R²: {r_squared:.6f}")

        # Expected range for Juno-106 filter: ~30 Hz to ~12 kHz
        min_freq = np.min(cutoff_hz)
        max_freq = np.max(cutoff_hz)
        print(f"\nFrequency range:")
        print(f"  Minimum: {min_freq:.1f} Hz")
        print(f"  Maximum: {max_freq:.1f} Hz")

        # Plot
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

        # Linear scale
        ax1.plot(param_values, cutoff_hz, 'o', label='Measured', markersize=6)
        x_fit = np.linspace(0, 1, 100)
        y_fit = fitted_func(x_fit)
        ax1.plot(x_fit, y_fit, '-', label=f'{curve_name} fit (R²={r_squared:.4f})', linewidth=2)
        ax1.set_xlabel('Parameter Value')
        ax1.set_ylabel('Cutoff Frequency (Hz)')
        ax1.set_title('Filter Cutoff Curve (Linear)')
        ax1.grid(True, alpha=0.3)
        ax1.legend()

        # Log scale
        ax2.plot(param_values, cutoff_hz, 'o', label='Measured', markersize=6)
        ax2.plot(x_fit, y_fit, '-', label=f'{curve_name} fit', linewidth=2)
        ax2.set_xlabel('Parameter Value')
        ax2.set_ylabel('Cutoff Frequency (Hz)')
        ax2.set_title('Filter Cutoff Curve (Log Scale)')
        ax2.set_yscale('log')
        ax2.grid(True, alpha=0.3, which='both')
        ax2.legend()

        plt.tight_layout()

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"\n✓ Saved plot to {output_path}")
        else:
            plt.show()

        plt.close()

        return {
            'curve_type': curve_name,
            'parameters': params,
            'r_squared': r_squared,
            'min_freq': min_freq,
            'max_freq': max_freq
        }

    def analyze_envelope_timing(self,
                               param_values: np.ndarray,
                               time_seconds: np.ndarray,
                               param_name: str = "Attack",
                               output_path: Optional[Path] = None) -> Dict:
        """
        Analyze envelope timing parameter curve (Attack, Decay, Release).

        Args:
            param_values: Parameter values (0.0 - 1.0)
            time_seconds: Measured times in seconds
            param_name: Parameter name (Attack/Decay/Release)
            output_path: Path to save plot

        Returns:
            Analysis results
        """
        print("\n" + "=" * 60)
        print(f"ENVELOPE {param_name.upper()} TIMING ANALYSIS")
        print("=" * 60)

        # Envelope times are typically exponential
        fitted_func, params, curve_name, r_squared = self.fit_curve(
            param_values, time_seconds, 'exponential')

        print(f"\nBest fit: {curve_name}")
        print(f"Parameters: {params}")
        print(f"R²: {r_squared:.6f}")

        min_time = np.min(time_seconds)
        max_time = np.max(time_seconds)
        print(f"\nTime range:")
        print(f"  Minimum: {min_time:.4f} s")
        print(f"  Maximum: {max_time:.4f} s")

        # Plot
        plt.figure(figsize=(10, 6))
        plt.plot(param_values, time_seconds, 'o', label='Measured', markersize=8)
        x_fit = np.linspace(0, 1, 100)
        y_fit = fitted_func(x_fit)
        plt.plot(x_fit, y_fit, '-', label=f'{curve_name} fit (R²={r_squared:.4f})', linewidth=2)
        plt.xlabel('Parameter Value')
        plt.ylabel('Time (seconds)')
        plt.title(f'Envelope {param_name} Time Curve')
        plt.grid(True, alpha=0.3)
        plt.legend()

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"\n✓ Saved plot to {output_path}")
        else:
            plt.show()

        plt.close()

        return {
            'curve_type': curve_name,
            'parameters': params,
            'r_squared': r_squared,
            'min_time': min_time,
            'max_time': max_time
        }

    def load_csv_data(self, csv_path: Path) -> Tuple[np.ndarray, np.ndarray]:
        """
        Load parameter data from CSV file.

        Expected format:
        parameter_value,measured_value
        0.0,30.5
        0.1,45.2
        ...

        Returns:
            Tuple of (parameter_values, measured_values)
        """
        param_values = []
        measured_values = []

        with open(csv_path, 'r') as f:
            reader = csv.reader(f)
            header = next(reader, None)  # Skip header if present

            for row in reader:
                if len(row) >= 2:
                    try:
                        param_values.append(float(row[0]))
                        measured_values.append(float(row[1]))
                    except ValueError:
                        continue  # Skip invalid rows

        return np.array(param_values), np.array(measured_values)


def main():
    parser = argparse.ArgumentParser(
        description="Analyze TAL-U-NO-LX parameter behavior",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze filter cutoff curve from CSV
  python analyze_tal.py --parameter filter_cutoff --data filter_cutoff.csv --output cutoff_plot.png

  # Analyze envelope attack timing
  python analyze_tal.py --parameter envelope_attack --data attack_times.csv

CSV Format:
  parameter_value,measured_value
  0.0,30.5
  0.1,65.3
  0.2,120.8
  ...

Note: This tool requires pre-collected data from TAL-U-NO-LX or Poor House Juno.
Use the web interface or VST automation to generate parameter sweep data.
        """
    )

    parser.add_argument('--parameter', '-p', type=str,
                       choices=['filter_cutoff', 'filter_resonance',
                               'envelope_attack', 'envelope_decay', 'envelope_release',
                               'lfo_rate'],
                       help='Parameter to analyze')
    parser.add_argument('--data', '-d', type=str,
                       help='CSV file with parameter data')
    parser.add_argument('--output', '-o', type=str,
                       help='Output plot path (PNG)')
    parser.add_argument('--curve-type', '-c', type=str,
                       choices=['linear', 'exponential', 'logarithmic', 'power', 'auto'],
                       default='auto',
                       help='Curve type to fit (default: auto)')

    args = parser.parse_args()

    if not args.parameter or not args.data:
        print("Error: Must specify --parameter and --data", file=sys.stderr)
        parser.print_help()
        return 1

    analyzer = ParameterAnalyzer()

    try:
        # Load data
        data_path = Path(args.data)
        if not data_path.exists():
            print(f"Error: Data file not found: {data_path}", file=sys.stderr)
            return 1

        param_values, measured_values = analyzer.load_csv_data(data_path)

        if len(param_values) == 0:
            print("Error: No valid data found in CSV file", file=sys.stderr)
            return 1

        print(f"Loaded {len(param_values)} data points from {data_path}")

        output_path = Path(args.output) if args.output else None

        # Analyze based on parameter type
        if args.parameter == 'filter_cutoff':
            results = analyzer.analyze_filter_cutoff(
                param_values, measured_values, output_path)

        elif args.parameter in ['envelope_attack', 'envelope_decay', 'envelope_release']:
            param_name = args.parameter.split('_')[1].capitalize()
            results = analyzer.analyze_envelope_timing(
                param_values, measured_values, param_name, output_path)

        elif args.parameter == 'filter_resonance':
            print("\n" + "=" * 60)
            print("FILTER RESONANCE ANALYSIS")
            print("=" * 60)

            fitted_func, params, curve_name, r_squared = analyzer.fit_curve(
                param_values, measured_values, args.curve_type)

            print(f"\nBest fit: {curve_name}")
            print(f"Parameters: {params}")
            print(f"R²: {r_squared:.6f}")

            plt.figure(figsize=(10, 6))
            plt.plot(param_values, measured_values, 'o', label='Measured', markersize=8)
            x_fit = np.linspace(0, 1, 100)
            y_fit = fitted_func(x_fit)
            plt.plot(x_fit, y_fit, '-', label=f'{curve_name} fit (R²={r_squared:.4f})', linewidth=2)
            plt.xlabel('Resonance Parameter')
            plt.ylabel('Measured Value (Q factor or peak dB)')
            plt.title('Filter Resonance Curve')
            plt.grid(True, alpha=0.3)
            plt.legend()

            if output_path:
                plt.savefig(output_path, dpi=150, bbox_inches='tight')
                print(f"\n✓ Saved plot to {output_path}")
            else:
                plt.show()

            plt.close()

        elif args.parameter == 'lfo_rate':
            print("\n" + "=" * 60)
            print("LFO RATE ANALYSIS")
            print("=" * 60)

            fitted_func, params, curve_name, r_squared = analyzer.fit_curve(
                param_values, measured_values, args.curve_type)

            print(f"\nBest fit: {curve_name}")
            print(f"Parameters: {params}")
            print(f"R²: {r_squared:.6f}")

            plt.figure(figsize=(10, 6))
            plt.plot(param_values, measured_values, 'o', label='Measured', markersize=8)
            x_fit = np.linspace(0, 1, 100)
            y_fit = fitted_func(x_fit)
            plt.plot(x_fit, y_fit, '-', label=f'{curve_name} fit (R²={r_squared:.4f})', linewidth=2)
            plt.xlabel('LFO Rate Parameter')
            plt.ylabel('Frequency (Hz)')
            plt.title('LFO Rate Curve')
            plt.grid(True, alpha=0.3)
            plt.legend()

            if output_path:
                plt.savefig(output_path, dpi=150, bbox_inches='tight')
                print(f"\n✓ Saved plot to {output_path}")
            else:
                plt.show()

            plt.close()

    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
