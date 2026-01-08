#pragma once

#include "types.h"

namespace phj {

/**
 * Simple sine oscillator for initial testing.
 * This will be replaced with the full DCO implementation later.
 */
class SineOscillator {
public:
    SineOscillator();

    void setSampleRate(float sampleRate);
    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void reset();

    // Process a single sample
    Sample process();

    // Process a buffer
    void process(Sample* output, int numSamples);

private:
    float sampleRate_;
    float frequency_;
    float amplitude_;
    float phase_;
    float phaseIncrement_;

    void updatePhaseIncrement();
};

} // namespace phj
