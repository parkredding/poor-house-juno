/**
 * Unit tests for Voice (integrated DCO, Filter, Envelopes)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>
#include "voice.h"

using namespace phj;
using Catch::Matchers::WithinAbs;

TEST_CASE("Voice basic functionality", "[voice]") {
    Voice voice;
    const float sampleRate = 48000.0f;
    voice.setSampleRate(sampleRate);

    // Set up basic parameters
    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;
    filterParams.resonance = 0.0f;

    EnvelopeParams filterEnvParams;
    filterEnvParams.attack = 0.01f;
    filterEnvParams.decay = 0.1f;
    filterEnvParams.sustain = 0.7f;
    filterEnvParams.release = 0.05f;

    EnvelopeParams ampEnvParams;
    ampEnvParams.attack = 0.01f;
    ampEnvParams.decay = 0.1f;
    ampEnvParams.sustain = 0.7f;
    ampEnvParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, filterEnvParams, ampEnvParams);

    SECTION("Voice starts inactive") {
        REQUIRE_FALSE(voice.isActive());
        REQUIRE(voice.getCurrentNote() == -1);
    }

    SECTION("Note on activates voice") {
        voice.noteOn(60, 1.0f);  // Middle C

        REQUIRE(voice.isActive());
        REQUIRE(voice.getCurrentNote() == 60);
    }

    SECTION("Voice produces sound after note on") {
        voice.noteOn(60, 1.0f);

        // Generate some samples
        std::vector<Sample> samples(1000);
        voice.process(samples.data(), 1000);

        // Should have some non-zero samples
        bool hasSound = false;
        for (Sample s : samples) {
            if (std::abs(s) > 0.01f) {
                hasSound = true;
                break;
            }
        }
        REQUIRE(hasSound);
    }

    SECTION("Note off starts release") {
        voice.noteOn(60, 1.0f);

        // Process to sustain
        for (int i = 0; i < 10000; ++i) {
            voice.process();
        }

        voice.noteOff();
        REQUIRE(voice.isReleasing());
    }

    SECTION("Voice becomes inactive after release") {
        voice.noteOn(60, 1.0f);

        // Process to sustain
        for (int i = 0; i < 10000; ++i) {
            voice.process();
        }

        voice.noteOff();

        // Process through release
        for (int i = 0; i < 10000; ++i) {
            voice.process();
        }

        // Should be inactive now
        REQUIRE_FALSE(voice.isActive());
    }

    SECTION("Reset clears voice state") {
        voice.noteOn(60, 1.0f);

        // Process some samples
        for (int i = 0; i < 100; ++i) {
            voice.process();
        }

        voice.reset();
        REQUIRE_FALSE(voice.isActive());
        REQUIRE(voice.getCurrentNote() == -1);
    }

    SECTION("Different notes produce different frequencies") {
        // Note A4 (440 Hz)
        voice.noteOn(69, 1.0f);
        std::vector<Sample> samplesA4(1000);
        voice.process(samplesA4.data(), 1000);

        voice.reset();

        // Note A5 (880 Hz)
        voice.noteOn(81, 1.0f);
        std::vector<Sample> samplesA5(1000);
        voice.process(samplesA5.data(), 1000);

        // The waveforms should be different due to different frequencies
        bool isDifferent = false;
        for (size_t i = 0; i < 1000; ++i) {
            if (std::abs(samplesA4[i] - samplesA5[i]) > 0.1f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }
}

TEST_CASE("Voice velocity sensitivity", "[voice]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.5f;
    filterParams.resonance = 0.0f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;  // Fast attack
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("Different velocities produce different amplitudes") {
        // Note with low velocity
        voice.noteOn(60, 0.3f);
        std::vector<Sample> samplesLow(1000);
        voice.process(samplesLow.data(), 1000);

        voice.reset();

        // Note with high velocity
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesHigh(1000);
        voice.process(samplesHigh.data(), 1000);

        // Calculate RMS for both
        float rmsLow = 0.0f;
        float rmsHigh = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {  // Skip attack transient
            rmsLow += samplesLow[i] * samplesLow[i];
            rmsHigh += samplesHigh[i] * samplesHigh[i];
        }
        rmsLow = std::sqrt(rmsLow / 900.0f);
        rmsHigh = std::sqrt(rmsHigh / 900.0f);

        // High velocity should be louder
        REQUIRE(rmsHigh > rmsLow);
    }

    SECTION("Velocity sensitivity controls can be adjusted (M14)") {
        // Set velocity sensitivity
        voice.setVelocitySensitivity(1.0f, 1.0f);  // Full sensitivity

        voice.noteOn(60, 0.5f);
        std::vector<Sample> samples(1000);
        voice.process(samples.data(), 1000);

        // Just verify it doesn't crash and produces output
        bool hasOutput = false;
        for (Sample s : samples) {
            if (std::abs(s) > 0.01f) {
                hasOutput = true;
                break;
            }
        }
        REQUIRE(hasOutput);
    }
}

TEST_CASE("Voice portamento (M11)", "[voice][m11]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("Portamento OFF produces instant pitch change") {
        voice.setPortamentoTime(0.0f);  // No glide

        voice.noteOn(60, 1.0f);  // C4
        std::vector<Sample> samples1(100);
        voice.process(samples1.data(), 100);

        voice.noteOn(72, 1.0f);  // C5 (octave up)
        std::vector<Sample> samples2(100);
        voice.process(samples2.data(), 100);

        // Pitch should change immediately
        // We can't directly measure frequency, but the waveforms should be different
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(samples1[i] - samples2[i]) > 0.1f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Portamento ON produces gradual pitch change") {
        voice.setPortamentoTime(0.5f);  // 500ms glide

        voice.noteOn(60, 1.0f);  // C4

        // Process to stable state
        for (int i = 0; i < 1000; ++i) {
            voice.process();
        }

        // Trigger new note
        voice.noteOn(72, 1.0f);  // C5

        // Process a few samples - pitch should still be changing
        std::vector<Sample> earlyGlide(100);
        voice.process(earlyGlide.data(), 100);

        // Process more samples
        for (int i = 0; i < 5000; ++i) {
            voice.process();
        }

        std::vector<Sample> midGlide(100);
        voice.process(midGlide.data(), 100);

        // The two sample sets should be different (pitch is gliding)
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(earlyGlide[i] - midGlide[i]) > 0.05f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Portamento eventually reaches target note") {
        voice.setPortamentoTime(0.1f);  // 100ms glide

        voice.noteOn(60, 1.0f);

        // Stabilize
        for (int i = 0; i < 5000; ++i) {
            voice.process();
        }

        std::vector<Sample> beforeGlide(100);
        voice.process(beforeGlide.data(), 100);

        // New note
        voice.noteOn(62, 1.0f);  // D4 (2 semitones up)

        // Process through entire glide time (100ms = 4800 samples + buffer)
        for (int i = 0; i < 10000; ++i) {
            voice.process();
        }

        // Should now be stable at new pitch
        std::vector<Sample> afterGlide(100);
        voice.process(afterGlide.data(), 100);

        // Verify we got some output and it's different from before
        bool hasOutput = false;
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(afterGlide[i]) > 0.01f) {
                hasOutput = true;
            }
            if (std::abs(beforeGlide[i] - afterGlide[i]) > 0.1f) {
                isDifferent = true;
            }
        }
        REQUIRE(hasOutput);
        REQUIRE(isDifferent);
    }
}

TEST_CASE("Voice pitch bend (M11)", "[voice][m11]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("Pitch bend affects pitch") {
        voice.noteOn(60, 1.0f);

        // No pitch bend
        voice.setPitchBend(0.0f, 2.0f);  // Center, ±2 semitones
        std::vector<Sample> samplesCenter(1000);
        voice.process(samplesCenter.data(), 1000);

        voice.reset();
        voice.noteOn(60, 1.0f);

        // Full up pitch bend
        voice.setPitchBend(1.0f, 2.0f);  // +2 semitones
        std::vector<Sample> samplesUp(1000);
        voice.process(samplesUp.data(), 1000);

        // Should be different
        bool isDifferent = false;
        for (size_t i = 0; i < 1000; ++i) {
            if (std::abs(samplesCenter[i] - samplesUp[i]) > 0.05f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Pitch bend range is configurable") {
        voice.noteOn(60, 1.0f);

        // Small range
        voice.setPitchBend(1.0f, 1.0f);  // +1 semitone
        std::vector<Sample> samplesSmall(100);
        voice.process(samplesSmall.data(), 100);

        voice.reset();
        voice.noteOn(60, 1.0f);

        // Large range
        voice.setPitchBend(1.0f, 12.0f);  // +12 semitones (octave)
        std::vector<Sample> samplesLarge(100);
        voice.process(samplesLarge.data(), 100);

        // Should be different
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(samplesSmall[i] - samplesLarge[i]) > 0.1f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }
}

TEST_CASE("Voice VCA mode (M13)", "[voice][m13]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.05f;  // 50ms attack
    envParams.decay = 0.1f;
    envParams.sustain = 0.7f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("ENV mode uses envelope") {
        voice.setVcaMode(0);  // ENV mode

        voice.noteOn(60, 1.0f);

        // Early samples should be quiet (attack phase)
        std::vector<Sample> earlySamples(100);
        voice.process(earlySamples.data(), 100);

        float earlyMax = 0.0f;
        for (Sample s : earlySamples) {
            earlyMax = std::max(earlyMax, std::abs(s));
        }

        // Process more (should be louder)
        for (int i = 0; i < 5000; ++i) {
            voice.process();
        }

        std::vector<Sample> lateSamples(100);
        voice.process(lateSamples.data(), 100);

        float lateMax = 0.0f;
        for (Sample s : lateSamples) {
            lateMax = std::max(lateMax, std::abs(s));
        }

        // Later samples should be louder (envelope has risen)
        REQUIRE(lateMax > earlyMax);
    }

    SECTION("GATE mode provides instant on/off") {
        voice.setVcaMode(1);  // GATE mode

        voice.noteOn(60, 1.0f);

        // Should be loud immediately (no attack)
        std::vector<Sample> immediateSamples(100);
        voice.process(immediateSamples.data(), 100);

        float immediateMax = 0.0f;
        for (size_t i = 10; i < 100; ++i) {  // Skip first few samples
            immediateMax = std::max(immediateMax, std::abs(immediateSamples[i]));
        }

        // Should have reasonable amplitude immediately
        REQUIRE(immediateMax > 0.1f);
    }
}

TEST_CASE("Voice VCA level (M14)", "[voice][m14]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("VCA level controls output volume") {
        // Full level
        voice.setVcaLevel(1.0f);
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesFull(1000);
        voice.process(samplesFull.data(), 1000);

        voice.reset();

        // Half level
        voice.setVcaLevel(0.5f);
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesHalf(1000);
        voice.process(samplesHalf.data(), 1000);

        // Calculate RMS
        float rmsFull = 0.0f;
        float rmsHalf = 0.0f;
        for (size_t i = 100; i < 1000; ++i) {
            rmsFull += samplesFull[i] * samplesFull[i];
            rmsHalf += samplesHalf[i] * samplesHalf[i];
        }
        rmsFull = std::sqrt(rmsFull / 900.0f);
        rmsHalf = std::sqrt(rmsHalf / 900.0f);

        // Half level should be quieter
        REQUIRE(rmsHalf < rmsFull);
        REQUIRE_THAT(rmsHalf, WithinAbs(rmsFull * 0.5f, 0.2f));
    }
}

TEST_CASE("Voice master tune (M14)", "[voice][m14]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("Master tune shifts pitch") {
        // No tune
        voice.setMasterTune(0.0f);
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesNoTune(1000);
        voice.process(samplesNoTune.data(), 1000);

        voice.reset();

        // +50 cents (half semitone)
        voice.setMasterTune(50.0f);
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesUpTune(1000);
        voice.process(samplesUpTune.data(), 1000);

        // Should be different
        bool isDifferent = false;
        for (size_t i = 0; i < 1000; ++i) {
            if (std::abs(samplesNoTune[i] - samplesUpTune[i]) > 0.05f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("Master tune range is ±50 cents") {
        // Test with negative tune
        voice.setMasterTune(-50.0f);
        voice.noteOn(60, 1.0f);
        std::vector<Sample> samplesDownTune(100);
        voice.process(samplesDownTune.data(), 100);

        // Should produce output
        bool hasOutput = false;
        for (Sample s : samplesDownTune) {
            if (std::abs(s) > 0.01f) {
                hasOutput = true;
                break;
            }
        }
        REQUIRE(hasOutput);
    }
}

TEST_CASE("Voice LFO modulation", "[voice]") {
    Voice voice;
    voice.setSampleRate(48000.0f);

    DcoParams dcoParams;
    dcoParams.sawLevel = 1.0f;
    dcoParams.lfoTarget = DcoParams::LFO_PITCH;
    dcoParams.enableDrift = false;

    FilterParams filterParams;
    filterParams.cutoff = 0.8f;

    EnvelopeParams envParams;
    envParams.attack = 0.001f;
    envParams.decay = 0.1f;
    envParams.sustain = 1.0f;
    envParams.release = 0.05f;

    voice.setParameters(dcoParams, filterParams, envParams, envParams);

    SECTION("LFO affects voice output") {
        voice.noteOn(60, 1.0f);

        // No LFO
        voice.setLfoValue(0.0f);
        std::vector<Sample> samplesNoLfo(1000);
        voice.process(samplesNoLfo.data(), 1000);

        voice.reset();
        voice.noteOn(60, 1.0f);

        // Full LFO
        voice.setLfoValue(1.0f);
        std::vector<Sample> samplesWithLfo(1000);
        voice.process(samplesWithLfo.data(), 1000);

        // Should be different
        bool isDifferent = false;
        for (size_t i = 0; i < 1000; ++i) {
            if (std::abs(samplesNoLfo[i] - samplesWithLfo[i]) > 0.05f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }
}
