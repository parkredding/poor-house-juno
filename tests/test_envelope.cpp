/**
 * Unit tests for ADSR Envelope
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "envelope.h"

using namespace phj;
using Catch::Matchers::WithinAbs;

TEST_CASE("Envelope ADSR stages", "[envelope]") {
    Envelope env;
    const float sampleRate = 48000.0f;
    env.setSampleRate(sampleRate);

    EnvelopeParams params;
    params.attack = 0.01f;   // 10ms
    params.decay = 0.1f;     // 100ms
    params.sustain = 0.7f;   // 70%
    params.release = 0.05f;  // 50ms
    env.setParameters(params);

    SECTION("Envelope starts in IDLE state") {
        REQUIRE(env.getStage() == Envelope::IDLE);
        REQUIRE_FALSE(env.isActive());

        float value = env.process();
        REQUIRE_THAT(value, WithinAbs(0.0f, 0.001f));
    }

    SECTION("Note on triggers ATTACK stage") {
        env.noteOn();
        REQUIRE(env.getStage() == Envelope::ATTACK);
        REQUIRE(env.isActive());

        // Value should start increasing
        float prev = 0.0f;
        bool increasing = true;
        for (int i = 0; i < 100; ++i) {
            float value = env.process();
            if (value <= prev) {
                increasing = false;
                break;
            }
            prev = value;
        }
        REQUIRE(increasing);
    }

    SECTION("Envelope reaches peak and enters DECAY") {
        env.noteOn();

        // Process through attack phase (10ms = 480 samples at 48kHz)
        for (int i = 0; i < 1000; ++i) {
            env.process();
        }

        // Should be in DECAY or SUSTAIN stage now
        auto stage = env.getStage();
        REQUIRE((stage == Envelope::DECAY || stage == Envelope::SUSTAIN));
    }

    SECTION("Envelope settles at SUSTAIN level") {
        env.noteOn();

        // Process long enough to reach sustain (10ms attack + 100ms decay = ~5500 samples)
        for (int i = 0; i < 10000; ++i) {
            env.process();
        }

        REQUIRE(env.getStage() == Envelope::SUSTAIN);

        // Value should be close to sustain level (0.7)
        float value = env.process();
        REQUIRE_THAT(value, WithinAbs(params.sustain, 0.1f));
    }

    SECTION("Note off triggers RELEASE stage") {
        env.noteOn();

        // Reach sustain
        for (int i = 0; i < 10000; ++i) {
            env.process();
        }

        env.noteOff();
        REQUIRE(env.getStage() == Envelope::RELEASE);

        // Value should start decreasing
        float prev = env.process();
        bool decreasing = false;
        for (int i = 0; i < 100; ++i) {
            float value = env.process();
            if (value < prev) {
                decreasing = true;
                break;
            }
            prev = value;
        }
        REQUIRE(decreasing);
    }

    SECTION("Envelope returns to IDLE after release") {
        env.noteOn();

        // Reach sustain
        for (int i = 0; i < 10000; ++i) {
            env.process();
        }

        env.noteOff();

        // Process through release (50ms = 2400 samples at 48kHz)
        for (int i = 0; i < 5000; ++i) {
            env.process();
        }

        REQUIRE(env.getStage() == Envelope::IDLE);
        REQUIRE_FALSE(env.isActive());

        float value = env.process();
        REQUIRE_THAT(value, WithinAbs(0.0f, 0.01f));
    }

    SECTION("Reset returns envelope to initial state") {
        env.noteOn();

        // Process some samples
        for (int i = 0; i < 100; ++i) {
            env.process();
        }

        env.reset();
        REQUIRE(env.getStage() == Envelope::IDLE);
        REQUIRE_FALSE(env.isActive());

        float value = env.process();
        REQUIRE_THAT(value, WithinAbs(0.0f, 0.001f));
    }

    SECTION("Note off during attack goes directly to release") {
        env.noteOn();

        // Process part of attack
        for (int i = 0; i < 100; ++i) {
            env.process();
        }

        REQUIRE(env.getStage() == Envelope::ATTACK);

        env.noteOff();
        REQUIRE(env.getStage() == Envelope::RELEASE);
    }

    SECTION("Multiple note on/off cycles work correctly") {
        for (int cycle = 0; cycle < 3; ++cycle) {
            env.noteOn();
            REQUIRE(env.isActive());

            // Process for a bit
            for (int i = 0; i < 500; ++i) {
                env.process();
            }

            env.noteOff();

            // Process through release
            for (int i = 0; i < 5000; ++i) {
                env.process();
            }

            REQUIRE(env.getStage() == Envelope::IDLE);
        }
    }
}

TEST_CASE("Envelope timing accuracy", "[envelope]") {
    Envelope env;
    const float sampleRate = 48000.0f;
    env.setSampleRate(sampleRate);

    SECTION("Attack time is approximately correct") {
        EnvelopeParams params;
        params.attack = 0.1f;  // 100ms
        params.decay = 1.0f;
        params.sustain = 1.0f;
        params.release = 0.1f;
        env.setParameters(params);

        env.noteOn();

        // Process and measure when we reach ~99% of peak
        int samplesUntilPeak = 0;
        for (int i = 0; i < 10000; ++i) {
            float value = env.process();
            samplesUntilPeak++;
            if (value > 0.99f) {
                break;
            }
        }

        // Should take approximately 100ms = 4800 samples at 48kHz
        // Allow 20% tolerance for exponential curves
        float timeMs = (samplesUntilPeak / sampleRate) * 1000.0f;
        REQUIRE_THAT(timeMs, WithinAbs(100.0f, 30.0f));
    }

    SECTION("Release time is approximately correct") {
        EnvelopeParams params;
        params.attack = 0.01f;
        params.decay = 0.01f;
        params.sustain = 1.0f;
        params.release = 0.1f;  // 100ms
        env.setParameters(params);

        env.noteOn();

        // Reach sustain
        for (int i = 0; i < 5000; ++i) {
            env.process();
        }

        env.noteOff();
        float startValue = env.process();

        // Process and measure when we reach ~1% of start value
        int samplesUntilQuiet = 0;
        for (int i = 0; i < 10000; ++i) {
            float value = env.process();
            samplesUntilQuiet++;
            if (value < startValue * 0.01f) {
                break;
            }
        }

        // Should take approximately 100ms = 4800 samples
        float timeMs = (samplesUntilQuiet / sampleRate) * 1000.0f;
        REQUIRE_THAT(timeMs, WithinAbs(100.0f, 30.0f));
    }
}

TEST_CASE("Envelope parameter changes", "[envelope]") {
    Envelope env;
    env.setSampleRate(48000.0f);

    SECTION("Changing parameters during playback works") {
        EnvelopeParams params;
        params.attack = 0.01f;
        params.decay = 0.1f;
        params.sustain = 0.5f;
        params.release = 0.05f;
        env.setParameters(params);

        env.noteOn();

        // Process partway through
        for (int i = 0; i < 1000; ++i) {
            env.process();
        }

        // Change sustain level
        params.sustain = 0.8f;
        env.setParameters(params);

        // Process to sustain
        for (int i = 0; i < 10000; ++i) {
            env.process();
        }

        // Should settle at new sustain level
        float value = env.process();
        REQUIRE_THAT(value, WithinAbs(0.8f, 0.15f));
    }
}
