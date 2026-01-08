#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../../dsp/oscillator.h"
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
    {
        osc_.setSampleRate(sampleRate);
        osc_.setFrequency(440.0f);
        osc_.setAmplitude(0.5f);
    }

    // Process audio (called from AudioWorklet)
    void process(uintptr_t leftPtr, uintptr_t rightPtr, int numSamples) {
        float* left = reinterpret_cast<float*>(leftPtr);
        float* right = reinterpret_cast<float*>(rightPtr);

        for (int i = 0; i < numSamples; ++i) {
            float sample = noteOn_ ? osc_.process() : 0.0f;
            left[i] = sample;
            right[i] = sample;
        }
    }

    // MIDI handling
    void handleMidi(int status, int data1, int data2) {
        uint8_t statusByte = status & 0xF0;

        if (statusByte == MIDI_NOTE_ON && data2 > 0) {
            float freq = midiNoteToFrequency(data1);
            osc_.setFrequency(freq);
            float amplitude = (data2 / 127.0f) * 0.5f;
            osc_.setAmplitude(amplitude);
            noteOn_ = true;
        } else if (statusByte == MIDI_NOTE_OFF || (statusByte == MIDI_NOTE_ON && data2 == 0)) {
            noteOn_ = false;
        }
    }

    // Parameter control
    void setFrequency(float freq) {
        osc_.setFrequency(freq);
    }

    void setAmplitude(float amp) {
        osc_.setAmplitude(amp);
    }

    void setNoteOn(bool on) {
        noteOn_ = on;
    }

private:
    float sampleRate_;
    SineOscillator osc_;
    bool noteOn_;
};

// Emscripten bindings
EMSCRIPTEN_BINDINGS(synth_module) {
    class_<WebSynth>("WebSynth")
        .constructor<float>()
        .function("process", &WebSynth::process)
        .function("handleMidi", &WebSynth::handleMidi)
        .function("setFrequency", &WebSynth::setFrequency)
        .function("setAmplitude", &WebSynth::setAmplitude)
        .function("setNoteOn", &WebSynth::setNoteOn)
        ;
}
