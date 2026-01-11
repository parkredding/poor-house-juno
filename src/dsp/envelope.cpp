#include "envelope.h"
#include <cmath>

namespace phj {

Envelope::Envelope()
    : sampleRate_(SAMPLE_RATE)
    , stage_(IDLE)
    , value_(0.0f)
    , targetValue_(0.0f)
    , attackCoeff_(0.0f)
    , decayCoeff_(0.0f)
    , releaseCoeff_(0.0f)
{
    updateCoefficients();
}

void Envelope::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Envelope::setParameters(const EnvelopeParams& params) {
    params_ = params;
    updateCoefficients();
}

void Envelope::noteOn() {
    // If starting from idle, initialize to sustain level to prevent
    // harsh filter sweep artifact. This makes the attack sweep from
    // sustain→peak instead of 0→peak, reducing the sweep range.
    if (stage_ == IDLE) {
        value_ = params_.sustain;
    }
    stage_ = ATTACK;
    targetValue_ = 1.0f;
}

void Envelope::noteOff() {
    if (stage_ != IDLE) {
        stage_ = RELEASE;
        targetValue_ = 0.0f;
    }
}

void Envelope::reset() {
    stage_ = IDLE;
    value_ = 0.0f;
    targetValue_ = 0.0f;
}

float Envelope::process() {
    switch (stage_) {
        case IDLE:
            return 0.0f;

        case ATTACK:
            // Exponential rise to 1.0
            value_ += (targetValue_ - value_) * attackCoeff_;

            // Transition to decay when close to target
            if (value_ >= 0.999f) {
                value_ = 1.0f;
                stage_ = DECAY;
                targetValue_ = params_.sustain;
            }
            break;

        case DECAY:
            // Exponential fall to sustain level
            value_ += (targetValue_ - value_) * decayCoeff_;

            // Transition to sustain when close to target
            if (std::abs(value_ - targetValue_) < 0.001f) {
                value_ = targetValue_;
                stage_ = SUSTAIN;
            }
            break;

        case SUSTAIN:
            // Hold at sustain level
            value_ = params_.sustain;
            break;

        case RELEASE:
            // Exponential fall to 0.0
            value_ += (targetValue_ - value_) * releaseCoeff_;

            // Transition to idle when close to zero
            // Use larger threshold to avoid denormal CPU slowdown
            if (value_ <= 0.0001f) {
                value_ = 0.0f;
                stage_ = IDLE;
            }
            break;
    }

    return clamp(value_, 0.0f, 1.0f);
}

void Envelope::updateCoefficients() {
    attackCoeff_ = calculateCoefficient(params_.attack);
    decayCoeff_ = calculateCoefficient(params_.decay);
    releaseCoeff_ = calculateCoefficient(params_.release);
}

float Envelope::calculateCoefficient(float timeSeconds) {
    // Calculate coefficient for exponential curve
    // Formula: coeff = 1 - exp(-4.6 / (time * sampleRate))
    // This reaches approximately 99% of target in the specified time

    if (timeSeconds <= 0.0f) {
        return 1.0f;  // Instant
    }

    float samples = timeSeconds * sampleRate_;
    // Use 4.6 time constants to reach 99% in the specified time
    return 1.0f - std::exp(-4.6f / samples);
}

} // namespace phj
