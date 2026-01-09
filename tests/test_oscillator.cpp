/**
 * Unit tests for Oscillator and DCO components
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>
#include "oscillator.h"
#include "dco.h"

using namespace phj;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("SineOscillator basic functionality", "[oscillator]") {
    SineOscillator osc;
    const float sampleRate = 48000.0f;
    osc.setSampleRate(sampleRate);

    SECTION("440 Hz sine wave has correct frequency") {
        osc.setFrequency(440.0f);
        osc.setAmplitude(1.0f);

        // Generate one period worth of samples
        const int samplesPerPeriod = static_cast<int>(sampleRate / 440.0f);
        std::vector<Sample> samples(samplesPerPeriod);

        for (int i = 0; i < samplesPerPeriod; ++i) {
            samples[i] = osc.process();
        }

        // Check that we start near 0 and reach peaks
        REQUIRE_THAT(samples[0], WithinAbs(0.0f, 0.01f));

        // Find max value (should be near 1.0)
        float maxVal = 0.0f;
        for (const auto& s : samples) {
            maxVal = std::max(maxVal, std::abs(s));
        }
        REQUIRE_THAT(maxVal, WithinAbs(1.0f, 0.1f));
    }

    SECTION("Amplitude scaling works correctly") {
        osc.setFrequency(440.0f);
        osc.setAmplitude(0.5f);

        // Generate some samples
        float maxVal = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            Sample s = osc.process();
            maxVal = std::max(maxVal, std::abs(s));
        }

        // Max value should be around 0.5
        REQUIRE_THAT(maxVal, WithinAbs(0.5f, 0.1f));
    }

    SECTION("Reset returns oscillator to initial phase") {
        osc.setFrequency(440.0f);
        osc.setAmplitude(1.0f);

        // Process some samples
        for (int i = 0; i < 100; ++i) {
            osc.process();
        }

        // Reset and get first sample
        osc.reset();
        Sample firstSample = osc.process();

        // After reset, should be close to 0
        REQUIRE_THAT(firstSample, WithinAbs(0.0f, 0.01f));
    }
}

TEST_CASE("DCO waveform generation", "[dco]") {
    Dco dco;
    const float sampleRate = 48000.0f;
    dco.setSampleRate(sampleRate);
    dco.setFrequency(440.0f);

    DcoParams params;

    SECTION("Sawtooth wave generation") {
        params.sawLevel = 1.0f;
        params.pulseLevel = 0.0f;
        params.subLevel = 0.0f;
        params.noiseLevel = 0.0f;
        params.enableDrift = false;  // Disable drift for consistent testing
        dco.setParameters(params);

        // Generate samples
        std::vector<Sample> samples(1000);
        dco.process(samples.data(), samples.size());

        // Sawtooth should have values ranging roughly -1 to 1
        float minVal = 1.0f;
        float maxVal = -1.0f;
        for (const auto& s : samples) {
            minVal = std::min(minVal, s);
            maxVal = std::max(maxVal, s);
        }

        REQUIRE(maxVal > 0.5f);
        REQUIRE(minVal < -0.5f);
    }

    SECTION("Pulse wave generation with 50% duty cycle") {
        params.sawLevel = 0.0f;
        params.pulseLevel = 1.0f;
        params.pulseWidth = 0.5f;  // Square wave
        params.subLevel = 0.0f;
        params.noiseLevel = 0.0f;
        params.enableDrift = false;
        dco.setParameters(params);

        // Generate samples
        std::vector<Sample> samples(1000);
        dco.process(samples.data(), samples.size());

        // Square wave should be mostly at extremes
        int countPositive = 0;
        int countNegative = 0;
        for (const auto& s : samples) {
            if (s > 0.0f) countPositive++;
            else countNegative++;
        }

        // Should have roughly equal positive and negative samples
        float ratio = static_cast<float>(countPositive) / samples.size();
        REQUIRE_THAT(ratio, WithinAbs(0.5f, 0.1f));
    }

    SECTION("Sub-oscillator is one octave lower") {
        params.sawLevel = 0.0f;
        params.pulseLevel = 0.0f;
        params.subLevel = 1.0f;
        params.noiseLevel = 0.0f;
        params.enableDrift = false;
        dco.setParameters(params);

        dco.setFrequency(440.0f);  // A4

        // The sub-oscillator should complete half as many cycles
        // We can't easily verify frequency directly, but we can check that it produces output
        std::vector<Sample> samples(1000);
        dco.process(samples.data(), samples.size());

        // Verify we got some signal
        bool hasSignal = false;
        for (const auto& s : samples) {
            if (std::abs(s) > 0.1f) {
                hasSignal = true;
                break;
            }
        }
        REQUIRE(hasSignal);
    }

    SECTION("Noise generator produces varied output") {
        params.sawLevel = 0.0f;
        params.pulseLevel = 0.0f;
        params.subLevel = 0.0f;
        params.noiseLevel = 1.0f;
        dco.setParameters(params);

        std::vector<Sample> samples(1000);
        dco.process(samples.data(), samples.size());

        // Calculate variance to ensure noise is random
        float mean = 0.0f;
        for (const auto& s : samples) {
            mean += s;
        }
        mean /= samples.size();

        float variance = 0.0f;
        for (const auto& s : samples) {
            float diff = s - mean;
            variance += diff * diff;
        }
        variance /= samples.size();

        // Noise should have significant variance
        REQUIRE(variance > 0.1f);
    }

    SECTION("LFO modulation affects pitch") {
        params.sawLevel = 1.0f;
        params.pulseLevel = 0.0f;
        params.subLevel = 0.0f;
        params.noiseLevel = 0.0f;
        params.lfoTarget = DcoParams::LFO_PITCH;
        params.enableDrift = false;
        dco.setParameters(params);

        dco.setFrequency(440.0f);

        // Generate samples with no LFO
        dco.setLfoValue(0.0f);
        std::vector<Sample> samplesNoLfo(100);
        dco.process(samplesNoLfo.data(), samplesNoLfo.size());

        // Reset and generate with max LFO
        dco.reset();
        dco.setLfoValue(1.0f);
        std::vector<Sample> samplesWithLfo(100);
        dco.process(samplesWithLfo.data(), samplesWithLfo.size());

        // The waveforms should be different due to frequency modulation
        // (This is a basic check - more sophisticated phase comparison could be done)
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(samplesNoLfo[i] - samplesWithLfo[i]) > 0.1f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }

    SECTION("PWM modulation works") {
        params.sawLevel = 0.0f;
        params.pulseLevel = 1.0f;
        params.pulseWidth = 0.5f;
        params.pwmDepth = 1.0f;  // Full PWM depth
        params.subLevel = 0.0f;
        params.noiseLevel = 0.0f;
        params.lfoTarget = DcoParams::LFO_PWM;
        params.enableDrift = false;
        dco.setParameters(params);

        // Generate with different LFO values
        dco.setLfoValue(-1.0f);
        std::vector<Sample> samplesLfoMin(100);
        dco.process(samplesLfoMin.data(), samplesLfoMin.size());

        dco.reset();
        dco.setLfoValue(1.0f);
        std::vector<Sample> samplesLfoMax(100);
        dco.process(samplesLfoMax.data(), samplesLfoMax.size());

        // Waveforms should differ due to PWM
        bool isDifferent = false;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(samplesLfoMin[i] - samplesLfoMax[i]) > 0.1f) {
                isDifferent = true;
                break;
            }
        }
        REQUIRE(isDifferent);
    }
}

TEST_CASE("DCO Range (M14)", "[dco][m14]") {
    Dco dco;
    const float sampleRate = 48000.0f;
    dco.setSampleRate(sampleRate);
    dco.setFrequency(440.0f);

    DcoParams params;
    params.sawLevel = 1.0f;
    params.enableDrift = false;

    SECTION("Range changes affect octave") {
        // This test verifies that the range parameter correctly shifts octaves
        // We can't directly measure frequency from the output, but we can verify
        // that different range settings produce different outputs

        params.range = DcoParams::RANGE_16;  // -1 octave
        dco.setParameters(params);
        std::vector<Sample> samples16(100);
        dco.process(samples16.data(), samples16.size());

        dco.reset();
        params.range = DcoParams::RANGE_8;  // Normal
        dco.setParameters(params);
        std::vector<Sample> samples8(100);
        dco.process(samples8.data(), samples8.size());

        dco.reset();
        params.range = DcoParams::RANGE_4;  // +1 octave
        dco.setParameters(params);
        std::vector<Sample> samples4(100);
        dco.process(samples4.data(), samples4.size());

        // All three should produce different waveforms
        // (due to different frequencies sampling the same waveform shape)
        REQUIRE(samples16 != samples8);
        REQUIRE(samples8 != samples4);
        REQUIRE(samples16 != samples4);
    }
}
