#include "midi_driver.h"
#include <iostream>

namespace phj {

MidiDriver::MidiDriver()
    : handle_(nullptr)
    , callback_(nullptr)
    , callbackUserData_(nullptr)
    , running_(false)
    , midiThread_(0)
{
}

MidiDriver::~MidiDriver() {
    shutdown();
}

bool MidiDriver::initialize(const std::string& deviceName) {
    int err = snd_rawmidi_open(&handle_, nullptr, deviceName.c_str(), SND_RAWMIDI_NONBLOCK);
    if (err < 0) {
        std::cerr << "Cannot open MIDI device " << deviceName << ": " << snd_strerror(err) << std::endl;
        return false;
    }

    std::cout << "MIDI initialized: " << deviceName << std::endl;
    return true;
}

void MidiDriver::shutdown() {
    stop();

    if (handle_) {
        snd_rawmidi_close(handle_);
        handle_ = nullptr;
    }
}

void MidiDriver::setCallback(MidiCallback callback, void* userData) {
    callback_ = callback;
    callbackUserData_ = userData;
}

bool MidiDriver::start() {
    if (running_ || !handle_ || !callback_) {
        return false;
    }

    running_ = true;

    int err = pthread_create(&midiThread_, nullptr, midiThreadFunc, this);
    if (err != 0) {
        std::cerr << "Failed to create MIDI thread" << std::endl;
        running_ = false;
        return false;
    }

    return true;
}

void MidiDriver::stop() {
    if (!running_) return;

    running_ = false;

    if (midiThread_) {
        pthread_join(midiThread_, nullptr);
        midiThread_ = 0;
    }
}

void* MidiDriver::midiThreadFunc(void* arg) {
    MidiDriver* driver = static_cast<MidiDriver*>(arg);
    driver->runMidiLoop();
    return nullptr;
}

void MidiDriver::runMidiLoop() {
    uint8_t buffer[256];

    while (running_) {
        ssize_t bytes = snd_rawmidi_read(handle_, buffer, sizeof(buffer));

        if (bytes > 0) {
            // Call user callback with MIDI data
            callback_(buffer, bytes, callbackUserData_);
        } else if (bytes < 0 && bytes != -EAGAIN) {
            std::cerr << "MIDI read error: " << snd_strerror(bytes) << std::endl;
        }

        // Small sleep to avoid busy-waiting
        usleep(1000);  // 1ms
    }
}

} // namespace phj
