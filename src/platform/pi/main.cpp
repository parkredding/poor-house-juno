#include <iostream>
#include <csignal>
#include <atomic>
#include "audio_driver.h"
#include "midi_driver.h"
#include "../../dsp/dco.h"
#include "../../dsp/parameters.h"

using namespace phj;

// Global state for signal handler
static std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// Test synth state
struct TestSynth {
    Dco dco;
    DcoParams params;
    bool noteOn;
    float lfoPhase;
    float lfoRate;
};

static TestSynth g_synth;

// Audio callback
void audioCallback(float* left, float* right, int numSamples, void* userData) {
    TestSynth* synth = static_cast<TestSynth*>(userData);

    for (int i = 0; i < numSamples; ++i) {
        // Update LFO
        float lfoValue = std::sin(synth->lfoPhase * TWO_PI);
        synth->dco.setLfoValue(lfoValue);
        synth->lfoPhase += synth->lfoRate / 48000.0f;
        if (synth->lfoPhase >= 1.0f) synth->lfoPhase -= 1.0f;

        // Generate sample
        float sample = synth->noteOn ? synth->dco.process() : 0.0f;
        left[i] = sample;
        right[i] = sample;
    }
}

// MIDI callback
void midiCallback(const uint8_t* data, int length, void* userData) {
    TestSynth* synth = static_cast<TestSynth*>(userData);

    if (length < 3) return;

    uint8_t status = data[0] & 0xF0;
    uint8_t note = data[1];
    uint8_t velocity = data[2];

    if (status == MIDI_NOTE_ON && velocity > 0) {
        float freq = midiNoteToFrequency(note);
        synth->dco.setFrequency(freq);
        synth->dco.noteOn();
        synth->noteOn = true;
        std::cout << "Note ON: " << (int)note << " (" << freq << " Hz), vel=" << (int)velocity << std::endl;
    } else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && velocity == 0)) {
        synth->dco.noteOff();
        synth->noteOn = false;
        std::cout << "Note OFF: " << (int)note << std::endl;
    }
}

int main(int argc, char** argv) {
    std::cout << "Poor House Juno - Raspberry Pi Test" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize test synth
    g_synth.dco.setSampleRate(48000.0f);
    g_synth.dco.setFrequency(440.0f);
    g_synth.noteOn = false;
    g_synth.lfoPhase = 0.0f;
    g_synth.lfoRate = 2.0f;

    // Setup DCO parameters - sawtooth wave
    g_synth.params.sawLevel = 0.5f;
    g_synth.params.pulseLevel = 0.0f;
    g_synth.params.subLevel = 0.0f;
    g_synth.params.noiseLevel = 0.0f;
    g_synth.params.pulseWidth = 0.5f;
    g_synth.params.pwmDepth = 0.0f;
    g_synth.params.lfoTarget = DcoParams::LFO_OFF;
    g_synth.params.detune = 0.0f;
    g_synth.params.enableDrift = true;
    g_synth.dco.setParameters(g_synth.params);

    // Initialize audio driver
    AudioDriver audio;
    if (!audio.initialize("default", 48000, 128)) {
        std::cerr << "Failed to initialize audio" << std::endl;
        return 1;
    }

    audio.setCallback(audioCallback, &g_synth);

    if (!audio.start()) {
        std::cerr << "Failed to start audio" << std::endl;
        return 1;
    }

    // Initialize MIDI driver
    MidiDriver midi;
    if (!midi.initialize("default")) {
        std::cerr << "Failed to initialize MIDI (this is optional)" << std::endl;
        // Continue without MIDI
    } else {
        midi.setCallback(midiCallback, &g_synth);
        if (!midi.start()) {
            std::cerr << "Failed to start MIDI" << std::endl;
        }
    }

    std::cout << "\nAudio running at " << audio.getSampleRate() << " Hz" << std::endl;
    std::cout << "Buffer size: " << audio.getBufferSize() << " samples" << std::endl;
    std::cout << "\nPlaying 440 Hz sine wave. Send MIDI to control." << std::endl;
    std::cout << "Press Ctrl+C to exit.\n" << std::endl;

    // Test: play a note for a few seconds if no MIDI
    if (!midi.isRunning()) {
        std::cout << "No MIDI available, playing test tone for 3 seconds..." << std::endl;
        g_synth.noteOn = true;
        sleep(3);
        g_synth.noteOn = false;
        std::cout << "Test tone finished. Running idle (waiting for Ctrl+C)..." << std::endl;
    }

    // Main loop
    while (g_running) {
        sleep(1);
    }

    std::cout << "Shutting down..." << std::endl;

    midi.shutdown();
    audio.shutdown();

    std::cout << "Goodbye!" << std::endl;

    return 0;
}
