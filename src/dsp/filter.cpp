#include "filter.h"
#include <cmath>

namespace phj {

Filter::Filter()
    : sampleRate_(SAMPLE_RATE)
    , envValue_(0.0f)
    , lfoValue_(0.0f)
    , noteFrequency_(440.0f)
    , velocityValue_(1.0f)  // M14: Default to full velocity
    , velocityAmount_(0.0f)  // M14: Default to no velocity modulation
    , stage1_(0.0f)
    , stage2_(0.0f)
    , stage3_(0.0f)
    , stage4_(0.0f)
    , g_(0.0f)
    , k_(0.0f)
    , hpfState_(0.0f)
    , hpfG_(0.0f)
{
    updateCoefficients();
}

void Filter::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Filter::setParameters(const FilterParams& params) {
    params_ = params;
    updateCoefficients();
}

void Filter::setEnvValue(float envValue) {
    envValue_ = clamp(envValue, 0.0f, 1.0f);
}

void Filter::setLfoValue(float lfoValue) {
    lfoValue_ = clamp(lfoValue, -1.0f, 1.0f);
}

void Filter::setNoteFrequency(float noteFreq) {
    noteFrequency_ = noteFreq;
}

void Filter::setVelocityValue(float velocity, float amount) {
    // M14: Set velocity modulation
    velocityValue_ = clamp(velocity, 0.0f, 1.0f);
    velocityAmount_ = clamp(amount, 0.0f, 1.0f);
}

void Filter::reset() {
    stage1_ = 0.0f;
    stage2_ = 0.0f;
    stage3_ = 0.0f;
    stage4_ = 0.0f;
    hpfState_ = 0.0f;
}

Sample Filter::process(Sample input) {
    // M11: Apply HPF first (if enabled)
    if (params_.hpfMode > 0) {
        input = processHPF(input);
    }

    // Apply input drive/saturation
    input *= params_.drive;
    input = saturate(input);

    // ZDF 4-pole ladder filter
    // Based on Vadim Zavalishin's "The Art of VA Filter Design"

    // Calculate feedback amount
    float feedback = stage4_ * k_;

    // Input with feedback subtraction
    float inputWithFeedback = input - feedback;

    // Process through 4 stages
    float v1 = (inputWithFeedback - stage1_) * g_;
    float out1 = v1 + stage1_;
    stage1_ = out1 + v1;

    float v2 = (out1 - stage2_) * g_;
    float out2 = v2 + stage2_;
    stage2_ = out2 + v2;

    float v3 = (out2 - stage3_) * g_;
    float out3 = v3 + stage3_;
    stage3_ = out3 + v3;

    float v4 = (out3 - stage4_) * g_;
    float out4 = v4 + stage4_;
    stage4_ = out4 + v4;

    // Output is the 4th stage (4-pole = 24dB/octave)
    return out4;
}

void Filter::process(const Sample* input, Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i]);
    }
}

void Filter::updateCoefficients() {
    float cutoffHz = calculateCutoffHz();

    // Clamp cutoff to valid range
    cutoffHz = clamp(cutoffHz, 20.0f, sampleRate_ * 0.49f);

    // Calculate g coefficient (normalized cutoff)
    // g = tan(pi * fc / fs) for bilinear transform
    float wc = TWO_PI * cutoffHz / sampleRate_;
    g_ = std::tan(wc * 0.5f);

    // Calculate resonance coefficient
    // k controls feedback amount (0.0 = no resonance, 4.0 = self-oscillation)
    // Map resonance parameter (0-1) to k (0-4)
    k_ = params_.resonance * 4.0f;

    // M11: Calculate HPF coefficient based on mode
    // Mode 0 = Off, 1 = 30Hz, 2 = 60Hz, 3 = 120Hz
    if (params_.hpfMode > 0) {
        float hpfCutoff = 30.0f * std::pow(2.0f, params_.hpfMode - 1);  // 30, 60, 120 Hz
        float hpfWc = TWO_PI * hpfCutoff / sampleRate_;
        hpfG_ = std::tan(hpfWc * 0.5f);
    }
}

float Filter::calculateCutoffHz() {
    // Base cutoff (logarithmic mapping from 0-1 to 20Hz-20kHz)
    float baseCutoff = 20.0f * std::pow(1000.0f, params_.cutoff);

    // Envelope modulation (bipolar: -1 to +1)
    // Modulates cutoff in semitones
    float envMod = 0.0f;
    if (params_.envAmount != 0.0f) {
        // Map envelope amount to ±48 semitones (4 octaves)
        float envSemitones = params_.envAmount * 48.0f * envValue_;
        envMod = std::pow(2.0f, envSemitones / 12.0f);
    }

    // LFO modulation
    float lfoMod = 1.0f;
    if (params_.lfoAmount > 0.0f) {
        // LFO modulates ±24 semitones (2 octaves)
        float lfoSemitones = lfoValue_ * params_.lfoAmount * 24.0f;
        lfoMod = std::pow(2.0f, lfoSemitones / 12.0f);
    }

    // Key tracking
    float keyTrackMod = 1.0f;
    if (params_.keyTrack != FilterParams::KEY_TRACK_OFF) {
        // Calculate ratio between note frequency and A4 (440 Hz reference)
        float ratio = noteFrequency_ / 440.0f;

        if (params_.keyTrack == FilterParams::KEY_TRACK_HALF) {
            // Half tracking: square root of ratio
            keyTrackMod = std::sqrt(ratio);
        } else if (params_.keyTrack == FilterParams::KEY_TRACK_FULL) {
            // Full tracking: direct ratio
            keyTrackMod = ratio;
        }
    }

    // M14: Velocity modulation
    float velocityMod = 1.0f;
    if (velocityAmount_ > 0.0f) {
        // Velocity modulates ±24 semitones (2 octaves) scaled by amount
        // Higher velocity opens the filter
        float velocitySemitones = (velocityValue_ - 0.5f) * 2.0f * velocityAmount_ * 24.0f;
        velocityMod = std::pow(2.0f, velocitySemitones / 12.0f);
    }

    // Combine all modulations (multiplicative)
    float finalCutoff = baseCutoff;

    if (envMod != 0.0f) {
        finalCutoff *= envMod;
    }

    finalCutoff *= lfoMod;
    finalCutoff *= keyTrackMod;
    finalCutoff *= velocityMod;

    return finalCutoff;
}

float Filter::saturate(float x) {
    // Soft saturation using tanh
    // Provides subtle warmth characteristic of the IR3109
    // Only apply if drive > 1.0
    if (params_.drive <= 1.0f) {
        return x;
    }

    // Soft clipping
    return std::tanh(x);
}

float Filter::processHPF(float input) {
    // M11: 1-pole high-pass filter using ZDF topology
    // HPF(s) = s / (s + wc)
    float v = (input - hpfState_) * hpfG_;
    float lp = v + hpfState_;
    hpfState_ = lp + v;

    // High-pass output = input - lowpass
    return input - lp;
}

} // namespace phj
