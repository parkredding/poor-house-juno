#include "voice.h"

namespace phj {

Voice::Voice()
    : currentNote_(-1)
    , velocity_(0.0f)
    , age_(0.0f)
    , noteActive_(false)
    , sampleRate_(SAMPLE_RATE)
    , lfoValue_(0.0f)
{
    dco_.setSampleRate(sampleRate_);
    filter_.setSampleRate(sampleRate_);
    filterEnv_.setSampleRate(sampleRate_);
    ampEnv_.setSampleRate(sampleRate_);
}

void Voice::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    dco_.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
}

void Voice::setParameters(const DcoParams& dcoParams,
                         const FilterParams& filterParams,
                         const EnvelopeParams& filterEnvParams,
                         const EnvelopeParams& ampEnvParams) {
    dco_.setParameters(dcoParams);
    filter_.setParameters(filterParams);
    filterEnv_.setParameters(filterEnvParams);
    ampEnv_.setParameters(ampEnvParams);
}

void Voice::noteOn(int midiNote, float velocity) {
    currentNote_ = midiNote;
    velocity_ = clamp(velocity, 0.0f, 1.0f);
    age_ = 0.0f;
    noteActive_ = true;

    // Set frequency for DCO and filter
    float frequency = midiNoteToFrequency(midiNote);
    dco_.setFrequency(frequency);
    filter_.setNoteFrequency(frequency);

    // Trigger envelopes and oscillator
    dco_.noteOn();
    filterEnv_.noteOn();
    ampEnv_.noteOn();
}

void Voice::noteOff() {
    noteActive_ = false;
    dco_.noteOff();
    filterEnv_.noteOff();
    ampEnv_.noteOff();
}

void Voice::reset() {
    currentNote_ = -1;
    velocity_ = 0.0f;
    age_ = 0.0f;
    noteActive_ = false;
    lfoValue_ = 0.0f;

    dco_.reset();
    filter_.reset();
    filterEnv_.reset();
    ampEnv_.reset();
}

void Voice::setLfoValue(float lfoValue) {
    lfoValue_ = lfoValue;
    dco_.setLfoValue(lfoValue);
    filter_.setLfoValue(lfoValue);
}

Sample Voice::process() {
    // Update voice age if active
    if (isActive()) {
        age_ += 1.0f;
    }

    // Process envelopes
    float filterEnvValue = filterEnv_.process();
    float ampEnvValue = ampEnv_.process();

    // Set filter envelope modulation
    filter_.setEnvValue(filterEnvValue);

    // Generate oscillator output
    Sample dcoOut = dco_.process();

    // Process through filter
    Sample filtered = filter_.process(dcoOut);

    // Apply amplitude envelope and velocity
    Sample output = filtered * ampEnvValue * velocity_;

    return output;
}

void Voice::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

bool Voice::isActive() const {
    // Voice is active if either envelope is active
    return filterEnv_.isActive() || ampEnv_.isActive();
}

bool Voice::isReleasing() const {
    // Voice is releasing if note is off but still active
    return !noteActive_ && isActive();
}

} // namespace phj
