#pragma once

#include <alsa/asoundlib.h>
#include <string>
#include "../../dsp/types.h"

namespace phj {

class AudioDriver {
public:
    AudioDriver();
    ~AudioDriver();

    bool initialize(const std::string& deviceName, unsigned int sampleRate, unsigned int bufferSize);
    void shutdown();

    // Audio callback - called by ALSA when buffer needs filling
    using AudioCallback = void(*)(float* left, float* right, int numSamples, void* userData);
    void setCallback(AudioCallback callback, void* userData);

    bool start();
    void stop();

    bool isRunning() const { return running_; }
    unsigned int getSampleRate() const { return sampleRate_; }
    unsigned int getBufferSize() const { return bufferSize_; }

private:
    snd_pcm_t* handle_;
    AudioCallback callback_;
    void* callbackUserData_;

    unsigned int sampleRate_;
    unsigned int bufferSize_;
    bool running_;

    float* interleavedBuffer_;  // Temporary buffer for ALSA interleaved format

    void runAudioLoop();
    static void* audioThreadFunc(void* arg);
    pthread_t audioThread_;
};

} // namespace phj
