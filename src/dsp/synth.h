#pragma once

#include "types.h"
#include "parameters.h"
#include "voice.h"
#include "lfo.h"
#include "chorus.h"

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
    void setChorusParameters(const ChorusParams& params);
    void setPerformanceParameters(const PerformanceParams& params);  // M11

    // MIDI handling
    void handleNoteOn(int midiNote, float velocity = 1.0f);
    void handleNoteOff(int midiNote);
    void allNotesOff();
    void handlePitchBend(float pitchBend);  // M11: -1.0 to 1.0

    // Audio processing
    Sample process();  // Mono output (stereo mixed down)
    void process(Sample* output, int numSamples);
    void processStereo(Sample& leftOut, Sample& rightOut);  // Stereo output with chorus
    void processStereo(Sample* leftOutput, Sample* rightOutput, int numSamples);

    // Reset all state
    void reset();

private:
    float sampleRate_;

    // Global LFO (shared by all voices)
    Lfo lfo_;
    LfoParams lfoParams_;

    // 6 voices for polyphony
    Voice voices_[NUM_VOICES];

    // Chorus effect
    Chorus chorus_;
    ChorusParams chorusParams_;

    // Current parameters
    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;
    PerformanceParams performanceParams_;  // M11

    // Voice management helpers
    int findFreeVoice() const;
    int findVoiceToSteal() const;
};

} // namespace phj
