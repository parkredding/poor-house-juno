/**
 * Unit tests for LFO (Low-Frequency Oscillator)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>
#include "lfo.h"

using namespace phj;
using Catch::Matchers::WithinAbs;

TEST_CASE("LFO triangle wave generation", "[lfo]") {
    Lfo lfo;
    const float sampleRate = 48000.0f;
    lfo.setSampleRate(sampleRate);
    lfo.setRate(1.0f);  // 1 Hz
    lfo.setDelay(0.0f);  // No delay

    SECTION("LFO generates triangle wave") {
        std::vector<float> samples;

        // Generate one complete cycle (48000 samples at 1 Hz)
        for (int i = 0; i < 48000; ++i) {
            samples.push_back(lfo.process());
        }

        // Find min and max values
        float minVal = 1.0f;
        float maxVal = -1.0f;
        for (float s : samples) {
            minVal = std::min(minVal, s);
            maxVal = std::max(maxVal, s);
        }

        // Should range from -1 to +1
        REQUIRE_THAT(minVal, WithinAbs(-1.0f, 0.1f));
        REQUIRE_THAT(maxVal, WithinAbs(1.0f, 0.1f));
    }

    SECTION("LFO starts at zero phase") {
        lfo.reset();
        float firstSample = lfo.process();

        // Triangle wave should start at 0
        REQUIRE_THAT(firstSample, WithinAbs(0.0f, 0.01f));
    }

    SECTION("LFO rate affects frequency") {
        lfo.reset();
        lfo.setRate(2.0f);  // 2 Hz

        // At 2 Hz with 48kHz sample rate, one cycle is 24000 samples
        // Generate enough samples to see at least one full cycle
        std::vector<float> samples;
        for (int i = 0; i < 24000; ++i) {
            samples.push_back(lfo.process());
        }

        // Should have both positive and negative values
        bool hasPositive = false;
        bool hasNegative = false;
        for (float s : samples) {
            if (s > 0.1f) hasPositive = true;
            if (s < -0.1f) hasNegative = true;
        }

        REQUIRE(hasPositive);
        REQUIRE(hasNegative);
    }

    SECTION("LFO is continuous across multiple process calls") {
        lfo.reset();
        lfo.setRate(1.0f);

        float lastValue = lfo.process();

        // Process several samples and verify continuity
        for (int i = 0; i < 100; ++i) {
            float currentValue = lfo.process();

            // Values should change gradually (not jump)
            float diff = std::abs(currentValue - lastValue);
            REQUIRE(diff < 0.01f);  // Small change per sample at 1 Hz

            lastValue = currentValue;
        }
    }
}

TEST_CASE("LFO delay functionality (M12)", "[lfo][m12]") {
    Lfo lfo;
    const float sampleRate = 48000.0f;
    lfo.setSampleRate(sampleRate);
    lfo.setRate(1.0f);  // 1 Hz
    lfo.setDelay(0.1f);  // 100ms delay

    SECTION("LFO starts at zero with delay enabled") {
        lfo.trigger();  // Trigger delay timer

        float firstValue = lfo.process();
        REQUIRE_THAT(firstValue, WithinAbs(0.0f, 0.01f));
    }

    SECTION("LFO gradually fades in during delay period") {
        lfo.trigger();

        // Process through delay period (100ms = 4800 samples at 48kHz)
        std::vector<float> samples;
        for (int i = 0; i < 5000; ++i) {
            samples.push_back(lfo.process());
        }

        // Early samples should be close to zero
        REQUIRE_THAT(std::abs(samples[0]), WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(std::abs(samples[100]), WithinAbs(0.0f, 0.1f));

        // Later samples should have more modulation
        float earlyMax = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            earlyMax = std::max(earlyMax, std::abs(samples[i]));
        }

        float lateMax = 0.0f;
        for (int i = 4000; i < 5000; ++i) {
            lateMax = std::max(lateMax, std::abs(samples[i]));
        }

        // Late samples should have larger amplitude than early samples
        REQUIRE(lateMax > earlyMax);
    }

    SECTION("LFO reaches full depth after delay period") {
        lfo.trigger();

        // Process well past delay period (200ms = 9600 samples)
        for (int i = 0; i < 10000; ++i) {
            lfo.process();
        }

        // Now LFO should be at full amplitude
        std::vector<float> samples;
        for (int i = 0; i < 48000; ++i) {
            samples.push_back(lfo.process());
        }

        float maxVal = 0.0f;
        for (float s : samples) {
            maxVal = std::max(maxVal, std::abs(s));
        }

        // Should reach close to ±1.0
        REQUIRE_THAT(maxVal, WithinAbs(1.0f, 0.1f));
    }

    SECTION("LFO with zero delay starts immediately") {
        lfo.reset();  // Ensure clean starting state
        lfo.setDelay(0.0f);
        lfo.trigger();

        // Process enough samples to see meaningful amplitude
        // At 1Hz, need at least 6000 samples to reach quarter cycle (value ~0.5)
        std::vector<float> samples;
        for (int i = 0; i < 6000; ++i) {
            samples.push_back(lfo.process());
        }

        // Should have normal amplitude from the start
        float maxVal = 0.0f;
        for (float s : samples) {
            maxVal = std::max(maxVal, std::abs(s));
        }

        REQUIRE(maxVal > 0.1f);
    }

    SECTION("Multiple trigger calls reset delay timer") {
        lfo.setDelay(0.1f);
        lfo.trigger();

        // Process partway through delay
        for (int i = 0; i < 1000; ++i) {
            lfo.process();
        }

        // Trigger again - should reset
        lfo.trigger();
        float value = lfo.process();

        // Should be back to zero
        REQUIRE_THAT(std::abs(value), WithinAbs(0.0f, 0.1f));
    }
}

TEST_CASE("LFO different delay times", "[lfo][m12]") {
    Lfo lfo;
    const float sampleRate = 48000.0f;
    lfo.setSampleRate(sampleRate);
    lfo.setRate(1.0f);

    SECTION("Longer delay takes longer to reach full amplitude") {
        // Test with 0.5s delay
        lfo.setDelay(0.5f);
        lfo.trigger();

        // After 0.25s (12000 samples), should still be ramping up
        for (int i = 0; i < 12000; ++i) {
            lfo.process();
        }

        // Sample for 0.1s to stay within mid-delay period
        // This is while delayScale ramps from ~0.5 to ~0.7
        std::vector<float> samples;
        for (int i = 0; i < 4800; ++i) {
            samples.push_back(lfo.process());
        }

        float midDelayMax = 0.0f;
        for (float s : samples) {
            midDelayMax = std::max(midDelayMax, std::abs(s));
        }

        // Should not yet be at full amplitude (delay scale ~0.5-0.7)
        REQUIRE(midDelayMax < 0.75f);

        // Reset and test after full delay period
        lfo.setDelay(0.5f);
        lfo.trigger();

        // Skip past the full delay period (0.5s = 24000 samples)
        for (int i = 0; i < 24000; ++i) {
            lfo.process();
        }

        // Sample over one full LFO cycle to capture true amplitude
        // At 1Hz, one cycle = 48000 samples
        samples.clear();
        for (int i = 0; i < 48000; ++i) {
            samples.push_back(lfo.process());
        }

        float postDelayMax = 0.0f;
        for (float s : samples) {
            postDelayMax = std::max(postDelayMax, std::abs(s));
        }

        // Should be at higher amplitude now (delay scale = 1.0)
        REQUIRE(postDelayMax > midDelayMax);
        REQUIRE(postDelayMax > 0.85f);  // Should be close to full amplitude
    }

    SECTION("Maximum delay of 3 seconds works") {
        lfo.setDelay(3.0f);
        lfo.trigger();

        // After 1 second (48000 samples), should still be ramping
        for (int i = 0; i < 48000; ++i) {
            lfo.process();
        }

        float value = lfo.process();
        float early = std::abs(value);

        // After 3 seconds (144000 samples total)
        for (int i = 0; i < 96000; ++i) {
            lfo.process();
        }

        value = lfo.process();
        float late = std::abs(value);

        // Later values should be larger (or at least not smaller)
        REQUIRE(late >= early * 0.9f);  // Allow some tolerance
    }
}

TEST_CASE("LFO reset behavior", "[lfo]") {
    Lfo lfo;
    lfo.setSampleRate(48000.0f);
    lfo.setRate(2.0f);
    lfo.setDelay(0.0f);

    SECTION("Reset returns LFO to initial phase") {
        // Process some samples
        for (int i = 0; i < 1000; ++i) {
            lfo.process();
        }

        // Reset
        lfo.reset();
        float value = lfo.process();

        // Should be back at start of triangle wave (near 0)
        REQUIRE_THAT(value, WithinAbs(0.0f, 0.01f));
    }

    SECTION("Reset clears delay timer") {
        lfo.setDelay(0.5f);
        lfo.trigger();

        // Process partway through delay
        for (int i = 0; i < 5000; ++i) {
            lfo.process();
        }

        // Reset should clear both phase and delay
        lfo.reset();
        lfo.trigger();

        float value = lfo.process();
        REQUIRE_THAT(std::abs(value), WithinAbs(0.0f, 0.01f));
    }
}
