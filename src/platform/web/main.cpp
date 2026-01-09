#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../../dsp/synth.h"
#include "../../dsp/parameters.h"
#include "../../dsp/types.h"

using namespace emscripten;
using namespace phj;

/**
 * Web Audio synth processor
 * This will be instantiated from JavaScript and called from an AudioWorklet
 */
class WebSynth {
public:
    WebSynth(float sampleRate)
        : sampleRate_(sampleRate)
    {
        synth_.setSampleRate(sampleRate);

        // Default DCO parameters - sawtooth wave
        dcoParams_.sawLevel = 0.5f;
        dcoParams_.pulseLevel = 0.0f;
        dcoParams_.subLevel = 0.0f;
        dcoParams_.noiseLevel = 0.0f;
        dcoParams_.pulseWidth = 0.5f;
        dcoParams_.pwmDepth = 0.0f;
        dcoParams_.lfoTarget = DcoParams::LFO_OFF;
        dcoParams_.detune = 0.0f;
        dcoParams_.enableDrift = true;
        synth_.setDcoParameters(dcoParams_);

        // Default filter parameters
        filterParams_.cutoff = 0.8f;  // Start fairly open
        filterParams_.resonance = 0.0f;
        filterParams_.envAmount = 0.0f;
        filterParams_.lfoAmount = 0.0f;
        filterParams_.keyTrack = FilterParams::KEY_TRACK_OFF;
        filterParams_.drive = 1.0f;
        filterParams_.hpfMode = 0;  // M11: HPF off by default
        synth_.setFilterParameters(filterParams_);

        // Default filter envelope parameters
        filterEnvParams_.attack = 0.01f;
        filterEnvParams_.decay = 0.3f;
        filterEnvParams_.sustain = 0.7f;
        filterEnvParams_.release = 0.5f;
        synth_.setFilterEnvParameters(filterEnvParams_);

        // Default amplitude envelope parameters
        ampEnvParams_.attack = 0.005f;   // Fast attack for plucky sounds
        ampEnvParams_.decay = 0.3f;
        ampEnvParams_.sustain = 0.8f;
        ampEnvParams_.release = 0.3f;
        synth_.setAmpEnvParameters(ampEnvParams_);

        // Default LFO parameters
        lfoParams_.rate = 2.0f;
        lfoParams_.delay = 0.0f;  // M12
        synth_.setLfoParameters(lfoParams_);

        // Default chorus parameters (off by default)
        chorusParams_.mode = 0;  // OFF
        synth_.setChorusParameters(chorusParams_);

        // M11/M13/M14: Default performance parameters
        performanceParams_.pitchBend = 0.0f;
        performanceParams_.pitchBendRange = 2.0f;
        performanceParams_.portamentoTime = 0.0f;
        performanceParams_.modWheel = 1.0f;  // M13: Default to full modulation
        performanceParams_.vcaMode = PerformanceParams::VCA_ENV;  // M13: Default to envelope mode
        performanceParams_.filterEnvPolarity = PerformanceParams::FILTER_ENV_NORMAL;  // M13: Default to normal
        performanceParams_.vcaLevel = 0.8f;  // M14: Default VCA level
        performanceParams_.masterTune = 0.0f;  // M14: Default to no tuning offset
        performanceParams_.velocityToFilter = 0.0f;  // M14: Default to no velocity sensitivity
        performanceParams_.velocityToAmp = 1.0f;  // M14: Default to full velocity to amp
        synth_.setPerformanceParameters(performanceParams_);
    }

    // Process audio (called from AudioWorklet)
    void process(uintptr_t leftPtr, uintptr_t rightPtr, int numSamples) {
        float* left = reinterpret_cast<float*>(leftPtr);
        float* right = reinterpret_cast<float*>(rightPtr);

        // Process stereo output with chorus
        synth_.processStereo(left, right, numSamples);
    }

    // MIDI handling
    void handleMidi(int status, int data1, int data2) {
        uint8_t statusByte = status & 0xF0;

        if (statusByte == MIDI_NOTE_ON && data2 > 0) {
            float velocity = data2 / 127.0f;
            synth_.handleNoteOn(data1, velocity);
        } else if (statusByte == MIDI_NOTE_OFF || (statusByte == MIDI_NOTE_ON && data2 == 0)) {
            synth_.handleNoteOff(data1);
        } else if (statusByte == MIDI_PITCH_BEND) {
            // M11: Pitch bend - combine data1 (LSB) and data2 (MSB) into 14-bit value
            int bendValue = data1 | (data2 << 7);
            // Convert from 0-16383 to -1.0 to 1.0
            float bendNormalized = (bendValue - 8192) / 8192.0f;
            synth_.handlePitchBend(bendNormalized);
        } else if (statusByte == MIDI_CONTROL_CHANGE) {
            // M13: Handle MIDI CC messages
            if (data1 == 1) {  // CC #1: Modulation Wheel
                float modWheelValue = data2 / 127.0f;
                synth_.handleModWheel(modWheelValue);
            }
        }
    }

