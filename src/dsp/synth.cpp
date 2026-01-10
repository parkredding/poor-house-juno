#include "synth.h"

namespace phj {

Synth::Synth()
    : sampleRate_(SAMPLE_RATE)
{
    lfo_.setSampleRate(sampleRate_);
    chorus_.setSampleRate(sampleRate_);

    // Initialize all voices
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setSampleRate(sampleRate_);
        voices_[i].setParameters(dcoParams_, filterParams_, filterEnvParams_, ampEnvParams_);
    }

    // Set default LFO parameters
    lfo_.setRate(lfoParams_.rate);
    lfo_.setDelay(lfoParams_.delay);  // M12

    // Set default chorus mode
    chorus_.setMode(static_cast<Chorus::Mode>(chorusParams_.mode));
}

void Synth::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    lfo_.setSampleRate(sampleRate);
    chorus_.setSampleRate(sampleRate);

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
    lfo_.setDelay(lfoParams_.delay);  // M12
}

void Synth::setChorusParameters(const ChorusParams& params) {
    chorusParams_ = params;
    chorus_.setMode(static_cast<Chorus::Mode>(chorusParams_.mode));
}

void Synth::setPerformanceParameters(const PerformanceParams& params) {
    performanceParams_ = params;
    // Update all voices with new performance parameters
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setPitchBend(performanceParams_.pitchBend, performanceParams_.pitchBendRange);
        voices_[i].setPortamentoTime(performanceParams_.portamentoTime);
        // M13: Update VCA mode and filter envelope polarity
        voices_[i].setVcaMode(performanceParams_.vcaMode);
        voices_[i].setFilterEnvPolarity(performanceParams_.filterEnvPolarity);
        // M14: Update VCA level, velocity sensitivity, and master tune
        voices_[i].setVcaLevel(performanceParams_.vcaLevel);
        voices_[i].setVelocitySensitivity(performanceParams_.velocityToFilter, performanceParams_.velocityToAmp);
        voices_[i].setMasterTune(performanceParams_.masterTune);
    }
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
        // M12: Trigger LFO delay timer on note-on
        lfo_.trigger();
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

void Synth::handlePitchBend(float pitchBend) {
    performanceParams_.pitchBend = clamp(pitchBend, -1.0f, 1.0f);
    // Update all voices with new pitch bend value
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].setPitchBend(performanceParams_.pitchBend, performanceParams_.pitchBendRange);
    }
}

void Synth::handleModWheel(float modWheel) {
    // M13: Modulation wheel controls LFO depth (MIDI CC #1)
    performanceParams_.modWheel = clamp(modWheel, 0.0f, 1.0f);
}

