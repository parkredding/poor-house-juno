#pragma once

#include "dco.h"
#include "filter.h"
#include "envelope.h"
#include "lfo.h"
#include "parameters.h"
#include "types.h"

namespace phj {

/**
 * Voice - Single synthesizer voice
 *
 * Encapsulates all DSP components for a single voice:
 * - DCO (oscillator with multiple waveforms)
 * - Filter (IR3109 4-pole ladder)
 * - Filter Envelope (ADSR)
 * - Amplitude Envelope (ADSR)
 * - LFO (shared across voices, but processed per-voice)
 *
 * Signal flow:
 *   LFO → DCO → Filter → Amp Envelope → Output
 *          ↑       ↑
 *          |       Filter Envelope
 *          |
 *          LFO (optional routing)
 *
 * This class is designed to be reusable for polyphonic synthesis (M7).
 */
class Voice {
public:
    Voice();
    ~Voice() = default;

    // Initialization
    void setSampleRate(float sampleRate);

    // Audio processing
    Sample process(float lfoValue);

    // MIDI control
    void noteOn(int midiNote, int velocity);
    void noteOff();
    bool isActive() const { return isActive_; }
    int getCurrentNote() const { return currentNote_; }

    // Parameter control - DCO
    void setDcoParams(const DcoParams& params);

    // Parameter control - Filter
    void setFilterParams(const FilterParams& params);

    // Parameter control - Envelopes
    void setFilterEnvParams(const EnvelopeParams& params);
    void setAmpEnvParams(const EnvelopeParams& params);

    // Direct frequency control (for testing/legacy)
    void setFrequency(float freq);

private:
    // DSP components
    Dco dco_;
    Filter filter_;
    Envelope filterEnv_;
    Envelope ampEnv_;

    // Parameters
    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;

    // Voice state
    bool isActive_;
    int currentNote_;
    float currentFreq_;
    float sampleRate_;
};

} // namespace phj
