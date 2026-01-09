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

    // M14: Range selection (octave shifting)
    enum Range {
        RANGE_16 = 0,    // 16' (down 1 octave)
        RANGE_8 = 1,     // 8' (normal pitch)
        RANGE_4 = 2      // 4' (up 1 octave)
    };
    int range;           // DCO range/footage

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
        , range(RANGE_8)
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

    // M11: High-Pass Filter
    int hpfMode;         // 0=Off, 1=Low, 2=Medium, 3=High

    FilterParams()
        : cutoff(0.5f)
        , resonance(0.0f)
        , envAmount(0.0f)
        , lfoAmount(0.0f)
        , keyTrack(KEY_TRACK_OFF)
        , drive(1.0f)
        , hpfMode(0)
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
    float delay;     // 0.0 - 3.0 seconds (M12: delay before modulation starts)

    LfoParams()
        : rate(2.0f)
        , delay(0.0f)
    {}
};

/**
 * Chorus parameters
 */
struct ChorusParams {
    int mode;        // 0=Off, 1=Mode I, 2=Mode II, 3=Mode I+II

    ChorusParams()
        : mode(0)  // Off by default
    {}
};

/**
 * M11: Performance parameters (Pitch Bend and Portamento)
 * M13: Performance Controls (Mod Wheel, VCA Mode, Filter Env Polarity)
 * M14: Range & Voice Control (VCA Level, Velocity Sensitivity, Master Tune)
 */
struct PerformanceParams {
    float pitchBend;         // -1.0 to 1.0 (pitch bend amount)
    float pitchBendRange;    // Semitones (default ±2)
    float portamentoTime;    // 0.0 - 10.0 seconds (glide time)

    // M13: Performance Controls
    float modWheel;          // 0.0 - 1.0 (modulation wheel amount, MIDI CC #1)

    enum VcaMode {
        VCA_ENV = 0,         // Amplitude envelope controls output (normal)
        VCA_GATE = 1         // Gate mode: instant on/off (organ-style)
    };
    int vcaMode;             // VCA control mode

    enum FilterEnvPolarity {
        FILTER_ENV_NORMAL = 0,   // Envelope opens filter (normal)
        FILTER_ENV_INVERSE = 1   // Envelope closes filter (inverted)
    };
    int filterEnvPolarity;   // Filter envelope polarity

    // M14: Range & Voice Control
    float vcaLevel;          // 0.0 - 1.0 (VCA output level, separate from master volume)
    float masterTune;        // -50.0 to +50.0 cents (global pitch offset)

    // M14: Velocity sensitivity amounts (0.0 - 1.0 for each parameter)
    float velocityToFilter;  // How much velocity affects filter cutoff
    float velocityToAmp;     // How much velocity affects amplitude

    PerformanceParams()
        : pitchBend(0.0f)
        , pitchBendRange(2.0f)
        , portamentoTime(0.0f)
        , modWheel(0.0f)
        , vcaMode(VCA_ENV)
        , filterEnvPolarity(FILTER_ENV_NORMAL)
        , vcaLevel(0.8f)
        , masterTune(0.0f)
        , velocityToFilter(0.0f)
        , velocityToAmp(1.0f)
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
    DCO_RANGE,  // M14

    // Filter
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_ENV_AMOUNT,
    FILTER_LFO_AMOUNT,
    FILTER_KEY_TRACK,
    FILTER_HPF_MODE,

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
    LFO_DELAY,

    // Chorus
    CHORUS_MODE,

    // M11: Performance
    PITCH_BEND,
    PITCH_BEND_RANGE,
    PORTAMENTO_TIME,

    // M13: Performance Controls
    MOD_WHEEL,
    VCA_MODE,
    FILTER_ENV_POLARITY,

    // M14: Range & Voice Control
    VCA_LEVEL,
    MASTER_TUNE,
    VELOCITY_TO_FILTER,
    VELOCITY_TO_AMP,

    // Global
    MASTER_VOLUME,

    PARAM_COUNT
};

} // namespace phj
