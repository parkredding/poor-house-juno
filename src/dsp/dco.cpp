#include "dco.h"
#include <cmath>

namespace phj {

Dco::Dco()
    : sampleRate_(SAMPLE_RATE)
    , baseFrequency_(440.0f)
    , currentFrequency_(440.0f)
    , mainPhase_(0.0f)
    , subPhase_(0.0f)
    , mainPhaseInc_(0.0f)
    , subPhaseInc_(0.0f)
    , lfoValue_(0.0f)
    , driftAmount_(0.0f)
    , driftTarget_(0.0f)
    , driftCounter_(0)
    , rng_(std::random_device{}())
    , noiseDist_(-1.0f, 1.0f)
    , driftDist_(0.0f, 0.5f)  // ±0.5 cents standard deviation
{
    updatePhaseIncrements();
}

void Dco::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrements();
}

void Dco::setFrequency(float frequency) {
    baseFrequency_ = frequency;
    updatePhaseIncrements();
}

void Dco::setParameters(const DcoParams& params) {
    params_ = params;
    // No need to update phase increments here unless frequency changed
}

void Dco::setLfoValue(float lfoValue) {
    lfoValue_ = clamp(lfoValue, -1.0f, 1.0f);
}

void Dco::noteOn() {
    // Random phase on note-on (Juno characteristic)
    std::uniform_real_distribution<float> phaseDist(0.0f, 1.0f);
    mainPhase_ = phaseDist(rng_);
    subPhase_ = phaseDist(rng_);

    // Reset drift
    driftAmount_ = 0.0f;
    driftTarget_ = params_.enableDrift ? driftDist_(rng_) : 0.0f;
    driftCounter_ = 0;
}

void Dco::noteOff() {
    // Nothing special on note-off for DCO
}

void Dco::reset() {
    mainPhase_ = 0.0f;
    subPhase_ = 0.0f;
    driftAmount_ = 0.0f;
    driftTarget_ = 0.0f;
    driftCounter_ = 0;
}

Sample Dco::process() {
    // Update pitch drift and phase increments
    updateDrift();

    // Always update phase increments to apply LFO modulation
    // (updateDrift only calls this if drift is enabled)
    if (!params_.enableDrift) {
        updatePhaseIncrements();
    }

    // Calculate current pulse width (with LFO modulation if enabled)
    float pulseWidth = params_.pulseWidth;
    if (params_.lfoTarget == DcoParams::LFO_PWM ||
        params_.lfoTarget == DcoParams::LFO_BOTH) {
        // LFO modulates pulse width
        pulseWidth += lfoValue_ * params_.pwmDepth * 0.4f; // ±40% modulation range
        pulseWidth = clamp(pulseWidth, 0.05f, 0.95f);
    }

    // Generate waveforms
    Sample saw = params_.sawLevel * generateSaw(mainPhase_, mainPhaseInc_);
    Sample pulse = params_.pulseLevel * generatePulse(mainPhase_, mainPhaseInc_, pulseWidth);
    Sample sub = params_.subLevel * generateSub(subPhase_);
    Sample noise = params_.noiseLevel * generateNoise();

    // Mix waveforms
    Sample output = saw + pulse + sub + noise;

    // Advance phases
    mainPhase_ += mainPhaseInc_;
    subPhase_ += subPhaseInc_;

    // Wrap phases to [0, 1)
    if (mainPhase_ >= 1.0f) mainPhase_ -= 1.0f;
    if (subPhase_ >= 1.0f) subPhase_ -= 1.0f;

    return output;
}

void Dco::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

void Dco::updatePhaseIncrements() {
    // M14: Apply range/octave shift
    float rangeFactor = 1.0f;
    switch (params_.range) {
        case DcoParams::RANGE_16:
            rangeFactor = 0.5f;  // 16' = down 1 octave
            break;
        case DcoParams::RANGE_8:
            rangeFactor = 1.0f;  // 8' = normal pitch
            break;
        case DcoParams::RANGE_4:
            rangeFactor = 2.0f;  // 4' = up 1 octave
            break;
    }

    // Calculate total frequency with detune, drift, and LFO
    float detuneFactor = std::pow(2.0f, params_.detune / 1200.0f);
    float driftFactor = std::pow(2.0f, driftAmount_ / 1200.0f);

    float pitchMod = 1.0f;
    if (params_.lfoTarget == DcoParams::LFO_PITCH ||
        params_.lfoTarget == DcoParams::LFO_BOTH) {
        // LFO pitch modulation: ±1 semitone range typical
        pitchMod = std::pow(2.0f, lfoValue_ / 12.0f);
    }

    currentFrequency_ = baseFrequency_ * rangeFactor * detuneFactor * driftFactor * pitchMod;

    // Calculate phase increments
    mainPhaseInc_ = currentFrequency_ / sampleRate_;
    subPhaseInc_ = (currentFrequency_ * 0.5f) / sampleRate_; // -1 octave = half frequency
}

void Dco::updateDrift() {
    if (!params_.enableDrift) {
        driftAmount_ = 0.0f;
        return;
    }

    driftCounter_++;

    // Update drift target every ~100ms
    if (driftCounter_ >= DRIFT_UPDATE_SAMPLES) {
        driftTarget_ = driftDist_(rng_);
        driftCounter_ = 0;
    }

    // Smoothly move towards target (simple low-pass filter)
    float alpha = 0.0001f; // Very slow drift
    driftAmount_ += alpha * (driftTarget_ - driftAmount_);

    // Update phase increments with new drift
    updatePhaseIncrements();
}

Sample Dco::generateSaw(float phase, float phaseInc) {
    // Naive sawtooth: ramp from -1 to +1
    Sample saw = 2.0f * phase - 1.0f;

    // Apply polyBLEP anti-aliasing at discontinuity
    saw -= polyBlep(phase, phaseInc);

    return saw;
}

Sample Dco::generatePulse(float phase, float phaseInc, float pulseWidth) {
    // Naive pulse wave
    Sample pulse = (phase < pulseWidth) ? 1.0f : -1.0f;

    // Apply polyBLEP at both discontinuities
    pulse += polyBlep(phase, phaseInc);                        // Rising edge at 0
    pulse -= polyBlep(std::fmod(phase + (1.0f - pulseWidth), 1.0f), phaseInc); // Falling edge

    return pulse;
}

Sample Dco::generateSub(float phase) {
    // Simple square wave (no polyBLEP needed for sub-oscillator at -1 octave)
    // At low frequencies, aliasing is negligible
    return (phase < 0.5f) ? 1.0f : -1.0f;
}

Sample Dco::generateNoise() {
    // White noise
    return noiseDist_(rng_);
}

float Dco::polyBlep(float t, float dt) {
    // PolyBLEP (Polynomial Bandlimited Step)
    // Reduces aliasing at discontinuities
    // t: phase position (0.0 - 1.0)
    // dt: phase increment per sample

    // Handle discontinuity at t = 0
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    // Handle discontinuity at t = 1
    else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}

} // namespace phj