    // Parameter control - DCO
    void setSawLevel(float level) {
        dcoParams_.sawLevel = level;
        synth_.setDcoParameters(dcoParams_);
    }

    void setPulseLevel(float level) {
        dcoParams_.pulseLevel = level;
        synth_.setDcoParameters(dcoParams_);
    }

    void setSubLevel(float level) {
        dcoParams_.subLevel = level;
        synth_.setDcoParameters(dcoParams_);
    }

    void setNoiseLevel(float level) {
        dcoParams_.noiseLevel = level;
        synth_.setDcoParameters(dcoParams_);
    }

    void setPulseWidth(float width) {
        dcoParams_.pulseWidth = width;
        synth_.setDcoParameters(dcoParams_);
    }

    void setPwmDepth(float depth) {
        dcoParams_.pwmDepth = depth;
        synth_.setDcoParameters(dcoParams_);
    }

    void setLfoTarget(int target) {
        dcoParams_.lfoTarget = target;
        synth_.setDcoParameters(dcoParams_);
    }

    void setLfoRate(float rate) {
        lfoParams_.rate = rate;
        synth_.setLfoParameters(lfoParams_);
    }

    void setLfoDelay(float delay) {
        lfoParams_.delay = delay;
        synth_.setLfoParameters(lfoParams_);
    }

    void setDetune(float cents) {
        dcoParams_.detune = cents;
        synth_.setDcoParameters(dcoParams_);
    }

    void setDriftEnabled(bool enabled) {
        dcoParams_.enableDrift = enabled;
        synth_.setDcoParameters(dcoParams_);
    }

    // Parameter control - Filter
    void setFilterCutoff(float cutoff) {
        filterParams_.cutoff = cutoff;
        synth_.setFilterParameters(filterParams_);
    }

    void setFilterResonance(float resonance) {
        filterParams_.resonance = resonance;
        synth_.setFilterParameters(filterParams_);
    }

    void setFilterEnvAmount(float amount) {
        filterParams_.envAmount = amount;
        synth_.setFilterParameters(filterParams_);
    }

    void setFilterLfoAmount(float amount) {
        filterParams_.lfoAmount = amount;
        synth_.setFilterParameters(filterParams_);
    }

    void setFilterKeyTrack(int mode) {
        filterParams_.keyTrack = mode;
        synth_.setFilterParameters(filterParams_);
    }

    // M11: HPF
    void setFilterHpfMode(int mode) {
        filterParams_.hpfMode = mode;
        synth_.setFilterParameters(filterParams_);
    }

    // Parameter control - Filter Envelope
    void setFilterEnvAttack(float attack) {
        filterEnvParams_.attack = attack;
        synth_.setFilterEnvParameters(filterEnvParams_);
    }

    void setFilterEnvDecay(float decay) {
        filterEnvParams_.decay = decay;
        synth_.setFilterEnvParameters(filterEnvParams_);
    }

    void setFilterEnvSustain(float sustain) {
        filterEnvParams_.sustain = sustain;
        synth_.setFilterEnvParameters(filterEnvParams_);
    }

    void setFilterEnvRelease(float release) {
        filterEnvParams_.release = release;
        synth_.setFilterEnvParameters(filterEnvParams_);
    }

    // Parameter control - Amplitude Envelope
    void setAmpEnvAttack(float attack) {
        ampEnvParams_.attack = attack;
        synth_.setAmpEnvParameters(ampEnvParams_);
    }

    void setAmpEnvDecay(float decay) {
        ampEnvParams_.decay = decay;
        synth_.setAmpEnvParameters(ampEnvParams_);
    }

    void setAmpEnvSustain(float sustain) {
        ampEnvParams_.sustain = sustain;
        synth_.setAmpEnvParameters(ampEnvParams_);
    }

    void setAmpEnvRelease(float release) {
        ampEnvParams_.release = release;
        synth_.setAmpEnvParameters(ampEnvParams_);
    }

    // Parameter control - Chorus
    void setChorusMode(int mode) {
        chorusParams_.mode = mode;
        synth_.setChorusParameters(chorusParams_);
    }