void Synth::handleControlChange(int controller, int value) {
    // M16: Generic MIDI CC handler for Arturia MiniLab and other controllers
    // Convert MIDI value (0-127) to normalized 0.0-1.0
    float normalized = clamp(value / 127.0f, 0.0f, 1.0f);

    switch (controller) {
        case 1:  // Mod Wheel (also handled by handleModWheel)
            performanceParams_.modWheel = normalized;
            break;

        case 64:  // Sustain Pedal
            handleSustainPedal(value >= 64);  // Threshold at 64 for on/off
            break;

        case 71:  // Filter Resonance
            filterParams_.resonance = normalized;
            setFilterParameters(filterParams_);
            break;

        case 73:  // Filter Env Amount (bipolar: -1.0 to 1.0)
            filterParams_.envAmount = (normalized * 2.0f) - 1.0f;
            setFilterParameters(filterParams_);
            break;

        case 74:  // Filter Cutoff (primary control)
            filterParams_.cutoff = normalized;
            setFilterParameters(filterParams_);
            break;

        case 75: {  // LFO Rate (0.1 - 30.0 Hz, exponential scaling)
            // Exponential mapping for musical control
            float minRate = 0.1f;
            float maxRate = 30.0f;
            lfoParams_.rate = minRate * std::pow(maxRate / minRate, normalized);
            setLfoParameters(lfoParams_);
            break;
        }

        case 76: {  // LFO Delay (0.0 - 3.0 seconds)
            lfoParams_.delay = normalized * 3.0f;
            setLfoParameters(lfoParams_);
            break;
        }

        case 77:  // DCO Pulse Width (0.05 - 0.95)
            dcoParams_.pulseWidth = 0.05f + (normalized * 0.9f);
            setDcoParameters(dcoParams_);
            break;

        case 78:  // DCO PWM Depth (0.0 - 1.0)
            dcoParams_.pwmDepth = normalized;
            setDcoParameters(dcoParams_);
            break;

        case 79:  // Filter Env Attack (0.001 - 3.0 seconds, exponential)
            filterEnvParams_.attack = 0.001f * std::pow(3000.0f, normalized);
            setFilterEnvParameters(filterEnvParams_);
            break;

        case 80:  // Filter Env Decay (0.002 - 12.0 seconds, exponential)
            filterEnvParams_.decay = 0.002f * std::pow(6000.0f, normalized);
            setFilterEnvParameters(filterEnvParams_);
            break;

        case 81:  // Filter Env Sustain (0.0 - 1.0, linear)
            filterEnvParams_.sustain = normalized;
            setFilterEnvParameters(filterEnvParams_);
            break;

        case 82:  // Filter Env Release (0.002 - 12.0 seconds, exponential)
            filterEnvParams_.release = 0.002f * std::pow(6000.0f, normalized);
            setFilterEnvParameters(filterEnvParams_);
            break;

        case 83:  // Amp Env Attack (0.001 - 3.0 seconds, exponential)
            ampEnvParams_.attack = 0.001f * std::pow(3000.0f, normalized);
            setAmpEnvParameters(ampEnvParams_);
            break;

        case 84:  // Amp Env Decay (0.002 - 12.0 seconds, exponential)
            ampEnvParams_.decay = 0.002f * std::pow(6000.0f, normalized);
            setAmpEnvParameters(ampEnvParams_);
            break;

        case 85:  // Amp Env Sustain (0.0 - 1.0, linear)
            ampEnvParams_.sustain = normalized;
            setAmpEnvParameters(ampEnvParams_);
            break;

        case 86:  // Amp Env Release (0.002 - 12.0 seconds, exponential)
            ampEnvParams_.release = 0.002f * std::pow(6000.0f, normalized);
            setAmpEnvParameters(ampEnvParams_);
            break;

        case 91:  // Chorus Mode (0-3 mapped to 4 discrete values)
            chorusParams_.mode = static_cast<int>(normalized * 3.99f);  // 0, 1, 2, or 3
            setChorusParameters(chorusParams_);
            break;

        case 102: {  // Portamento Time (0.0 - 10.0 seconds, exponential)
            performanceParams_.portamentoTime = normalized * normalized * 10.0f;
            setPerformanceParameters(performanceParams_);
            break;
        }

        case 103: {  // Pitch Bend Range (0 - 12 semitones)
            performanceParams_.pitchBendRange = normalized * 12.0f;
            setPerformanceParameters(performanceParams_);
            break;
        }

        // Add more CC mappings as needed
        default:
            // Unhandled CC - ignore silently
            break;
    }
}

void Synth::handleSustainPedal(bool sustain) {
    // M16: Sustain pedal handling (CC #64)
    // Basic implementation - stores state for future use
    // Full sustain logic will be implemented in Voice class later
    performanceParams_.sustainPedal = sustain;

    // TODO: Implement full sustain logic in Voice class
    // - When sustain is on, prevent note-off from releasing voices
    // - When sustain is released, release all sustained voices
}

void Synth::processStereo(Sample& leftOut, Sample& rightOut) {
    // Update global LFO (shared by all voices)
    float lfoValue = lfo_.process();

    // M13: Scale LFO by modulation wheel (0.0 - 1.0)
    float modulatedLfo = lfoValue * performanceParams_.modWheel;

    // Mix all voices
    Sample mixedVoices = 0.0f;

    for (int i = 0; i < NUM_VOICES; ++i) {
        // Update voice with LFO value (scaled by mod wheel)
        voices_[i].setLfoValue(modulatedLfo);

        // Process and accumulate
        mixedVoices += voices_[i].process();
    }

    // Scale output to prevent clipping with multiple voices
    // Using 1/sqrt(NUM_VOICES) gives good headroom while maintaining loudness
    constexpr float voiceScale = 1.0f / 2.45f;  // sqrt(6) ≈ 2.45
    mixedVoices *= voiceScale;

    // Process through chorus (converts mono to stereo)
    chorus_.process(mixedVoices, leftOut, rightOut);
}

Sample Synth::process() {
    // Process stereo and mix down to mono
    Sample left, right;
    processStereo(left, right);
    return (left + right) * 0.5f;  // Average L and R channels
}

void Synth::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

void Synth::processStereo(Sample* leftOutput, Sample* rightOutput, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        processStereo(leftOutput[i], rightOutput[i]);
    }
}

void Synth::reset() {
    lfo_.reset();
    chorus_.reset();

    for (int i = 0; i < NUM_VOICES; ++i) {
        voices_[i].reset();
    }
}

} // namespace phj
