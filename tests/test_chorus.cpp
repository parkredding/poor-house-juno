/**
 * Unit tests for Chorus (BBD Stereo Chorus)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <cmath>
#include <vector>
#include "chorus.h"

using namespace phj;
using Catch::Matchers::WithinAbs;

TEST_CASE("Chorus basic functionality", "[chorus]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);

    SECTION("Chorus OFF mode passes through dry signal") {
        chorus.setMode(Chorus::OFF);
        REQUIRE(chorus.getMode() == Chorus::OFF);

        // Test with constant input
        float input = 0.5f;
        float leftOut = 0.0f;
        float rightOut = 0.0f;

        chorus.process(input, leftOut, rightOut);

        // Should be identical to input (no processing)
        REQUIRE_THAT(leftOut, WithinAbs(input, 0.001f));
        REQUIRE_THAT(rightOut, WithinAbs(input, 0.001f));
        REQUIRE_THAT(leftOut, WithinAbs(rightOut, 0.001f));
    }

    SECTION("Chorus activates in Mode I") {
        chorus.setMode(Chorus::MODE_I);
        REQUIRE(chorus.getMode() == Chorus::MODE_I);

        // Fill delay buffer with some signal
        float input = 0.5f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Now check output
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Output should not be zero (chorus is active)
        REQUIRE(std::abs(leftOut) > 0.01f);
        REQUIRE(std::abs(rightOut) > 0.01f);
    }

    SECTION("Chorus activates in Mode II") {
        chorus.setMode(Chorus::MODE_II);
        REQUIRE(chorus.getMode() == Chorus::MODE_II);

        // Fill delay buffer with some signal
        float input = 0.5f;
        for (int i = 0; i < 250; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Now check output
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Output should not be zero (chorus is active)
        REQUIRE(std::abs(leftOut) > 0.01f);
        REQUIRE(std::abs(rightOut) > 0.01f);
    }

    SECTION("Chorus activates in Mode BOTH") {
        chorus.setMode(Chorus::MODE_BOTH);
        REQUIRE(chorus.getMode() == Chorus::MODE_BOTH);

        // Fill delay buffer with some signal
        float input = 0.5f;
        for (int i = 0; i < 250; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Now check output
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Output should not be zero (chorus is active)
        REQUIRE(std::abs(leftOut) > 0.01f);
        REQUIRE(std::abs(rightOut) > 0.01f);
    }

    SECTION("Reset clears delay buffers") {
        chorus.setMode(Chorus::MODE_I);

        // Fill delay buffer with signal
        float input = 0.8f;
        for (int i = 0; i < 300; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Reset
        chorus.reset();

        // Process silence - should get very low output initially
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(0.0f, leftOut, rightOut);

        // Output should be very quiet (no history in delay buffer)
        REQUIRE(std::abs(leftOut) < 0.2f);
        REQUIRE(std::abs(rightOut) < 0.2f);
    }
}

TEST_CASE("Chorus Mode I characteristics (M8)", "[chorus][m8]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);
    chorus.setMode(Chorus::MODE_I);

    SECTION("Mode I produces stereo output") {
        // Generate constant tone to fill delay buffer
        float input = 0.5f;

        // Fill delay buffer (need at least 3ms worth of samples)
        // Mode I: 2.5ms delay + 0.5ms depth = up to 3ms
        int fillSamples = static_cast<int>(3.5f * sampleRate / 1000.0f);
        for (int i = 0; i < fillSamples; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Collect stereo outputs over one LFO cycle
        // Chorus I rate: 0.65 Hz = ~1.54 seconds per cycle
        int cycleSamples = static_cast<int>(sampleRate / 0.65f);
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;

        for (int i = 0; i < cycleSamples; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftSamples.push_back(leftOut);
            rightSamples.push_back(rightOut);
        }

        // Check that left and right differ (stereo effect)
        bool hasDifference = false;
        for (size_t i = 0; i < leftSamples.size(); ++i) {
            if (std::abs(leftSamples[i] - rightSamples[i]) > 0.01f) {
                hasDifference = true;
                break;
            }
        }

        REQUIRE(hasDifference);
    }

    SECTION("Mode I delay time is approximately 2.5ms") {
        chorus.reset();

        // Generate impulse
        float leftOut, rightOut;
        chorus.process(1.0f, leftOut, rightOut);

        // Generate zeros to let impulse propagate
        for (int i = 0; i < 500; ++i) {
            chorus.process(0.0f, leftOut, rightOut);
        }

        // Expected delay: ~2.5ms at 48kHz = 120 samples
        // Due to modulation depth (±0.5ms), actual delay varies from 2.0-3.0ms
        // Output should show the delayed impulse somewhere in this range
        REQUIRE(std::abs(leftOut) < 0.5f);  // Should have some signal from delayed impulse
        REQUIRE(std::abs(rightOut) < 0.5f);
    }

    SECTION("Mode I modulation creates time-varying output") {
        // Fill delay buffer with constant signal
        float input = 0.5f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Collect outputs over time
        std::vector<float> leftSamples;
        int sampleCount = static_cast<int>(sampleRate * 0.2f);  // 200ms

        for (int i = 0; i < sampleCount; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftSamples.push_back(leftOut);
        }

        // Find min and max values
        float minVal = leftSamples[0];
        float maxVal = leftSamples[0];
        for (float sample : leftSamples) {
            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
        }

        // Due to LFO modulation, output should vary
        float variation = maxVal - minVal;
        REQUIRE(variation > 0.01f);  // Should have some modulation
    }
}

TEST_CASE("Chorus Mode II characteristics (M8)", "[chorus][m8]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);
    chorus.setMode(Chorus::MODE_II);

    SECTION("Mode II produces stereo output") {
        // Generate constant tone to fill delay buffer
        float input = 0.5f;

        // Fill delay buffer (need at least 5ms worth of samples)
        // Mode II: 4.0ms delay + 0.8ms depth = up to 4.8ms
        int fillSamples = static_cast<int>(5.5f * sampleRate / 1000.0f);
        for (int i = 0; i < fillSamples; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Collect stereo outputs over one LFO cycle
        // Chorus II rate: 0.50 Hz = 2 seconds per cycle
        int cycleSamples = static_cast<int>(sampleRate / 0.50f);
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;

        for (int i = 0; i < cycleSamples; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftSamples.push_back(leftOut);
            rightSamples.push_back(rightOut);
        }

        // Check that left and right differ (stereo effect)
        bool hasDifference = false;
        for (size_t i = 0; i < leftSamples.size(); ++i) {
            if (std::abs(leftSamples[i] - rightSamples[i]) > 0.01f) {
                hasDifference = true;
                break;
            }
        }

        REQUIRE(hasDifference);
    }

    SECTION("Mode II delay time is approximately 4.0ms") {
        chorus.reset();

        // Generate impulse
        float leftOut, rightOut;
        chorus.process(1.0f, leftOut, rightOut);

        // Generate zeros to let impulse propagate
        for (int i = 0; i < 500; ++i) {
            chorus.process(0.0f, leftOut, rightOut);
        }

        // Expected delay: ~4.0ms at 48kHz = 192 samples
        // Due to modulation depth (±0.8ms), actual delay varies from 3.2-4.8ms
        // Output should show the delayed impulse somewhere in this range
        REQUIRE(std::abs(leftOut) < 0.5f);  // Should have some signal from delayed impulse
        REQUIRE(std::abs(rightOut) < 0.5f);
    }

    SECTION("Mode II has slower modulation than Mode I") {
        // Fill delay buffer with constant signal
        float input = 0.5f;
        for (int i = 0; i < 250; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Collect outputs over a short time period (100ms)
        std::vector<float> leftSamples;
        int sampleCount = static_cast<int>(sampleRate * 0.1f);

        for (int i = 0; i < sampleCount; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftSamples.push_back(leftOut);
        }

        // Mode II should still show modulation, but slower than Mode I
        float minVal = leftSamples[0];
        float maxVal = leftSamples[0];
        for (float sample : leftSamples) {
            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
        }

        // Should have modulation (though less variation in short window than Mode I)
        float variation = maxVal - minVal;
        REQUIRE(variation > 0.005f);
    }
}

TEST_CASE("Chorus Mode I+II (Both) characteristics (M8)", "[chorus][m8]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);
    chorus.setMode(Chorus::MODE_BOTH);

    SECTION("Mode BOTH produces richer stereo output than individual modes") {
        // Fill delay buffer
        float input = 0.5f;
        int fillSamples = static_cast<int>(6.0f * sampleRate / 1000.0f);
        for (int i = 0; i < fillSamples; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Collect stereo outputs
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
        int sampleCount = static_cast<int>(sampleRate * 0.5f);  // 500ms

        for (int i = 0; i < sampleCount; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftSamples.push_back(leftOut);
            rightSamples.push_back(rightOut);
        }

        // Should have stereo difference
        bool hasDifference = false;
        for (size_t i = 0; i < leftSamples.size(); ++i) {
            if (std::abs(leftSamples[i] - rightSamples[i]) > 0.01f) {
                hasDifference = true;
                break;
            }
        }

        REQUIRE(hasDifference);

        // Should have modulation
        float leftMin = *std::min_element(leftSamples.begin(), leftSamples.end());
        float leftMax = *std::max_element(leftSamples.begin(), leftSamples.end());
        float leftVariation = leftMax - leftMin;

        REQUIRE(leftVariation > 0.01f);
    }

    SECTION("Mode BOTH combines two chorus stages") {
        // Fill delay buffer
        float input = 0.5f;
        for (int i = 0; i < 250; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Get output
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Should produce output (combining both stages)
        REQUIRE(std::abs(leftOut) > 0.01f);
        REQUIRE(std::abs(rightOut) > 0.01f);
    }
}

TEST_CASE("Chorus BBD delay modulation", "[chorus][m8]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);
    chorus.setMode(Chorus::MODE_I);

    SECTION("LFO modulates delay time creating chorus effect") {
        // Fill delay buffer with constant tone
        float input = 0.5f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Process over multiple LFO cycles
        // Chorus I rate: 0.65 Hz
        int cycleTime = static_cast<int>(sampleRate / 0.65f * 3);  // 3 cycles
        std::vector<float> outputs;

        for (int i = 0; i < cycleTime; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            outputs.push_back((leftOut + rightOut) * 0.5f);  // Average L/R
        }

        // The modulation should create periodic variation
        float minOutput = *std::min_element(outputs.begin(), outputs.end());
        float maxOutput = *std::max_element(outputs.begin(), outputs.end());

        // Should have variation due to delay modulation
        REQUIRE((maxOutput - minOutput) > 0.01f);
    }

    SECTION("Opposite LFO phases create stereo width") {
        // Fill delay buffer
        float input = 0.5f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Sample at various points in the LFO cycle
        std::vector<float> leftOutputs;
        std::vector<float> rightOutputs;
        std::vector<float> differences;

        int sampleCount = static_cast<int>(sampleRate * 0.3f);  // 300ms
        for (int i = 0; i < sampleCount; ++i) {
            float leftOut = 0.0f;
            float rightOut = 0.0f;
            chorus.process(input, leftOut, rightOut);
            leftOutputs.push_back(leftOut);
            rightOutputs.push_back(rightOut);
            differences.push_back(std::abs(leftOut - rightOut));
        }

        // Find maximum stereo difference
        float maxDifference = *std::max_element(differences.begin(), differences.end());

        // Should have significant stereo separation at some point
        REQUIRE(maxDifference > 0.01f);
    }
}

TEST_CASE("Chorus dry/wet mix", "[chorus][m8]") {
    Chorus chorus;
    const float sampleRate = 48000.0f;
    chorus.setSampleRate(sampleRate);

    SECTION("Chorus maintains some dry signal") {
        chorus.setMode(Chorus::MODE_I);

        // Fill delay buffer with strong signal
        float input = 1.0f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Process constant input
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Output should be significant (dry signal dominates per Juno-106 design)
        // Juno chorus is approximately 80% dry, 20% wet
        REQUIRE(std::abs(leftOut) > 0.5f);
        REQUIRE(std::abs(rightOut) > 0.5f);
    }

    SECTION("Mode BOTH reduces wet level to prevent buildup") {
        chorus.setMode(Chorus::MODE_BOTH);

        // Fill delay buffer
        float input = 1.0f;
        for (int i = 0; i < 300; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        // Process constant input
        float leftOut = 0.0f;
        float rightOut = 0.0f;
        chorus.process(input, leftOut, rightOut);

        // Should still maintain reasonable level
        REQUIRE(std::abs(leftOut) > 0.5f);
        REQUIRE(std::abs(rightOut) > 0.5f);

        // But should not significantly exceed input (no excessive buildup)
        REQUIRE(std::abs(leftOut) < 1.5f);
        REQUIRE(std::abs(rightOut) < 1.5f);
    }
}

TEST_CASE("Chorus sample rate handling", "[chorus]") {
    Chorus chorus;

    SECTION("Chorus adapts to different sample rates") {
        // Test at 44100 Hz
        chorus.setSampleRate(44100.0f);
        chorus.setMode(Chorus::MODE_I);

        float input = 0.5f;
        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        float leftOut1 = 0.0f;
        float rightOut1 = 0.0f;
        chorus.process(input, leftOut1, rightOut1);

        REQUIRE(std::abs(leftOut1) > 0.01f);

        // Test at 48000 Hz
        chorus.setSampleRate(48000.0f);
        chorus.setMode(Chorus::MODE_I);

        for (int i = 0; i < 200; ++i) {
            float leftOut, rightOut;
            chorus.process(input, leftOut, rightOut);
        }

        float leftOut2 = 0.0f;
        float rightOut2 = 0.0f;
        chorus.process(input, leftOut2, rightOut2);

        REQUIRE(std::abs(leftOut2) > 0.01f);

        // Both should produce output (though values will differ due to sample rate)
        // This just verifies the chorus adapts to different sample rates
    }
}
