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

    // Global
    MASTER_VOLUME,
    MASTER_TUNE,

    PARAM_COUNT
};

} // namespace phj
