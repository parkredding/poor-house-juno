#pragma once

#include "types.h"

namespace phj {

/**
 * Triangle LFO (Low-Frequency Oscillator)
 *
 * Generates a triangle wave for modulation purposes.
 * Based on the Juno-106 LFO which uses a triangle waveform.
 */
class Lfo {
public:
    Lfo();

    void setSampleRate(float sampleRate);
    void setRate(float rateHz);

    void reset();

    // Process and return current LFO value (-1.0 to +1.0)
    float process();

    float getRate() const { return rateHz_; }

private:
    float sampleRate_;
    float rateHz_;        // LFO rate in Hz
    float phase_;         // Current phase (0.0 - 1.0)
    float phaseIncrement_;

    void updatePhaseIncrement();
};

} // namespace phj