    // M11: Performance parameters
    void setPitchBendRange(float semitones) {
        performanceParams_.pitchBendRange = semitones;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setPortamentoTime(float seconds) {
        performanceParams_.portamentoTime = seconds;
        synth_.setPerformanceParameters(performanceParams_);
    }

    // M13: Performance parameters
    void setModWheel(float value) {
        performanceParams_.modWheel = value;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setVcaMode(int mode) {
        performanceParams_.vcaMode = mode;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setFilterEnvPolarity(int polarity) {
        performanceParams_.filterEnvPolarity = polarity;
        synth_.setPerformanceParameters(performanceParams_);
    }

    // M14: Range & Voice Control
    void setDcoRange(int range) {
        dcoParams_.range = range;
        synth_.setDcoParameters(dcoParams_);
    }

    void setVcaLevel(float level) {
        performanceParams_.vcaLevel = level;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setMasterTune(float cents) {
        performanceParams_.masterTune = cents;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setVelocityToFilter(float amount) {
        performanceParams_.velocityToFilter = amount;
        synth_.setPerformanceParameters(performanceParams_);
    }

    void setVelocityToAmp(float amount) {
        performanceParams_.velocityToAmp = amount;
        synth_.setPerformanceParameters(performanceParams_);
    }

    // Legacy interface for compatibility (deprecated, but kept for backward compat)
    void setFrequency(float freq) {
        // No-op in new architecture - use handleMidi instead
    }

    void setNoteOn(bool on) {
        // No-op in new architecture - use handleMidi instead
    }

private:
    float sampleRate_;
    Synth synth_;

    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;
    LfoParams lfoParams_;
    ChorusParams chorusParams_;
    PerformanceParams performanceParams_;  // M11
};

// Emscripten bindings
EMSCRIPTEN_BINDINGS(synth_module) {
    class_<WebSynth>("WebSynth")
        .constructor<float>()
        .function("process", &WebSynth::process)
        .function("handleMidi", &WebSynth::handleMidi)

        // DCO parameters
        .function("setSawLevel", &WebSynth::setSawLevel)
        .function("setPulseLevel", &WebSynth::setPulseLevel)
        .function("setSubLevel", &WebSynth::setSubLevel)
        .function("setNoiseLevel", &WebSynth::setNoiseLevel)
        .function("setPulseWidth", &WebSynth::setPulseWidth)
        .function("setPwmDepth", &WebSynth::setPwmDepth)
        .function("setLfoTarget", &WebSynth::setLfoTarget)
        .function("setLfoRate", &WebSynth::setLfoRate)
        .function("setLfoDelay", &WebSynth::setLfoDelay)
        .function("setDetune", &WebSynth::setDetune)
        .function("setDriftEnabled", &WebSynth::setDriftEnabled)

        // Filter parameters
        .function("setFilterCutoff", &WebSynth::setFilterCutoff)
        .function("setFilterResonance", &WebSynth::setFilterResonance)
        .function("setFilterEnvAmount", &WebSynth::setFilterEnvAmount)
        .function("setFilterLfoAmount", &WebSynth::setFilterLfoAmount)
        .function("setFilterKeyTrack", &WebSynth::setFilterKeyTrack)
        .function("setFilterHpfMode", &WebSynth::setFilterHpfMode)  // M11

        // Filter envelope parameters
        .function("setFilterEnvAttack", &WebSynth::setFilterEnvAttack)
        .function("setFilterEnvDecay", &WebSynth::setFilterEnvDecay)
        .function("setFilterEnvSustain", &WebSynth::setFilterEnvSustain)
        .function("setFilterEnvRelease", &WebSynth::setFilterEnvRelease)

        // Amplitude envelope parameters
        .function("setAmpEnvAttack", &WebSynth::setAmpEnvAttack)
        .function("setAmpEnvDecay", &WebSynth::setAmpEnvDecay)
        .function("setAmpEnvSustain", &WebSynth::setAmpEnvSustain)
        .function("setAmpEnvRelease", &WebSynth::setAmpEnvRelease)

        // Chorus parameters
        .function("setChorusMode", &WebSynth::setChorusMode)

        // M11: Performance parameters
        .function("setPitchBendRange", &WebSynth::setPitchBendRange)
        .function("setPortamentoTime", &WebSynth::setPortamentoTime)

        // M13: Performance parameters
        .function("setModWheel", &WebSynth::setModWheel)
        .function("setVcaMode", &WebSynth::setVcaMode)
        .function("setFilterEnvPolarity", &WebSynth::setFilterEnvPolarity)

        // M14: Range & Voice Control
        .function("setDcoRange", &WebSynth::setDcoRange)
        .function("setVcaLevel", &WebSynth::setVcaLevel)
        .function("setMasterTune", &WebSynth::setMasterTune)
        .function("setVelocityToFilter", &WebSynth::setVelocityToFilter)
        .function("setVelocityToAmp", &WebSynth::setVelocityToAmp)

        // Legacy
        .function("setFrequency", &WebSynth::setFrequency)
        .function("setNoteOn", &WebSynth::setNoteOn)
        ;
}
