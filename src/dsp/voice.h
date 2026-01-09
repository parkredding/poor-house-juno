#pragma once

#include "types.h"
#include "parameters.h"
#include "dco.h"
#include "filter.h"
#include "envelope.h"

namespace phj {

/**
 * Voice - Single voice synthesizer
 *
 * Integrates DCO, Filter, and Envelopes into a complete voice.
 * Each voice is monophonic and can play one note at a time.
 * For polyphony, multiple Voice instances are managed by the Synth class.
 */
class Voice {
public:
    Voice();

    void setSampleRate(float sampleRate);
    void setParameters(const DcoParams& dcoParams,
                      const FilterParams& filterParams,
                      const EnvelopeParams& filterEnvParams,
                      const EnvelopeParams& ampEnvParams);

    // Voice control
    void noteOn(int midiNote, float velocity = 1.0f);
    void noteOff();
    void reset();

    // Modulation input (from shared LFO)
    void setLfoValue(float lfoValue);  // -1.0 to 1.0

    // Process single sample
    Sample process();

    // Process buffer
    void process(Sample* output, int numSamples);

    // Voice state queries
    bool isActive() const;
    bool isReleasing() const;
    int getCurrentNote() const { return currentNote_; }
    float getAge() const { return age_; }

private:
    // DSP components
    Dco dco_;
    Filter filter_;
    Envelope filterEnv_;
    Envelope ampEnv_;

    // Voice state
    int currentNote_;       // Current MIDI note (-1 if no note)
    float velocity_;        // Note velocity (0.0 - 1.0)
    float age_;            // Voice age in samples (for voice stealing)
    bool noteActive_;      // True if note is currently held

    float sampleRate_;
    float lfoValue_;
};

} // namespace phj
