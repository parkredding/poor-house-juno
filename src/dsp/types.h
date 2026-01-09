#pragma once

#include <cstdint>
#include <cmath>

namespace phj {

// Audio sample type
using Sample = float;

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;
constexpr int MAX_BUFFER_SIZE = 512;

// MIDI constants
constexpr int MIDI_NOTE_ON = 0x90;
constexpr int MIDI_NOTE_OFF = 0x80;
constexpr int MIDI_CONTROL_CHANGE = 0xB0;
constexpr int MIDI_CC = 0xB0;
constexpr int MIDI_PITCH_BEND = 0xE0;  // M11

// Voice constants
constexpr int NUM_VOICES = 6;

// Utility functions
inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline float midiNoteToFrequency(int note) {
    // A4 (MIDI note 69) = 440 Hz
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

inline float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

inline float linearToDb(float linear) {
    return 20.0f * std::log10(linear);
}

} // namespace phj
