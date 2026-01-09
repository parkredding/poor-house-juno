#include "lfo.h"
#include <cmath>

namespace phj {

Lfo::Lfo()
    : sampleRate_(SAMPLE_RATE)
    , rateHz_(2.0f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
{
    updatePhaseIncrement();
}

void Lfo::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
}

void Lfo::setRate(float rateHz) {
    rateHz_ = clamp(rateHz, 0.1f, 30.0f);
    updatePhaseIncrement();
}

void Lfo::reset() {
    phase_ = 0.0f;
}

float Lfo::process() {
    // Generate triangle wave from phase
    // Triangle: ramps from -1 to +1 to -1
    float value;
    if (phase_ < 0.5f) {
        // Rising: 0.0 -> 0.5 maps to -1.0 -> +1.0
        value = -1.0f + (phase_ * 4.0f);
    } else {
        // Falling: 0.5 -> 1.0 maps to +1.0 -> -1.0
        value = 3.0f - (phase_ * 4.0f);
    }

    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }

    return clamp(value, -1.0f, 1.0f);
}

void Lfo::updatePhaseIncrement() {
    phaseIncrement_ = rateHz_ / sampleRate_;
}

} // namespace phj
