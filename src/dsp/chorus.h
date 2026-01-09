#pragma once

#include "types.h"
#include <cmath>

namespace phj {

/**
 * Chorus - BBD (Bucket Brigade Device) Stereo Chorus
 *
 * Emulates the Juno-106's famous dual BBD chorus circuit.
 * The Juno-106 uses two MN3009 BBD chips with different modulation
 * characteristics to create a rich stereo chorus effect.
 *
 * Features:
 * - Two independent BBD stages (Chorus I and Chorus II)
 * - Stereo output with different modulation per channel
 * - Three modes: I, II, and I+II (both)
 *
 * M8: BBD Chorus Implementation
 */
class Chorus {
public:
    Chorus();

    void setSampleRate(float sampleRate);
    void reset();

    // Process stereo (input is mono, outputs are stereo)
    void process(Sample input, Sample& leftOut, Sample& rightOut);

    // Chorus modes (matching Juno-106)
    enum Mode {
        OFF = 0,     // No chorus
        MODE_I = 1,  // First BBD circuit
        MODE_II = 2, // Second BBD circuit
        MODE_BOTH = 3 // Both circuits (I+II)
    };

    void setMode(Mode mode);
    Mode getMode() const { return mode_; }

private:
    float sampleRate_;
    Mode mode_;

    // BBD delay lines for each stage
    // Maximum delay: ~10ms at 48kHz = 480 samples
    static constexpr int MAX_DELAY_SAMPLES = 512;
    Sample delayBuffer1_[MAX_DELAY_SAMPLES];
    Sample delayBuffer2_[MAX_DELAY_SAMPLES];
    int delayWritePos_;

    // LFO state for each BBD stage
    float lfo1Phase_;
    float lfo2Phase_;

    // BBD parameters (matched to Juno-106 characteristics)
    // Chorus I: shorter delay, faster modulation
    static constexpr float CHORUS_I_DELAY_MS = 2.5f;
    static constexpr float CHORUS_I_DEPTH_MS = 0.5f;
    static constexpr float CHORUS_I_RATE_HZ = 0.65f;

    // Chorus II: longer delay, slower modulation
    static constexpr float CHORUS_II_DELAY_MS = 4.0f;
    static constexpr float CHORUS_II_DEPTH_MS = 0.8f;
    static constexpr float CHORUS_II_RATE_HZ = 0.50f;

    // Helper functions
    Sample readDelayLine(const Sample* buffer, float delaySamples) const;
    float getLfoValue(float phase) const;
};

} // namespace phj
