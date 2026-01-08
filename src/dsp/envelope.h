#pragma once

#include "types.h"
#include "parameters.h"

namespace phj {

/**
 * ADSR Envelope Generator
 *
 * Classic Attack-Decay-Sustain-Release envelope with exponential curves.
 * Used for filter and amplitude modulation.
 */
class Envelope {
public:
    enum Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    Envelope();

    void setSampleRate(float sampleRate);
    void setParameters(const EnvelopeParams& params);

    void noteOn();
    void noteOff();
    void reset();

    // Process and return current envelope value (0.0 - 1.0)
    float process();

    Stage getStage() const { return stage_; }
    bool isActive() const { return stage_ != IDLE; }

private:
    float sampleRate_;
    EnvelopeParams params_;

    Stage stage_;
    float value_;           // Current envelope value
    float targetValue_;     // Target value for current stage

    // Exponential curve coefficients
    float attackCoeff_;
    float decayCoeff_;
    float releaseCoeff_;

    void updateCoefficients();
    float calculateCoefficient(float timeSeconds);
};

} // namespace phj
