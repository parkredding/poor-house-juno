#include "audio_driver.h"
#include <iostream>
#include <cstring>

namespace phj {

AudioDriver::AudioDriver()
    : handle_(nullptr)
    , callback_(nullptr)
    , callbackUserData_(nullptr)
    , sampleRate_(0)
    , bufferSize_(0)
    , running_(false)
    , interleavedBuffer_(nullptr)
    , audioThread_(0)
{
}

AudioDriver::~AudioDriver() {
    shutdown();
}

bool AudioDriver::initialize(const std::string& deviceName, unsigned int sampleRate, unsigned int bufferSize) {
    int err;

    // Open PCM device
    err = snd_pcm_open(&handle_, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        std::cerr << "Cannot open audio device " << deviceName << ": " << snd_strerror(err) << std::endl;
        return false;
    }

    // Allocate hardware parameters object
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle_, params);

    // Set parameters
    snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle_, params, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(handle_, params, 2);  // Stereo
    snd_pcm_hw_params_set_rate_near(handle_, params, &sampleRate, 0);
    snd_pcm_hw_params_set_period_size_near(handle_, params, (snd_pcm_uframes_t*)&bufferSize, 0);

    // Write parameters
    err = snd_pcm_hw_params(handle_, params);
    if (err < 0) {
        std::cerr << "Cannot set hardware parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    sampleRate_ = sampleRate;
    bufferSize_ = bufferSize;

    // Allocate interleaved buffer
    interleavedBuffer_ = new float[bufferSize_ * 2];  // Stereo

    std::cout << "Audio initialized: " << sampleRate_ << " Hz, " << bufferSize_ << " samples/buffer" << std::endl;

    return true;
}

void AudioDriver::shutdown() {
    stop();

    if (interleavedBuffer_) {
        delete[] interleavedBuffer_;
        interleavedBuffer_ = nullptr;
    }

    if (handle_) {
        snd_pcm_close(handle_);
        handle_ = nullptr;
    }
}

void AudioDriver::setCallback(AudioCallback callback, void* userData) {
    callback_ = callback;
    callbackUserData_ = userData;
}

bool AudioDriver::start() {
    if (running_ || !handle_ || !callback_) {
        return false;
    }

    running_ = true;

    // Create audio thread
    int err = pthread_create(&audioThread_, nullptr, audioThreadFunc, this);
    if (err != 0) {
        std::cerr << "Failed to create audio thread" << std::endl;
        running_ = false;
        return false;
    }

    return true;
}

void AudioDriver::stop() {
    if (!running_) return;

    running_ = false;

    if (audioThread_) {
        pthread_join(audioThread_, nullptr);
        audioThread_ = 0;
    }

    if (handle_) {
        snd_pcm_drop(handle_);
    }
}

void* AudioDriver::audioThreadFunc(void* arg) {
    AudioDriver* driver = static_cast<AudioDriver*>(arg);
    driver->runAudioLoop();
    return nullptr;
}

void AudioDriver::runAudioLoop() {
    float* leftBuffer = new float[bufferSize_];
    float* rightBuffer = new float[bufferSize_];

    while (running_) {
        // Call user callback to fill buffers
        callback_(leftBuffer, rightBuffer, bufferSize_, callbackUserData_);

        // Interleave samples (LRLRLR...)
        for (unsigned int i = 0; i < bufferSize_; ++i) {
            interleavedBuffer_[i * 2] = leftBuffer[i];
            interleavedBuffer_[i * 2 + 1] = rightBuffer[i];
        }

        // Write to ALSA
        snd_pcm_sframes_t frames = snd_pcm_writei(handle_, interleavedBuffer_, bufferSize_);

        if (frames < 0) {
            frames = snd_pcm_recover(handle_, frames, 0);
        }

        if (frames < 0) {
            std::cerr << "snd_pcm_writei failed: " << snd_strerror(frames) << std::endl;
            break;
        }

        if (frames > 0 && frames < (snd_pcm_sframes_t)bufferSize_) {
            std::cerr << "Short write (expected " << bufferSize_ << ", wrote " << frames << ")" << std::endl;
        }
    }

    delete[] leftBuffer;
    delete[] rightBuffer;
}

} // namespace phj
