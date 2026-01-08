#pragma once

#include "types.h"
#include "parameters.h"
#include <random>

namespace phj {

/**
 * DCO (Digitally Controlled Oscillator)
 *
 * Implements the Juno-106 oscillator with:
 * - Band-limited sawtooth (polyBLEP)
 * - Band-limited pulse with PWM (polyBLEP)
 * - Sub-oscillator (square wave, -1 octave)
 * - White noise generator
 * - Pitch drift emulation
 * - Per-voice detuning
 */
class Dco {
public:
    Dco();

    void setSampleRate(float sampleRate);
    void setFrequency(float frequency);
    void setParameters(const DcoParams& params);
    void setLfoValue(float lfoValue);  // -1.0 to 1.0

    void noteOn();
    void noteOff();
    void reset();

    // Process single sample
    Sample process();

    // Process buffer
    void process(Sample* output, int numSamples);

private:
    // Oscillator state
    float sampleRate_;
    float baseFrequency_;    // Base frequency without modulation
    float currentFrequency_; // Current frequency with drift/detune
    DcoParams params_;

    // Phase accumulators
    float mainPhase_;        // Main oscillator phase (0.0 - 1.0)
    float subPhase_;         // Sub-oscillator phase (0.0 - 1.0)

    // Phase increments
    float mainPhaseInc_;
    float subPhaseInc_;

    // LFO modulation
    float lfoValue_;

    // Pitch drift state
    float driftAmount_;      // Current drift amount in cents
    float driftTarget_;      // Target drift amount
    int driftCounter_;       // Sample counter for drift updates
    static constexpr int DRIFT_UPDATE_SAMPLES = 4800; // ~100ms at 48kHz

    // Random number generator (for noise and drift)
    std::mt19937 rng_;
    std::uniform_real_distribution<float> noiseDist_;
    std::normal_distribution<float> driftDist_;

    // Internal methods
    void updatePhaseIncrements();
    void updateDrift();

    // Waveform generators (with polyBLEP anti-aliasing)
    Sample generateSaw(float phase, float phaseInc);
    Sample generatePulse(float phase, float phaseInc, float pulseWidth);
    Sample generateSub(float phase);
    Sample generateNoise();

    // PolyBLEP anti-aliasing
    float polyBlep(float t, float dt);
};

} // namespace phj
