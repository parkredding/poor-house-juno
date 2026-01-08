#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../../dsp/dco.h"
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
        , lfoPhase_(0.0f)
        , lfoRate_(2.0f)
    {
        dco_.setSampleRate(sampleRate);
        dco_.setFrequency(440.0f);

        // Default parameters - sawtooth wave
        params_.sawLevel = 0.5f;
        params_.pulseLevel = 0.0f;
        params_.subLevel = 0.0f;
        params_.noiseLevel = 0.0f;
        params_.pulseWidth = 0.5f;
        params_.pwmDepth = 0.0f;
        params_.lfoTarget = DcoParams::LFO_OFF;
        params_.detune = 0.0f;
        params_.enableDrift = true;

        dco_.setParameters(params_);
    }

    // Process audio (called from AudioWorklet)
    void process(uintptr_t leftPtr, uintptr_t rightPtr, int numSamples) {
        float* left = reinterpret_cast<float*>(leftPtr);
        float* right = reinterpret_cast<float*>(rightPtr);

        for (int i = 0; i < numSamples; ++i) {
            // Update LFO
            float lfoValue = std::sin(lfoPhase_ * TWO_PI);
            dco_.setLfoValue(lfoValue);
            lfoPhase_ += lfoRate_ / sampleRate_;
            if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;

            // Generate sample
            float sample = noteOn_ ? dco_.process() : 0.0f;
            left[i] = sample;
            right[i] = sample;
        }
    }

    // MIDI handling
    void handleMidi(int status, int data1, int data2) {
        uint8_t statusByte = status & 0xF0;

        if (statusByte == MIDI_NOTE_ON && data2 > 0) {
            float freq = midiNoteToFrequency(data1);
            dco_.setFrequency(freq);
            dco_.noteOn();
            noteOn_ = true;
        } else if (statusByte == MIDI_NOTE_OFF || (statusByte == MIDI_NOTE_ON && data2 == 0)) {
            dco_.noteOff();
            noteOn_ = false;
        }
    }

    // Parameter control - DCO
    void setSawLevel(float level) {
        params_.sawLevel = level;
        dco_.setParameters(params_);
    }

    void setPulseLevel(float level) {
        params_.pulseLevel = level;
        dco_.setParameters(params_);
    }

    void setSubLevel(float level) {
        params_.subLevel = level;
        dco_.setParameters(params_);
    }

    void setNoiseLevel(float level) {
        params_.noiseLevel = level;
        dco_.setParameters(params_);
    }

    void setPulseWidth(float width) {
        params_.pulseWidth = width;
        dco_.setParameters(params_);
    }

    void setPwmDepth(float depth) {
        params_.pwmDepth = depth;
        dco_.setParameters(params_);
    }

    void setLfoTarget(int target) {
        params_.lfoTarget = target;
        dco_.setParameters(params_);
    }

    void setLfoRate(float rate) {
        lfoRate_ = rate;
    }

    void setDetune(float cents) {
        params_.detune = cents;
        dco_.setParameters(params_);
    }

    void setDriftEnabled(bool enabled) {
        params_.enableDrift = enabled;
        dco_.setParameters(params_);
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
    DcoParams params_;
    bool noteOn_;

    // Simple LFO (will be replaced with proper LFO module later)
    float lfoPhase_;
    float lfoRate_;
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

        // Legacy
        .function("setFrequency", &WebSynth::setFrequency)
        .function("setNoteOn", &WebSynth::setNoteOn)
        ;
}
