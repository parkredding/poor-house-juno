#include <iostream>
#include <csignal>
#include <atomic>
#include <cstring>
#include <getopt.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <alsa/asoundlib.h>
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

struct MidiDeviceInfo {
    std::string hwId;       // e.g., hw:2,0,0
    std::string cardName;   // e.g., PoorHouseJuno MIDI 1
    std::string deviceName; // e.g., USB MIDI Gadget
    bool isGadget;
};

// Enumerate available ALSA rawmidi input devices.
static std::vector<MidiDeviceInfo> listMidiInputs() {
    std::vector<MidiDeviceInfo> devices;
    int card = -1;
    if (snd_card_next(&card) < 0 || card < 0) {
        return devices;
    }

    while (card >= 0) {
        snd_ctl_t* ctl = nullptr;
        char cardId[32];
        snprintf(cardId, sizeof(cardId), "hw:%d", card);
        if (snd_ctl_open(&ctl, cardId, 0) < 0) {
            if (snd_card_next(&card) < 0) break;
            continue;
        }

        snd_ctl_card_info_t* cardInfo;
        snd_ctl_card_info_alloca(&cardInfo);
        snd_ctl_card_info(ctl, cardInfo);
        std::string cardName = snd_ctl_card_info_get_name(cardInfo);
        std::string cardIdStr = snd_ctl_card_info_get_id(cardInfo);

        int device = -1;
        while (snd_ctl_rawmidi_next_device(ctl, &device) == 0 && device >= 0) {
            snd_rawmidi_info_t* info;
            snd_rawmidi_info_alloca(&info);
            snd_rawmidi_info_set_device(info, device);
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
            snd_rawmidi_info_set_subdevice(info, 0);

            if (snd_ctl_rawmidi_info(ctl, info) < 0) {
                continue;
            }

            int subs = snd_rawmidi_info_get_subdevices_count(info);
            for (int sub = 0; sub < subs; ++sub) {
                snd_rawmidi_info_set_subdevice(info, sub);
                if (snd_ctl_rawmidi_info(ctl, info) < 0) {
                    continue;
                }

                char hwName[32];
                snprintf(hwName, sizeof(hwName), "hw:%d,%d,%d", card, device, sub);

                std::string deviceName = snd_rawmidi_info_get_name(info);
                std::string subName = snd_rawmidi_info_get_subdevice_name(info);
                if (!subName.empty()) {
                    deviceName += " ";
                    deviceName += subName;
                }

                bool isGadget = (cardName.find("g_midi") != std::string::npos) ||
                                (cardIdStr.find("g_midi") != std::string::npos) ||
                                (deviceName.find("Gadget") != std::string::npos) ||
                                (deviceName.find("PoorHouseJuno") != std::string::npos);

                devices.push_back({hwName, cardName, deviceName, isGadget});
            }
        }

        snd_ctl_close(ctl);
        if (snd_card_next(&card) < 0) break;
    }

    return devices;
}

// Decide which MIDI device to use when none was specified.
static MidiDeviceInfo chooseMidiDevice() {
    MidiDeviceInfo chosen;

    const char* envMidi = std::getenv("PHJ_MIDI_DEVICE");
    if (envMidi) {
        chosen.hwId = envMidi;
        chosen.cardName = "Env override";
        chosen.deviceName = envMidi;
        chosen.isGadget = false;
        return chosen;
    }

    auto devices = listMidiInputs();
    if (devices.empty()) {
        chosen.hwId = "default";
        chosen.cardName = "ALSA default";
        chosen.deviceName = "default";
        chosen.isGadget = false;
        return chosen;
    }

    // Prefer USB gadget (DAW → Pi), otherwise first available controller.
    for (const auto& d : devices) {
        if (d.isGadget) {
            return d;
        }
    }

    return devices.front();
}

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

        // M16: Handle all MIDI CC messages (including Arturia MiniLab support)
        synth->handleControlChange(controller, value);
    } else if (status == MIDI_PITCH_BEND && length >= 3) {
        // M11: Pitch bend - combine data[1] (LSB) and data[2] (MSB) into 14-bit value
        int bendValue = data[1] | (data[2] << 7);
        // Convert from 0-16383 to -1.0 to 1.0
        float bendNormalized = (bendValue - 8192) / 8192.0f;
        synth->handlePitchBend(bendNormalized);
        std::cout << "Pitch Bend: " << bendNormalized << std::endl;
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
    filterParams.hpfMode = 0;  // M11: HPF off by default
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
    synth.setLfoParameters(lfoParams);

    // Chorus parameters - classic Juno chorus mode II
    ChorusParams chorusParams;
    chorusParams.mode = 2;  // Mode II
    synth.setChorusParameters(chorusParams);

    // M11: Performance parameters - defaults
    PerformanceParams performanceParams;
    performanceParams.pitchBend = 0.0f;
    performanceParams.pitchBendRange = 2.0f;  // ±2 semitones
    performanceParams.portamentoTime = 0.0f;  // Off by default
    synth.setPerformanceParameters(performanceParams);
}

