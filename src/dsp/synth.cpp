#include "synth.h"

namespace phj {

Synth::Synth()
    : sampleRate_(SAMPLE_RATE)
{
    lfo_.setSampleRate(sampleRate_);

    // Initialize all voices
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setSampleRate(sampleRate_);
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }

    // Set default LFO parameters
    lfo_.setRate(lfoParams_.rate);
}

void Synth::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    lfo_.setSampleRate(sampleRate);

    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setSampleRate(sampleRate);
    }
}

void Synth::setDcoParameters(const DcoParams& params) {
    dcoParams_ = params;
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }
}

void Synth::setFilterParameters(const FilterParams& params) {
    filterParams_ = params;
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }
}

void Synth::setFilterEnvParameters(const EnvelopeParams& params) {
    filterEnvParams_ = params;
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }
}

void Synth::setAmpEnvParameters(const EnvelopeParams& params) {
    ampEnvParams_ = params;
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }
}

void Synth::setLfoParameters(const LfoParams& params) {
    lfoParams_ = params;
    lfo_.setRate(lfoParams_.rate);
}

int Synth::findFreeVoice() const {
    // First pass: find an inactive voice
    for (int i = 0; i < NUM_VOICES; ++i) {
        if (!voices_[i].isActive()) {
            return i;
        }
    }

    // No free voice found
    return -1;
}

int Synth::findVoiceToSteal() const {
    int oldestReleasingVoice = -1;
    float oldestReleasingAge = -1.0f;

    int oldestVoice = -1;
    float oldestAge = -1.0f;

    // Find oldest releasing voice, or oldest active voice
    for (int i = 0; i < NUM_VOICES; ++i) {
        float age = voices_[i].getAge();

        if (voices_[i].isReleasing()) {
            // Prefer releasing voices
            if (age > oldestReleasingAge) {
                oldestReleasingAge = age;
                oldestReleasingVoice = i;
            }
        } else if (voices_[i].isActive()) {
            // Track oldest active voice as fallback
            if (age > oldestAge) {
                oldestAge = age;
                oldestVoice = i;
            }
        }
    }

    // Prefer stealing releasing voices, fall back to oldest active
    if (oldestReleasingVoice != -1) {
        return oldestReleasingVoice;
    }

    return oldestVoice;
}

void Synth::handleNoteOn(int midiNote, float velocity) {
    // Find a voice to use
    int voiceIndex = findFreeVoice();

    if (voiceIndex == -1) {
        // No free voice, need to steal one
        voiceIndex = findVoiceToSteal();
    }

    // If we found a voice, trigger it
    if (voiceIndex != -1) {
        voices_[voiceIndex].noteOn(midiNote, velocity);
    }
}

void Synth::handleNoteOff(int midiNote) {
    // Release all voices playing this note
    for (int i = 0; i < NUM_VOICES; ++i) {
        if (voices_[i].getCurrentNote() == midiNote) {
            voices_[i].noteOff();
        }
    }
}

void Synth::allNotesOff() {
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].noteOff();
    }
}

Sample Synth::process() {
    // Update global LFO (shared by all voices)
    float lfoValue = lfo_.process();

    // Mix all voices
    Sample output = 0.0f;

    for (int i = 0; i < NUM_VOICES; ++i) {
        // Update voice with LFO value
        voices_[i].setLfoValue(lfoValue);

        // Process and accumulate
        output += voices_[i].process();
    }

    // Scale output to prevent clipping with multiple voices
    // Using 1/sqrt(NUM_VOICES) gives good headroom while maintaining loudness
    constexpr float voiceScale = 1.0f / 2.45f;  // sqrt(6) ≈ 2.45
    output *= voiceScale;

    return output;
}

void Synth::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

void Synth::reset() {
    lfo_.reset();

    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].reset();
    }
}

} // namespace phj
