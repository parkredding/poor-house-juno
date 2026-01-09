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
    void setDelay(float delaySeconds);  // M12: LFO delay time

    void reset();
    void trigger();  // M12: Trigger delay timer (called on note-on)

    // Process and return current LFO value (-1.0 to +1.0)
    // The value is automatically scaled during the delay period
    float process();

    float getRate() const { return rateHz_; }
    float getDelay() const { return delaySeconds_; }

private:
    float sampleRate_;
    float rateHz_;        // LFO rate in Hz
    float phase_;         // Current phase (0.0 - 1.0)
    float phaseIncrement_;

    // M12: LFO Delay
    float delaySeconds_;  // Delay time in seconds
    float delayTimer_;    // Current delay timer (counts up from 0)
    float delayScale_;    // Current scale factor (0.0 - 1.0)

    void updatePhaseIncrement();
    void updateDelayScale();  // M12: Update the delay scale factor
};

} // namespace phj
