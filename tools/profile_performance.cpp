/**
 * CPU Performance Profiling Tool for Poor House Juno
 *
 * This tool measures the CPU time spent in different DSP components
 * to identify performance hotspots and verify optimization targets.
 *
 * Usage:
 *   g++ -O3 -std=c++17 -I../src profile_performance.cpp \
 *       ../src/dsp/*.cpp -o profile_performance
 *   ./profile_performance
 */

#include "../src/dsp/synth.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace phj;
using namespace std::chrono;

struct PerformanceStats {
    double totalTimeMs;
    double avgTimePerSample;
    double cpuPercentAt48kHz;
    int numSamples;
};

PerformanceStats profileSynth(int numSamples, int numVoices, bool chorusEnabled) {
    Synth synth;
    synth.setSampleRate(48000.0f);

    // Configure for maximum load
    ChorusParams chorusParams;
    chorusParams.mode = chorusEnabled ? Chorus::MODE_BOTH : Chorus::MODE_OFF;
    synth.setChorusParams(chorusParams);

    // Trigger voices
    for (int i = 0; i < numVoices && i < 6; ++i) {
        synth.handleNoteOn(60 + i, 0.8f); // C4 and up
    }

    // Enable modulation
    synth.handleModWheel(1.0f); // Max LFO depth

    // Prepare buffers
    std::vector<float> leftOut(numSamples);
    std::vector<float> rightOut(numSamples);

    // Profile processing
    auto start = high_resolution_clock::now();

    synth.processStereo(leftOut.data(), rightOut.data(), numSamples);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    // Calculate statistics
    PerformanceStats stats;
    stats.totalTimeMs = duration.count() / 1000.0;
    stats.avgTimePerSample = duration.count() / (double)numSamples;
    stats.numSamples = numSamples;

    // Calculate CPU percentage at 48kHz
    // Available time per sample at 48kHz: 1/48000 = 20.833 microseconds
    double availableTimePerSample = 1000000.0 / 48000.0; // microseconds
    stats.cpuPercentAt48kHz = (stats.avgTimePerSample / availableTimePerSample) * 100.0;

    return stats;
}

void printStats(const std::string& testName, const PerformanceStats& stats) {
    std::cout << "\n" << testName << ":\n";
    std::cout << "  Samples processed: " << stats.numSamples << "\n";
    std::cout << "  Total time: " << std::fixed << std::setprecision(3)
              << stats.totalTimeMs << " ms\n";
    std::cout << "  Time per sample: " << std::fixed << std::setprecision(3)
              << stats.avgTimePerSample << " µs\n";
    std::cout << "  CPU usage @ 48kHz: " << std::fixed << std::setprecision(1)
              << stats.cpuPercentAt48kHz << "%\n";

    if (stats.cpuPercentAt48kHz > 50.0) {
        std::cout << "  ⚠️  WARNING: Exceeds 50% CPU target!\n";
    } else {
        std::cout << "  ✓ Meets 50% CPU target\n";
    }
}

int main() {
    std::cout << "Poor House Juno - Performance Profiling\n";
    std::cout << "========================================\n";
    std::cout << "\nTarget: <50% CPU on Raspberry Pi 4 @ 48kHz\n";
    std::cout << "Buffer size: 128 samples (2.67ms @ 48kHz)\n";

    const int bufferSize = 128;
    const int numIterations = 1000; // Process 1000 buffers
    const int totalSamples = bufferSize * numIterations;

    // Test 1: Single voice, no chorus
    std::cout << "\n--- Test 1: Single Voice (baseline) ---\n";
    auto stats1 = profileSynth(totalSamples, 1, false);
    printStats("1 voice, no chorus", stats1);

    // Test 2: 6 voices, no chorus
    std::cout << "\n--- Test 2: Full Polyphony ---\n";
    auto stats2 = profileSynth(totalSamples, 6, false);
    printStats("6 voices, no chorus", stats2);

    // Test 3: 6 voices with chorus (worst case)
    std::cout << "\n--- Test 3: Full Load (worst case) ---\n";
    auto stats3 = profileSynth(totalSamples, 6, true);
    printStats("6 voices + chorus", stats3);

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Worst-case CPU: " << std::fixed << std::setprecision(1)
              << stats3.cpuPercentAt48kHz << "%\n";

    if (stats3.cpuPercentAt48kHz <= 50.0) {
        std::cout << "  ✓ PASSED: Meets <50% CPU target\n";
        return 0;
    } else {
        std::cout << "  ✗ FAILED: Exceeds 50% CPU target\n";
        std::cout << "  Optimization needed: "
                  << (stats3.cpuPercentAt48kHz - 50.0) << "% reduction required\n";
        return 1;
    }
}
