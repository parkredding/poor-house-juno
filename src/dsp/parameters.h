#pragma once

#include "types.h"

namespace phj {

/**
 * DCO (Digitally Controlled Oscillator) parameters
 * Matches the Juno-106 oscillator section
 */
struct DcoParams {
    // Waveform levels (0.0 - 1.0)
    float sawLevel;      // Sawtooth level
    float pulseLevel;    // Pulse wave level
    float subLevel;      // Sub-oscillator level (square, -1 octave)
    float noiseLevel;    // White noise level

    // Pulse Width Modulation
    float pulseWidth;    // 0.05 - 0.95 (0.5 = square wave)
    float pwmDepth;      // 0.0 - 1.0 (LFO modulation amount)

    // LFO routing
    enum LfoTarget {
        LFO_OFF = 0,
        LFO_PITCH = 1,
        LFO_PWM = 2,
        LFO_BOTH = 3
    };
    int lfoTarget;       // LFO destination

    // Voice characteristics
    float detune;        // Per-voice detune in cents (±1 cent typical)
    bool enableDrift;    // Enable pitch drift emulation

    DcoParams()
        : sawLevel(0.5f)
        , pulseLevel(0.0f)
        , subLevel(0.0f)
        , noiseLevel(0.0f)
        , pulseWidth(0.5f)
        , pwmDepth(0.0f)
        , lfoTarget(LFO_OFF)
        , detune(0.0f)
        , enableDrift(true)
    {}
};

/**
 * Filter (IR3109 4-pole ladder) parameters
 */
struct FilterParams {
    float cutoff;        // 0.0 - 1.0 (maps to 20Hz - 20kHz logarithmically)
    float resonance;     // 0.0 - 1.0 (self-oscillation at ~0.95+)
    float envAmount;     // -1.0 - 1.0 (bipolar envelope modulation)
    float lfoAmount;     // 0.0 - 1.0 (LFO modulation depth)

    enum KeyTrack {
        KEY_TRACK_OFF = 0,
        KEY_TRACK_HALF = 1,
        KEY_TRACK_FULL = 2
    };
    int keyTrack;        // Key tracking mode

    float drive;         // 1.0 - 4.0 (internal saturation, subtle)

    FilterParams()
        : cutoff(0.5f)
        , resonance(0.0f)
        , envAmount(0.0f)
        , lfoAmount(0.0f)
        , keyTrack(KEY_TRACK_OFF)
        , drive(1.0f)
    {}
};

/**
 * ADSR Envelope parameters
 */
struct EnvelopeParams {
    float attack;    // 0.001 - 3.0 seconds
    float decay;     // 0.002 - 12.0 seconds
    float sustain;   // 0.0 - 1.0 level
    float release;   // 0.002 - 12.0 seconds

    EnvelopeParams()
        : attack(0.01f)
        , decay(0.3f)
        , sustain(0.7f)
        , release(0.5f)
    {}
};

/**
 * LFO parameters
 */
struct LfoParams {
    float rate;      // 0.1 - 30.0 Hz

    LfoParams()
        : rate(2.0f)
    {}
};

/**
 * Parameter IDs for external control
 */
enum class ParamId {
    // DCO
    DCO_SAW_LEVEL = 0,
    DCO_PULSE_LEVEL,
    DCO_SUB_LEVEL,
    DCO_NOISE_LEVEL,
    DCO_PULSE_WIDTH,
    DCO_PWM_DEPTH,
    DCO_LFO_TARGET,

    // Filter
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_ENV_AMOUNT,
    FILTER_LFO_AMOUNT,
    FILTER_KEY_TRACK,

    // Envelopes
    FILTER_ENV_ATTACK,
    FILTER_ENV_DECAY,
    FILTER_ENV_SUSTAIN,
    FILTER_ENV_RELEASE,

    AMP_ENV_ATTACK,
    AMP_ENV_DECAY,
    AMP_ENV_SUSTAIN,
    AMP_ENV_RELEASE,

    // LFO
    LFO_RATE,

    // Global
    MASTER_VOLUME,
    MASTER_TUNE,

    PARAM_COUNT
};

} // namespace phj
