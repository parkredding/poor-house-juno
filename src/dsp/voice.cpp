#include "voice.h"

namespace phj {

Voice::Voice()
    : isActive_(false)
    , currentNote_(-1)
    , currentFreq_(440.0f)
    , sampleRate_(48000.0f)
{
    // Initialize with default parameters
    // DCO - sawtooth wave by default
    dcoParams_.sawLevel = 0.5f;
    dcoParams_.pulseLevel = 0.0f;
    dcoParams_.subLevel = 0.0f;
    dcoParams_.noiseLevel = 0.0f;
    dcoParams_.pulseWidth = 0.5f;
    dcoParams_.pwmDepth = 0.0f;
    dcoParams_.lfoTarget = DcoParams::LFO_OFF;
    dcoParams_.detune = 0.0f;
    dcoParams_.enableDrift = true;

    // Filter - fairly open by default
    filterParams_.cutoff = 0.8f;
    filterParams_.resonance = 0.0f;
    filterParams_.envAmount = 0.0f;
    filterParams_.lfoAmount = 0.0f;
    filterParams_.keyTrack = FilterParams::KEY_TRACK_OFF;
    filterParams_.drive = 1.0f;

    // Filter envelope
    filterEnvParams_.attack = 0.01f;
    filterEnvParams_.decay = 0.3f;
    filterEnvParams_.sustain = 0.7f;
    filterEnvParams_.release = 0.5f;

    // Amplitude envelope
    ampEnvParams_.attack = 0.005f;
    ampEnvParams_.decay = 0.3f;
    ampEnvParams_.sustain = 0.8f;
    ampEnvParams_.release = 0.3f;
}

void Voice::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    dco_.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
}

Sample Voice::process(float lfoValue) {
    // Route LFO to DCO and/or Filter
    dco_.setLfoValue(lfoValue);
    filter_.setLfoValue(lfoValue);

    // Process envelopes
    float filterEnvValue = filterEnv_.process();
    float ampEnvValue = ampEnv_.process();

    // Set filter envelope modulation
    filter_.setEnvValue(filterEnvValue);

    // Generate oscillator output
    Sample dcoOut = dco_.process();

    // Process through filter
    Sample filtered = filter_.process(dcoOut);

    // Apply amplitude envelope
    Sample output = filtered * ampEnvValue;

    return output;
}

void Voice::noteOn(int midiNote, int velocity) {
    currentNote_ = midiNote;
    currentFreq_ = midiNoteToFrequency(midiNote);
    isActive_ = true;

    // Set frequency for oscillator and filter
    dco_.setFrequency(currentFreq_);
    filter_.setNoteFrequency(currentFreq_);

    // Trigger envelopes and oscillator
    dco_.noteOn();
    filterEnv_.noteOn();
    ampEnv_.noteOn();
}

void Voice::noteOff() {
    isActive_ = false;

    // Release envelopes and oscillator
    dco_.noteOff();
    filterEnv_.noteOff();
    ampEnv_.noteOff();
}

void Voice::setDcoParams(const DcoParams& params) {
    dcoParams_ = params;
    dco_.setParameters(dcoParams_);
}

void Voice::setFilterParams(const FilterParams& params) {
    filterParams_ = params;
    filter_.setParameters(filterParams_);
}

void Voice::setFilterEnvParams(const EnvelopeParams& params) {
    filterEnvParams_ = params;
    filterEnv_.setParameters(filterEnvParams_);
}

void Voice::setAmpEnvParams(const EnvelopeParams& params) {
    ampEnvParams_ = params;
    ampEnv_.setParameters(ampEnvParams_);
}

void Voice::setFrequency(float freq) {
    currentFreq_ = freq;
    dco_.setFrequency(freq);
    filter_.setNoteFrequency(freq);
}

} // namespace phj
