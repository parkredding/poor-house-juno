#include <iostream>
#include <csignal>
#include <atomic>
#include <cstring>
#include <getopt.h>
#include <unistd.h>
#include <chrono>
#include "audio_driver.h"
#include "midi_driver.h"
#include "../../dsp/synth.h"

using namespace phj;

// Global state for signal handler
static std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// Global synth instance
static Synth g_synth;

// CPU usage tracking
struct CpuMonitor {
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<uint64_t> totalSamples{0};
    std::atomic<uint64_t> totalProcessingTimeUs{0};
    float sampleRate;

    CpuMonitor() : sampleRate(48000.0f) {}

    void setSampleRate(float sr) {
        sampleRate = sr;
    }

    void update(uint64_t processingTimeUs, int numSamples) {
        totalProcessingTimeUs += processingTimeUs;
        totalSamples += numSamples;

        // Calculate CPU usage based on processing time versus real-time
        if (totalSamples >= sampleRate) {  // Update every ~1 second
            // Time available for processing (in microseconds)
            float timeAvailableUs = (totalSamples / sampleRate) * 1000000.0f;
            // CPU usage percentage
            cpuUsage = (totalProcessingTimeUs / timeAvailableUs) * 100.0f;
            totalProcessingTimeUs = 0;
            totalSamples = 0;
        }
    }

    float getCpuUsage() const {
        return cpuUsage.load();
    }
};

static CpuMonitor g_cpuMonitor;

// Audio callback
void audioCallback(float* left, float* right, int numSamples, void* userData) {
    Synth* synth = static_cast<Synth*>(userData);

    // Measure processing time
    auto start = std::chrono::high_resolution_clock::now();

    // Process stereo output with chorus
    synth->processStereo(left, right, numSamples);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    g_cpuMonitor.update(duration.count(), numSamples);
}

// MIDI callback
void midiCallback(const uint8_t* data, int length, void* userData) {
    Synth* synth = static_cast<Synth*>(userData);

    if (length < 1) return;

    uint8_t status = data[0] & 0xF0;

    // Handle MIDI messages
    if (status == MIDI_NOTE_ON && length >= 3) {
        uint8_t note = data[1];
        uint8_t velocity = data[2];

        if (velocity > 0) {
            float normalizedVelocity = velocity / 127.0f;
            synth->handleNoteOn(note, normalizedVelocity);
            std::cout << "Note ON: " << (int)note << ", vel=" << (int)velocity << std::endl;
        } else {
            // Velocity 0 is treated as Note OFF
            synth->handleNoteOff(note);
            std::cout << "Note OFF: " << (int)note << std::endl;
        }
    } else if (status == MIDI_NOTE_OFF && length >= 3) {
        uint8_t note = data[1];
        synth->handleNoteOff(note);
        std::cout << "Note OFF: " << (int)note << std::endl;
    } else if (status == MIDI_CONTROL_CHANGE && length >= 3) {
        uint8_t controller = data[1];
        uint8_t value = data[2];
        std::cout << "MIDI CC: " << (int)controller << " = " << (int)value << std::endl;
        // CC handling can be expanded in the future for real-time parameter control
    }
}

