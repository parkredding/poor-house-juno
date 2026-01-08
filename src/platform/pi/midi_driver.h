#pragma once

#include <alsa/asoundlib.h>
#include <string>
#include <cstdint>

namespace phj {

class MidiDriver {
public:
    MidiDriver();
    ~MidiDriver();

    bool initialize(const std::string& deviceName);
    void shutdown();

    // MIDI callback - called when MIDI message received
    using MidiCallback = void(*)(const uint8_t* data, int length, void* userData);
    void setCallback(MidiCallback callback, void* userData);

    bool start();
    void stop();

    bool isRunning() const { return running_; }

private:
    snd_rawmidi_t* handle_;
    MidiCallback callback_;
    void* callbackUserData_;
    bool running_;

    void runMidiLoop();
    static void* midiThreadFunc(void* arg);
    pthread_t midiThread_;
};

} // namespace phj
