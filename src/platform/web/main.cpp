#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../../dsp/dco.h"
#include "../../dsp/filter.h"
#include "../../dsp/envelope.h"
#include "../../dsp/lfo.h"
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
        , noteOn_(false)
        , currentFreq_(440.0f)
    {
        dco_.setSampleRate(sampleRate);
        dco_.setFrequency(440.0f);
        filter_.setSampleRate(sampleRate);
        filterEnv_.setSampleRate(sampleRate);
        ampEnv_.setSampleRate(sampleRate);
        lfo_.setSampleRate(sampleRate);

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
        dco_.setParameters(dcoParams_);

        // Default filter parameters
        filterParams_.cutoff = 0.8f;  // Start fairly open
        filterParams_.resonance = 0.0f;
        filterParams_.envAmount = 0.0f;
        filterParams_.lfoAmount = 0.0f;
        filterParams_.keyTrack = FilterParams::KEY_TRACK_OFF;
        filterParams_.drive = 1.0f;
        filter_.setParameters(filterParams_);

        // Default filter envelope parameters
        filterEnvParams_.attack = 0.01f;
        filterEnvParams_.decay = 0.3f;
        filterEnvParams_.sustain = 0.7f;
        filterEnvParams_.release = 0.5f;
        filterEnv_.setParameters(filterEnvParams_);

        // Default amplitude envelope parameters
        ampEnvParams_.attack = 0.005f;   // Fast attack for plucky sounds
        ampEnvParams_.decay = 0.3f;
        ampEnvParams_.sustain = 0.8f;
        ampEnvParams_.release = 0.3f;
        ampEnv_.setParameters(ampEnvParams_);

        // Default LFO parameters
        lfoParams_.rate = 2.0f;
        lfo_.setRate(lfoParams_.rate);
    }

    // Process audio (called from AudioWorklet)
    void process(uintptr_t leftPtr, uintptr_t rightPtr, int numSamples) {
        float* left = reinterpret_cast<float*>(leftPtr);
        float* right = reinterpret_cast<float*>(rightPtr);

        for (int i = 0; i < numSamples; ++i) {
            // Update LFO
            float lfoValue = lfo_.process();
            dco_.setLfoValue(lfoValue);
            filter_.setLfoValue(lfoValue);

            // Update envelopes
            float filterEnvValue = filterEnv_.process();
            float ampEnvValue = ampEnv_.process();
            filter_.setEnvValue(filterEnvValue);

            // Generate DCO sample
            float dcoOut = dco_.process();

            // Process through filter
            float filtered = filter_.process(dcoOut);

            // Apply amplitude envelope
            float sample = filtered * ampEnvValue;

            left[i] = sample;
            right[i] = sample;
        }
    }

    // MIDI handling
    void handleMidi(int status, int data1, int data2) {
        uint8_t statusByte = status & 0xF0;

        if (statusByte == MIDI_NOTE_ON && data2 > 0) {
            currentFreq_ = midiNoteToFrequency(data1);
            dco_.setFrequency(currentFreq_);
            dco_.noteOn();
            filter_.setNoteFrequency(currentFreq_);
            filterEnv_.noteOn();
            ampEnv_.noteOn();
            noteOn_ = true;
        } else if (statusByte == MIDI_NOTE_OFF || (statusByte == MIDI_NOTE_ON && data2 == 0)) {
            dco_.noteOff();
            filterEnv_.noteOff();
            ampEnv_.noteOff();
            noteOn_ = false;
        }
    }

    // Parameter control - DCO
    void setSawLevel(float level) {
        dcoParams_.sawLevel = level;
        dco_.setParameters(dcoParams_);
    }

    void setPulseLevel(float level) {
        dcoParams_.pulseLevel = level;
        dco_.setParameters(dcoParams_);
    }

    void setSubLevel(float level) {
        dcoParams_.subLevel = level;
        dco_.setParameters(dcoParams_);
    }

    void setNoiseLevel(float level) {
        dcoParams_.noiseLevel = level;
        dco_.setParameters(dcoParams_);
    }

    void setPulseWidth(float width) {
        dcoParams_.pulseWidth = width;
        dco_.setParameters(dcoParams_);
    }

    void setPwmDepth(float depth) {
        dcoParams_.pwmDepth = depth;
        dco_.setParameters(dcoParams_);
    }

    void setLfoTarget(int target) {
        dcoParams_.lfoTarget = target;
        dco_.setParameters(dcoParams_);
    }

    void setLfoRate(float rate) {
        lfoParams_.rate = rate;
        lfo_.setRate(rate);
    }

    void setDetune(float cents) {
        dcoParams_.detune = cents;
        dco_.setParameters(dcoParams_);
    }

    void setDriftEnabled(bool enabled) {
        dcoParams_.enableDrift = enabled;
        dco_.setParameters(dcoParams_);
    }

    // Parameter control - Filter
    void setFilterCutoff(float cutoff) {
        filterParams_.cutoff = cutoff;
        filter_.setParameters(filterParams_);
    }

    void setFilterResonance(float resonance) {
        filterParams_.resonance = resonance;
        filter_.setParameters(filterParams_);
    }

    void setFilterEnvAmount(float amount) {
        filterParams_.envAmount = amount;
        filter_.setParameters(filterParams_);
    }

    void setFilterLfoAmount(float amount) {
        filterParams_.lfoAmount = amount;
        filter_.setParameters(filterParams_);
    }

    void setFilterKeyTrack(int mode) {
        filterParams_.keyTrack = mode;
        filter_.setParameters(filterParams_);
    }

    // Parameter control - Filter Envelope
    void setFilterEnvAttack(float attack) {
        filterEnvParams_.attack = attack;
        filterEnv_.setParameters(filterEnvParams_);
    }

    void setFilterEnvDecay(float decay) {
        filterEnvParams_.decay = decay;
        filterEnv_.setParameters(filterEnvParams_);
    }

    void setFilterEnvSustain(float sustain) {
        filterEnvParams_.sustain = sustain;
        filterEnv_.setParameters(filterEnvParams_);
    }

    void setFilterEnvRelease(float release) {
        filterEnvParams_.release = release;
        filterEnv_.setParameters(filterEnvParams_);
    }

    // Parameter control - Amplitude Envelope
    void setAmpEnvAttack(float attack) {
        ampEnvParams_.attack = attack;
        ampEnv_.setParameters(ampEnvParams_);
    }

    void setAmpEnvDecay(float decay) {
        ampEnvParams_.decay = decay;
        ampEnv_.setParameters(ampEnvParams_);
    }

    void setAmpEnvSustain(float sustain) {
        ampEnvParams_.sustain = sustain;
        ampEnv_.setParameters(ampEnvParams_);
    }

    void setAmpEnvRelease(float release) {
        ampEnvParams_.release = release;
        ampEnv_.setParameters(ampEnvParams_);
    }

    // Legacy interface for compatibility
    void setFrequency(float freq) {
        dco_.setFrequency(freq);
    }

    void setNoteOn(bool on) {
        if (on && !noteOn_) {
            dco_.noteOn();
        } else if (!on && noteOn_) {
            dco_.noteOff();
        }
        noteOn_ = on;
    }

private:
    float sampleRate_;
    Dco dco_;
    Filter filter_;
    Envelope filterEnv_;
    Envelope ampEnv_;
    Lfo lfo_;

    DcoParams dcoParams_;
    FilterParams filterParams_;
    EnvelopeParams filterEnvParams_;
    EnvelopeParams ampEnvParams_;
    LfoParams lfoParams_;

    bool noteOn_;
    float currentFreq_;
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
        .function("setDetune", &WebSynth::setDetune)
        .function("setDriftEnabled", &WebSynth::setDriftEnabled)

        // Filter parameters
        .function("setFilterCutoff", &WebSynth::setFilterCutoff)
        .function("setFilterResonance", &WebSynth::setFilterResonance)
        .function("setFilterEnvAmount", &WebSynth::setFilterEnvAmount)
        .function("setFilterLfoAmount", &WebSynth::setFilterLfoAmount)
        .function("setFilterKeyTrack", &WebSynth::setFilterKeyTrack)

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

        // Legacy
        .function("setFrequency", &WebSynth::setFrequency)
        .function("setNoteOn", &WebSynth::setNoteOn)
        ;
}