// Default synth parameters (classic Juno sound)
void initializeDefaultParameters(Synth& synth) {
    // DCO parameters - classic sawtooth with some pulse
    DcoParams dcoParams;
    dcoParams.sawLevel = 0.6f;
    dcoParams.pulseLevel = 0.4f;
    dcoParams.subLevel = 0.0f;
    dcoParams.noiseLevel = 0.0f;
    dcoParams.pulseWidth = 0.5f;
    dcoParams.pwmDepth = 0.0f;
    dcoParams.lfoTarget = DcoParams::LFO_OFF;
    dcoParams.detune = 0.0f;
    dcoParams.enableDrift = true;
    synth.setDcoParameters(dcoParams);

    // Filter parameters - warm Juno sound
    FilterParams filterParams;
    filterParams.cutoff = 0.7f;
    filterParams.resonance = 0.3f;
    filterParams.envAmount = 0.5f;
    filterParams.lfoAmount = 0.0f;
    filterParams.keyTrack = FilterParams::KEY_TRACK_HALF;
    filterParams.drive = 1.0f;
    synth.setFilterParameters(filterParams);

    // Filter envelope - punchy but smooth
    EnvelopeParams filterEnvParams;
    filterEnvParams.attack = 0.01f;
    filterEnvParams.decay = 0.4f;
    filterEnvParams.sustain = 0.6f;
    filterEnvParams.release = 0.5f;
    synth.setFilterEnvParameters(filterEnvParams);

    // Amplitude envelope - fast attack
    EnvelopeParams ampEnvParams;
    ampEnvParams.attack = 0.005f;
    ampEnvParams.decay = 0.3f;
    ampEnvParams.sustain = 0.8f;
    ampEnvParams.release = 0.3f;
    synth.setAmpEnvParameters(ampEnvParams);

    // LFO parameters - moderate rate
    LfoParams lfoParams;
    lfoParams.rate = 3.0f;  // 3 Hz
    lfoParams.amount = 0.0f;
    lfoParams.target = LfoParams::LFO_OFF;
    synth.setLfoParameters(lfoParams);

    // Chorus parameters - classic Juno chorus mode II
    ChorusParams chorusParams;
    chorusParams.mode = ChorusParams::MODE_II;
    chorusParams.rate = 0.6f;
    chorusParams.depth = 0.5f;
    chorusParams.feedback = 0.3f;
    chorusParams.mix = 0.5f;
    synth.setChorusParameters(chorusParams);
}

int main(int argc, char** argv) {
    std::cout << "Poor House Juno - Raspberry Pi Edition" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "6-Voice Polyphonic Juno-106 Emulator" << std::endl;
    std::cout << "=======================================" << std::endl;

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize synth with default parameters
    const float sampleRate = 48000.0f;
    g_synth.setSampleRate(sampleRate);
    g_cpuMonitor.setSampleRate(sampleRate);
    initializeDefaultParameters(g_synth);

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
    std::cout << "Latency: ~" << (audio.getBufferSize() * 1000.0f / audio.getSampleRate()) << " ms" << std::endl;
    std::cout << "\nFeatures:" << std::endl;
    std::cout << "  - 6-voice polyphony with voice stealing" << std::endl;
    std::cout << "  - BBD stereo chorus effect" << std::endl;
    std::cout << "  - Full MIDI support (Note On/Off, velocity)" << std::endl;
    std::cout << "\nReady for MIDI input. Press Ctrl+C to exit.\n" << std::endl;

    // Test: play a chord for a few seconds if no MIDI
    if (!midi.isRunning()) {
        std::cout << "No MIDI available, playing test chord for 3 seconds..." << std::endl;
        std::cout << "(C major triad: C4, E4, G4)" << std::endl;
        g_synth.handleNoteOn(60, 0.8f);  // C4
        g_synth.handleNoteOn(64, 0.8f);  // E4
        g_synth.handleNoteOn(67, 0.8f);  // G4
        sleep(3);
        g_synth.allNotesOff();
        std::cout << "Test chord finished. Running idle (waiting for Ctrl+C)..." << std::endl;
    }

    // Main loop - display CPU usage every 5 seconds
    int loopCounter = 0;
    while (g_running) {
        sleep(1);
        loopCounter++;

        if (loopCounter >= 5) {
            float cpuUsage = g_cpuMonitor.getCpuUsage();
            if (cpuUsage > 0.0f) {
                std::cout << "CPU Usage: " << cpuUsage << "%" << std::endl;
            }
            loopCounter = 0;
        }
    }

    std::cout << "Shutting down..." << std::endl;

    midi.shutdown();
    audio.shutdown();

    std::cout << "Goodbye!" << std::endl;

    return 0;
}
