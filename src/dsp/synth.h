#pragma once

#include "types.h"
#include "parameters.h"
#include "voice.h"
#include "lfo.h"

namespace phj {

/**
 * Synth - Main synthesizer engine
 *
 * Manages voice(s), LFO, and global parameters.
 * M6: Single voice operation
 * M7: Will be extended to 6-voice polyphony
 */
class Synth {
public:
    Synth();

    void setSampleRate(float sampleRate);

    // Parameter setting
    void setDcoParameters(const DcoParams& params);
    void setFilterParameters(const FilterParams& params);
    void setFilterEnvParameters(const EnvelopeParams& params);
    void setAmpEnvParameters(const EnvelopeParams& params);
    void setLfoParameters(const LfoParams& params);

    // MIDI handling
    void handleNoteOn(int midiNote, float velocity = 1.0f);
    void handleNoteOff(int midiNote);
    void allNotesOff();

    // Audio processing
    Sample process();
    void process(Sample* output, int numSamples);

    // Reset all state
    void reset();

private:
    float sampleRate_;

    // Global LFO (shared by all voices)
    Lfo lfo_;
    LfoParams lfoParams_;

    // Voice (single voice for M6, will expand to array in M7)
    Voice voice_;

    // Current parameters
    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;
};

} // namespace phj
