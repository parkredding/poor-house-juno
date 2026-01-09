#pragma once

#include "types.h"
#include "parameters.h"
#include "voice.h"
#include "lfo.h"

namespace phj {

/**
 * Synth - Main synthesizer engine
 *
 * Manages 6-voice polyphony, LFO, and global parameters.
 * M7: 6-voice polyphony with voice stealing
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

    // 6 voices for polyphony
    Voice voices_[NUM_VOICES];

    // Current parameters
    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;

    // Voice management helpers
    int findFreeVoice() const;
    int findVoiceToSteal() const;
};

} // namespace phj
