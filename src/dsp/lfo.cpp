#include "lfo.h"
#include <cmath>

namespace phj {

Lfo::Lfo()
    : sampleRate_(SAMPLE_RATE)
    , rateHz_(2.0f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
    , delaySeconds_(0.0f)
    , delayTimer_(0.0f)
    , delayScale_(1.0f)
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

void Lfo::setDelay(float delaySeconds) {
    delaySeconds_ = clamp(delaySeconds, 0.0f, 3.0f);
}

void Lfo::reset() {
    phase_ = 0.0f;
    delayTimer_ = 0.0f;
    delayScale_ = (delaySeconds_ > 0.0f) ? 0.0f : 1.0f;
}

void Lfo::trigger() {
    delayTimer_ = 0.0f;
    delayScale_ = (delaySeconds_ > 0.0f) ? 0.0f : 1.0f;
}

float Lfo::process() {
    // M12: Update delay timer and scale
    updateDelayScale();

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

    // M12: Apply delay scale (fade in LFO after delay)
    return clamp(value * delayScale_, -1.0f, 1.0f);
}

void Lfo::updatePhaseIncrement() {
    phaseIncrement_ = rateHz_ / sampleRate_;
}

void Lfo::updateDelayScale() {
    if (delaySeconds_ <= 0.0f) {
        delayScale_ = 1.0f;
        return;
    }

    if (delayTimer_ < delaySeconds_) {
        // During delay period, fade in from 0 to 1
        delayTimer_ += 1.0f / sampleRate_;
        if (delayTimer_ >= delaySeconds_) {
            delayScale_ = 1.0f;
            delayTimer_ = delaySeconds_;
        } else {
            // Linear fade-in
            delayScale_ = delayTimer_ / delaySeconds_;
        }
    } else {
        delayScale_ = 1.0f;
    }
}

} // namespace phj
