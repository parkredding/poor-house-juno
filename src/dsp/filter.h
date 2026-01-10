#pragma once

#include "types.h"
#include "parameters.h"

namespace phj {

/**
 * IR3109 4-Pole Ladder Filter Emulation
 *
 * Uses Zero-Delay Feedback (ZDF) topology for accurate resonance
 * and self-oscillation behavior.
 *
 * Features:
 * - 24dB/octave lowpass characteristic
 * - Self-oscillation at high resonance
 * - Envelope modulation (bipolar)
 * - LFO modulation
 * - Key tracking (0%, 50%, 100%)
 * - Subtle saturation for IR3109 character
 */
class Filter {
public:
    Filter();

    void setSampleRate(float sampleRate);
    void setParameters(const FilterParams& params);
    void setEnvValue(float envValue);   // 0.0 - 1.0
    void setLfoValue(float lfoValue);   // -1.0 - 1.0
    void setNoteFrequency(float noteFreq);  // For key tracking
    void setVelocityValue(float velocity, float amount);  // M14: Velocity modulation

    void reset();

    // Process single sample
    Sample process(Sample input);

    // Process buffer
    void process(const Sample* input, Sample* output, int numSamples);

private:
    float sampleRate_;
    FilterParams params_;

    // Modulation sources
    float envValue_;
    float lfoValue_;
    float noteFrequency_;
    float velocityValue_;      // M14: Velocity value (0.0 - 1.0)
    float velocityAmount_;     // M14: Velocity modulation amount (0.0 - 1.0)

    // Filter state (4 stages for 4-pole)
    float stage1_;
    float stage2_;
    float stage3_;
    float stage4_;

    // ZDF coefficients
    float g_;           // Cutoff coefficient
    float k_;           // Resonance coefficient

    // M11: HPF state (1-pole high-pass filter)
    float hpfState_;
    float hpfG_;        // HPF cutoff coefficient

    // M15: Performance optimization - cache modulation values
    float cachedEnvValue_;
    float cachedLfoValue_;
    float cachedVelocityValue_;
    bool coefficientsNeedUpdate_;

    // Helper methods
    void updateCoefficients();
    void updateCoefficientsIfNeeded();  // M15: Conditional update
    float calculateCutoffHz();
    float saturate(float x);  // Soft saturation
    float processHPF(float input);  // M11: High-pass filter processing
};

} // namespace phj
