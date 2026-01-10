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

    // M11: Performance controls
    void setPitchBend(float pitchBend, float pitchBendRange);  // pitchBend: -1.0 to 1.0
    void setPortamentoTime(float portamentoTime);  // 0.0 - 10.0 seconds

    // M13: Performance controls
    void setVcaMode(int vcaMode);  // 0=ENV, 1=GATE
    void setFilterEnvPolarity(int filterEnvPolarity);  // 0=Normal, 1=Inverse

    // M14: Range & Voice Control
    void setVcaLevel(float vcaLevel);  // 0.0 - 1.0
    void setVelocitySensitivity(float filterAmount, float ampAmount);  // 0.0 - 1.0
    void setMasterTune(float cents);  // ±50 cents

    // M16: Sustain pedal support
    void setSustained(bool sustained);  // Mark voice as sustained
    bool isSustained() const { return sustained_; }

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
    bool sustained_;       // M16: True if note is sustained by pedal

    float sampleRate_;
    float lfoValue_;

    // M11: Pitch bend state
    float pitchBend_;       // -1.0 to 1.0
    float pitchBendRange_;  // Semitones

    // M11: Portamento state
    float portamentoTime_;  // Seconds
    int targetNote_;        // Target MIDI note for glide
    float currentFreq_;     // Current frequency (with glide)
    float targetFreq_;      // Target frequency
    float glideRate_;       // Frequency change per sample

    // M13: Performance control state
    int vcaMode_;           // 0=ENV, 1=GATE
    int filterEnvPolarity_; // 0=Normal, 1=Inverse

    // M14: Range & Voice Control state
    float vcaLevel_;            // VCA output level (0.0 - 1.0)
    float velocityToFilter_;    // Velocity sensitivity for filter (0.0 - 1.0)
    float velocityToAmp_;       // Velocity sensitivity for amplitude (0.0 - 1.0)
    float masterTune_;          // Master tune in cents (±50)

    void updateGlide();     // M11: Update portamento glide
};

} // namespace phj