// Load configuration from config file
struct Config {
    std::string audioDevice;
    std::string midiDevice;
};

Config loadConfig() {
    Config config;
    config.audioDevice = "";  // Empty means not set
    config.midiDevice = "";

    // Try to get HOME directory
    const char* home = std::getenv("HOME");
    if (!home) {
        return config;
    }

    // Build config file path
    std::string configPath = std::string(home) + "/.config/poor-house-juno/config";
    std::ifstream configFile(configPath);

    if (!configFile.is_open()) {
        return config;  // Config file doesn't exist, use defaults
    }

    std::string line;
    while (std::getline(configFile, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse KEY=VALUE format
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "AUDIO_DEVICE" && !value.empty()) {
                config.audioDevice = value;
            } else if (key == "MIDI_DEVICE" && !value.empty()) {
                config.midiDevice = value;
            }
        }
    }

    return config;
}

int main(int argc, char** argv) {
    std::cout << "Poor House Juno - Raspberry Pi Edition" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "6-Voice Polyphonic Juno-106 Emulator" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Usage: poor-house-juno [--audio hw:X,Y,Z] [--midi hw:A,B,C]" << std::endl;
    std::cout << "       Config file: ~/.config/poor-house-juno/config" << std::endl;
    std::cout << "       Env overrides: PHJ_AUDIO_DEVICE, PHJ_MIDI_DEVICE" << std::endl;

    // Load config file
    Config config = loadConfig();

    // Defaults and overrides (priority: CLI args > env vars > config file > default)
    std::string audioDevice = "default";

    // First check config file
    if (!config.audioDevice.empty()) {
        audioDevice = config.audioDevice;
    }

    // Environment variable overrides config file
    const char* envAudio = std::getenv("PHJ_AUDIO_DEVICE");
    if (envAudio) {
        audioDevice = envAudio;
    }

    std::string midiOverride;

    // Check config file for MIDI device
    if (!config.midiDevice.empty()) {
        midiOverride = config.midiDevice;
    }

    // Environment variable overrides config file
    const char* envMidi = std::getenv("PHJ_MIDI_DEVICE");
    if (envMidi) {
        midiOverride = envMidi;
    }

    // CLI options
    static struct option longOptions[] = {
        {"audio", required_argument, nullptr, 'a'},
        {"midi", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:m:h", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'a':
                audioDevice = optarg;
                break;
            case 'm':
                midiOverride = optarg;
                break;
            case 'h':
            default:
                std::cout << "Usage: poor-house-juno [--audio hw:X,Y,Z] [--midi hw:A,B,C]" << std::endl;
                return 0;
        }
    }

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
    std::cout << "Audio selection: " << audioDevice << std::endl;
    if (!audio.initialize(audioDevice, 48000, 128)) {
        std::cerr << "Failed to initialize audio device '" << audioDevice << "'" << std::endl;
        std::cerr << "Run 'aplay -l' to list devices; try --audio hw:0,0 or set PHJ_AUDIO_DEVICE." << std::endl;
        return 1;
    }

    audio.setCallback(audioCallback, &g_synth);

    if (!audio.start()) {
        std::cerr << "Failed to start audio" << std::endl;
        return 1;
    }

    // Initialize MIDI driver
    MidiDriver midi;
    MidiDeviceInfo midiDevice;
    if (!midiOverride.empty()) {
        midiDevice.hwId = midiOverride;
        midiDevice.cardName = "CLI override";
        midiDevice.deviceName = midiOverride;
        midiDevice.isGadget = false;
    } else {
        midiDevice = chooseMidiDevice();
    }

    std::cout << "MIDI selection: " << midiDevice.hwId
              << " (" << midiDevice.cardName << " - " << midiDevice.deviceName << ")"
              << (midiDevice.isGadget ? " [USB gadget / DAW]" : " [controller/standalone]")
              << std::endl;

    if (!midi.initialize(midiDevice.hwId)) {
        std::cerr << "Failed to initialize MIDI device " << midiDevice.hwId
                  << " (this is optional)" << std::endl;
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
    std::cout << "  - Full MIDI support (Note On/Off, velocity, pitch bend)" << std::endl;
    std::cout << "  - M11: HPF, Pitch Bend (±2 semitones), Portamento" << std::endl;
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
