#include "synth.h"

namespace phj {

Synth::Synth()
    : sampleRate_(SAMPLE_RATE)
{
    lfo_.setSampleRate(sampleRate_);
    voice_.setSampleRate(sampleRate_);

    // Set default parameters
    lfo_.setRate(lfoParams_.rate);
    voice_.setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
}

void Synth::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    lfo_.setSampleRate(sampleRate);
    voice_.setSampleRate(sampleRate);
}

void Synth::setDcoParameters(const DcoParams& params) {
    dcoParams_ = params;
    voice_.setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
}

void Synth::setFilterParameters(const FilterParams& params) {
    filterParams_ = params;
    voice_.setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
}

void Synth::setFilterEnvParameters(const EnvelopeParams& params) {
    filterEnvParams_ = params;
    voice_.setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
}

void Synth::setAmpEnvParameters(const EnvelopeParams& params) {
    ampEnvParams_ = params;
    voice_.setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
}

void Synth::setLfoParameters(const LfoParams& params) {
    lfoParams_ = params;
    lfo_.setRate(lfoParams_.rate);
}

void Synth::handleNoteOn(int midiNote, float velocity) {
    // For M6 (single voice): always use the single voice
    // For M7 (polyphony): will implement voice allocation/stealing
    voice_.noteOn(midiNote, velocity);
}

void Synth::handleNoteOff(int midiNote) {
    // For M6: release the voice if it's playing this note
    if (voice_.getCurrentNote() == midiNote) {
        voice_.noteOff();
    }
    // For M7: will check all voices and release matching ones
}

void Synth::allNotesOff() {
    voice_.noteOff();
    // For M7: will iterate all voices
}

Sample Synth::process() {
    // Update global LFO
    float lfoValue = lfo_.process();

    // Update voice with LFO value
    voice_.setLfoValue(lfoValue);

    // Process voice
    Sample output = voice_.process();

    return output;
}

void Synth::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

void Synth::reset() {
    lfo_.reset();
    voice_.reset();
}

} // namespace phj
