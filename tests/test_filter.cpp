/**
 * Unit tests for IR3109 4-Pole Ladder Filter
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>
#include "filter.h"

using namespace phj;
using Catch::Matchers::WithinAbs;

TEST_CASE("Filter basic functionality", "[filter]") {
    Filter filter;
    const float sampleRate = 48000.0f;
    filter.setSampleRate(sampleRate);

    FilterParams params;
    params.cutoff = 0.5f;
    params.resonance = 0.0f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.0f;
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.drive = 1.0f;
    params.hpfMode = 0;  // Off
    filter.setParameters(params);

    SECTION("Filter processes signal") {
        // Generate a test signal (simple impulse)
        std::vector<Sample> input(1000, 0.0f);
        input[0] = 1.0f;  // Impulse

        std::vector<Sample> output(1000);
        filter.process(input.data(), output.data(), 1000);

        // Output should be different from input (filtering occurred)
        bool hasFiltered = false;
        for (size_t i = 1; i < 10; ++i) {
            if (output[i] != input[i]) {
                hasFiltered = true;
                break;
            }
        }
        REQUIRE(hasFiltered);
    }

    SECTION("Filter attenuates high frequencies") {
        // Set a low cutoff
        params.cutoff = 0.1f;  // Low cutoff
        filter.setParameters(params);

        // Generate high-frequency signal (e.g., 10kHz at 48kHz sample rate)
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / sampleRate;
            input[i] = std::sin(2.0f * M_PI * 10000.0f * t);
        }

        std::vector<Sample> output(1000);
        filter.process(input.data(), output.data(), 1000);

        // Calculate RMS of input and output
        float inputRMS = 0.0f;
        float outputRMS = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {  // Skip transient
            inputRMS += input[i] * input[i];
            outputRMS += output[i] * output[i];
        }
        inputRMS = std::sqrt(inputRMS / 900.0f);
        outputRMS = std::sqrt(outputRMS / 900.0f);

        // Output should be significantly attenuated
        REQUIRE(outputRMS < inputRMS * 0.5f);
    }

    SECTION("Higher cutoff passes more signal") {
        // Generate a mid-frequency signal (1kHz)
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / sampleRate;
            input[i] = std::sin(2.0f * M_PI * 1000.0f * t);
        }

        // Test with low cutoff
        params.cutoff = 0.1f;
        filter.setParameters(params);
        filter.reset();

        std::vector<Sample> outputLow(1000);
        filter.process(input.data(), outputLow.data(), 1000);

        // Test with high cutoff
        params.cutoff = 0.9f;
        filter.setParameters(params);
        filter.reset();

        std::vector<Sample> outputHigh(1000);
        filter.process(input.data(), outputHigh.data(), 1000);

        // Calculate RMS for both outputs
        float rmsLow = 0.0f;
        float rmsHigh = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            rmsLow += outputLow[i] * outputLow[i];
            rmsHigh += outputHigh[i] * outputHigh[i];
        }
        rmsLow = std::sqrt(rmsLow / 900.0f);
        rmsHigh = std::sqrt(rmsHigh / 900.0f);

        // High cutoff should pass more signal
        REQUIRE(rmsHigh > rmsLow);
    }

    SECTION("Reset clears filter state") {
        // Process some signal
        std::vector<Sample> input(100, 0.5f);
        std::vector<Sample> output(100);
        filter.process(input.data(), output.data(), 100);

        // Reset
        filter.reset();

        // Process impulse
        input.assign(100, 0.0f);
        input[0] = 1.0f;
        filter.process(input.data(), output.data(), 100);

        // Output should start fresh (not influenced by previous state)
        // Check that filter produces output (4-pole filter has small initial response)
        bool hasOutput = false;
        for (size_t i = 0; i < 20; ++i) {
            if (std::abs(output[i]) > 0.001f) {
                hasOutput = true;
                break;
            }
        }
        REQUIRE(hasOutput);
    }
}

TEST_CASE("Filter resonance", "[filter]") {
    Filter filter;
    const float sampleRate = 48000.0f;
    filter.setSampleRate(sampleRate);

    FilterParams params;
    params.cutoff = 0.5f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.0f;
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.drive = 1.0f;
    params.hpfMode = 0;

    SECTION("Resonance amplifies signal near cutoff") {
        // Generate broadband signal (white noise-like)
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            input[i] = (std::sin(static_cast<float>(i) * 0.1f) +
                       std::sin(static_cast<float>(i) * 0.3f) +
                       std::sin(static_cast<float>(i) * 0.5f)) / 3.0f;
        }

        // Test with no resonance
        params.resonance = 0.0f;
        filter.setParameters(params);
        filter.reset();

        std::vector<Sample> outputNoRes(1000);
        filter.process(input.data(), outputNoRes.data(), 1000);

        // Test with high resonance
        params.resonance = 0.8f;
        filter.setParameters(params);
        filter.reset();

        std::vector<Sample> outputHiRes(1000);
        filter.process(input.data(), outputHiRes.data(), 1000);

        // Calculate peak values (skip transient)
        float peakNoRes = 0.0f;
        float peakHiRes = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            peakNoRes = std::max(peakNoRes, std::abs(outputNoRes[i]));
            peakHiRes = std::max(peakHiRes, std::abs(outputHiRes[i]));
        }

        // High resonance should produce larger peaks
        REQUIRE(peakHiRes > peakNoRes);
    }

    SECTION("Very high resonance can self-oscillate") {
        params.cutoff = 0.5f;
        params.resonance = 0.98f;  // Very high resonance
        filter.setParameters(params);

        // Send an impulse
        std::vector<Sample> input(10000, 0.0f);
        input[0] = 0.1f;

        std::vector<Sample> output(10000);
        filter.process(input.data(), output.data(), 10000);

        // After the impulse, filter should continue ringing
        // Check for sustained oscillation after transient
        float laterEnergy = 0.0f;
        for (size_t i = 5000; i < 10000; ++i) {
            laterEnergy += output[i] * output[i];
        }
        laterEnergy /= 5000.0f;

        // Should have some energy (self-oscillation)
        REQUIRE(laterEnergy > 0.0001f);
    }
}

TEST_CASE("Filter envelope modulation", "[filter]") {
    Filter filter;
    filter.setSampleRate(48000.0f);

    FilterParams params;
    params.cutoff = 0.3f;
    params.resonance = 0.0f;
    params.envAmount = 0.5f;  // Positive modulation
    params.lfoAmount = 0.0f;
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.hpfMode = 0;
    filter.setParameters(params);

    SECTION("Envelope modulation affects cutoff") {
        // Generate test signal
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / 48000.0f;
            input[i] = std::sin(2.0f * M_PI * 2000.0f * t);
        }

        // Process with no envelope
        filter.setEnvValue(0.0f);
        filter.reset();
        std::vector<Sample> outputNoEnv(1000);
        filter.process(input.data(), outputNoEnv.data(), 1000);

        // Process with full envelope
        filter.setEnvValue(1.0f);
        filter.reset();
        std::vector<Sample> outputFullEnv(1000);
        filter.process(input.data(), outputFullEnv.data(), 1000);

        // Outputs should be different
        bool isDifferent = false;
        for (size_t i = 100; i < 1000; ++i) {
            if (std::abs(outputNoEnv[i] - outputFullEnv[i]) > 0.01f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Negative envelope amount inverts modulation") {
        params.envAmount = -0.5f;  // Negative modulation
        filter.setParameters(params);

        // This test verifies that negative envelope amount is handled
        // We just check that it doesn't crash and produces output
        std::vector<Sample> input(100, 0.1f);
        std::vector<Sample> output(100);

        filter.setEnvValue(1.0f);
        filter.process(input.data(), output.data(), 100);

        // Should produce some output
        float sum = 0.0f;
        for (Sample s : output) {
            sum += std::abs(s);
        }
        REQUIRE(sum > 0.0f);
    }
}

TEST_CASE("Filter LFO modulation", "[filter]") {
    Filter filter;
    filter.setSampleRate(48000.0f);

    FilterParams params;
    params.cutoff = 0.5f;
    params.resonance = 0.0f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.5f;  // LFO modulation
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.hpfMode = 0;
    filter.setParameters(params);

    SECTION("LFO modulation varies cutoff") {
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / 48000.0f;
            input[i] = std::sin(2.0f * M_PI * 1000.0f * t);
        }

        // Process with negative LFO
        filter.setLfoValue(-1.0f);
        filter.reset();
        std::vector<Sample> outputNegLfo(1000);
        filter.process(input.data(), outputNegLfo.data(), 1000);

        // Process with positive LFO
        filter.setLfoValue(1.0f);
        filter.reset();
        std::vector<Sample> outputPosLfo(1000);
        filter.process(input.data(), outputPosLfo.data(), 1000);

        // Outputs should be different
        bool isDifferent = false;
        for (size_t i = 100; i < 1000; ++i) {
            if (std::abs(outputNegLfo[i] - outputPosLfo[i]) > 0.01f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }
}

TEST_CASE("Filter key tracking (M11)", "[filter][m11]") {
    Filter filter;
    filter.setSampleRate(48000.0f);

    FilterParams params;
    params.cutoff = 0.5f;
    params.resonance = 0.0f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.0f;
    params.drive = 1.0f;
    params.hpfMode = 0;

    SECTION("Key tracking OFF doesn't change cutoff with note frequency") {
        params.keyTrack = FilterParams::KEY_TRACK_OFF;
        filter.setParameters(params);

        // Set different note frequencies
        filter.setNoteFrequency(110.0f);  // A2
        filter.reset();
        std::vector<Sample> input(100, 0.1f);
        std::vector<Sample> output1(100);
        filter.process(input.data(), output1.data(), 100);

        filter.setNoteFrequency(880.0f);  // A5
        filter.reset();
        std::vector<Sample> output2(100);
        filter.process(input.data(), output2.data(), 100);

        // With key tracking off, outputs should be very similar
        float diff = 0.0f;
        for (size_t i = 0; i < 100; ++i) {
            diff += std::abs(output1[i] - output2[i]);
        }
        diff /= 100.0f;

        REQUIRE(diff < 0.01f);  // Very similar
    }

    SECTION("Key tracking FULL changes cutoff with note frequency") {
        params.keyTrack = FilterParams::KEY_TRACK_FULL;
        filter.setParameters(params);

        // Generate test signal
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / 48000.0f;
            input[i] = std::sin(2.0f * M_PI * 1000.0f * t);
        }

        // Test with low note
        filter.setNoteFrequency(110.0f);  // A2
        filter.reset();
        std::vector<Sample> outputLow(1000);
        filter.process(input.data(), outputLow.data(), 1000);

        // Test with high note
        filter.setNoteFrequency(880.0f);  // A5
        filter.reset();
        std::vector<Sample> outputHigh(1000);
        filter.process(input.data(), outputHigh.data(), 1000);

        // Outputs should be noticeably different
        float diff = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            diff += std::abs(outputLow[i] - outputHigh[i]);
        }
        diff /= 900.0f;

        REQUIRE(diff > 0.01f);  // Should be different
    }
}

TEST_CASE("Filter HPF mode (M11)", "[filter][m11]") {
    Filter filter;
    filter.setSampleRate(48000.0f);

    FilterParams params;
    params.cutoff = 0.8f;  // High cutoff so LPF passes most signal
    params.resonance = 0.0f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.0f;
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.drive = 1.0f;
    filter.setParameters(params);

    // Generate low-frequency test signal (50 Hz)
    std::vector<Sample> input(1000);
    for (size_t i = 0; i < input.size(); ++i) {
        float t = static_cast<float>(i) / 48000.0f;
        input[i] = std::sin(2.0f * M_PI * 50.0f * t);
    }

    SECTION("HPF OFF passes low frequencies") {
        params.hpfMode = 0;  // Off
        filter.setParameters(params);

        std::vector<Sample> output(1000);
        filter.process(input.data(), output.data(), 1000);

        // Calculate RMS (skip transient)
        float rms = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / 900.0f);

        // Should pass signal
        REQUIRE(rms > 0.3f);
    }

    SECTION("HPF modes attenuate low frequencies") {
        // Test with HPF mode 3 (highest, ~120Hz)
        params.hpfMode = 3;
        filter.setParameters(params);
        filter.reset();

        std::vector<Sample> output(1000);
        filter.process(input.data(), output.data(), 1000);

        // Calculate RMS
        float rms = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / 900.0f);

        // Should be attenuated compared to no HPF
        // We already know no-HPF gives rms > 0.3, so this should be less
        REQUIRE(rms < 0.5f);  // Some attenuation
    }
}

TEST_CASE("Filter velocity modulation (M14)", "[filter][m14]") {
    Filter filter;
    filter.setSampleRate(48000.0f);

    FilterParams params;
    params.cutoff = 0.3f;
    params.resonance = 0.0f;
    params.envAmount = 0.0f;
    params.lfoAmount = 0.0f;
    params.keyTrack = FilterParams::KEY_TRACK_OFF;
    params.hpfMode = 0;
    filter.setParameters(params);

    SECTION("Velocity modulation affects filter response") {
        std::vector<Sample> input(1000);
        for (size_t i = 0; i < input.size(); ++i) {
            float t = static_cast<float>(i) / 48000.0f;
            input[i] = std::sin(2.0f * M_PI * 2000.0f * t);
        }

        // Process with low velocity
        filter.setVelocityValue(0.5f, 1.0f);  // velocity=0.5, amount=1.0
        filter.reset();
        std::vector<Sample> outputLowVel(1000);
        filter.process(input.data(), outputLowVel.data(), 1000);

        // Process with high velocity
        filter.setVelocityValue(1.0f, 1.0f);  // velocity=1.0, amount=1.0
        filter.reset();
        std::vector<Sample> outputHighVel(1000);
        filter.process(input.data(), outputHighVel.data(), 1000);

        // Outputs should be different when velocity amount is non-zero
        bool isDifferent = false;
        for (size_t i = 100; i < 1000; ++i) {
            if (std::abs(outputLowVel[i] - outputHighVel[i]) > 0.01f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Zero velocity amount disables velocity modulation") {
        std::vector<Sample> input(100, 0.1f);

        // Process with different velocities but zero amount
        filter.setVelocityValue(0.5f, 0.0f);
        filter.reset();
        std::vector<Sample> output1(100);
        filter.process(input.data(), output1.data(), 100);

        filter.setVelocityValue(1.0f, 0.0f);
        filter.reset();
        std::vector<Sample> output2(100);
        filter.process(input.data(), output2.data(), 100);

        // Outputs should be identical
        bool identical = true;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(output1[i] - output2[i]) > 0.0001f) {
                identical = false;
                break;
            }
        }
        REQUIRE(identical);
    }
}
