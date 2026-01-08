#include "oscillator.h"

namespace phj {

SineOscillator::SineOscillator()
    : sampleRate_(SAMPLE_RATE)
    , frequency_(440.0f)
    , amplitude_(0.5f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
{
    updatePhaseIncrement();
}

void SineOscillator::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
}

void SineOscillator::setFrequency(float frequency) {
    frequency_ = frequency;
    updatePhaseIncrement();
}

void SineOscillator::setAmplitude(float amplitude) {
    amplitude_ = clamp(amplitude, 0.0f, 1.0f);
}

void SineOscillator::reset() {
    phase_ = 0.0f;
}

Sample SineOscillator::process() {
    Sample output = amplitude_ * std::sin(phase_);

    // Advance phase
    phase_ += phaseIncrement_;

    // Wrap phase to [0, 2π)
    if (phase_ >= TWO_PI) {
        phase_ -= TWO_PI;
    }

    return output;
}

void SineOscillator::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

void SineOscillator::updatePhaseIncrement() {
    phaseIncrement_ = TWO_PI * frequency_ / sampleRate_;
}

} // namespace phj
