#include "audio_driver.h"
#include <iostream>
#include <cstring>
#include <sched.h>
#include <cstdint>

// Enable denormal flushing to prevent CPU slowdown
#if defined(__x86_64__) || defined(__i386__)
    #include <xmmintrin.h>
    #include <pmmintrin.h>
#endif

namespace phj {

AudioDriver::AudioDriver()
    : handle_(nullptr)
    , callback_(nullptr)
    , callbackUserData_(nullptr)
    , sampleRate_(0)
    , bufferSize_(0)
    , running_(false)
    , format_(SND_PCM_FORMAT_UNKNOWN)
    , interleavedBuffer_(nullptr)
    , hwBuffer_(nullptr)
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

    err = snd_pcm_hw_params_any(handle_, params);
    if (err < 0) {
        std::cerr << "Cannot initialize hardware parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Set parameters with error checking
    err = snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        std::cerr << "Cannot set access type: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Try multiple formats in order of preference
    // 1. FLOAT_LE (best quality, no conversion needed)
    // 2. S32_LE (good quality, 32-bit integer)
    // 3. S16_LE (most compatible, 16-bit integer)
    snd_pcm_format_t formatsToTry[] = {
        SND_PCM_FORMAT_FLOAT_LE,
        SND_PCM_FORMAT_S32_LE,
        SND_PCM_FORMAT_S16_LE
    };
    const char* formatNames[] = {"FLOAT_LE", "S32_LE", "S16_LE"};

    bool formatSet = false;
    for (int i = 0; i < 3; i++) {
        err = snd_pcm_hw_params_set_format(handle_, params, formatsToTry[i]);
        if (err >= 0) {
            format_ = formatsToTry[i];
            formatSet = true;
            if (i > 0) {
                std::cout << "Note: Using " << formatNames[i] << " format (FLOAT_LE not supported)" << std::endl;
            }
            break;
        }
    }

    if (!formatSet) {
        std::cerr << "Cannot set sample format: Device doesn't support FLOAT_LE, S32_LE, or S16_LE" << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    err = snd_pcm_hw_params_set_channels(handle_, params, 2);
    if (err < 0) {
        std::cerr << "Cannot set channel count: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Set sample rate and read back actual value
    unsigned int requestedRate = sampleRate;
    err = snd_pcm_hw_params_set_rate_near(handle_, params, &sampleRate, 0);
    if (err < 0) {
        std::cerr << "Cannot set sample rate: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Validate sample rate is reasonable
    if (sampleRate < 8000 || sampleRate > 192000) {
        std::cerr << "Invalid sample rate returned by ALSA: " << sampleRate << " Hz" << std::endl;
        std::cerr << "Requested: " << requestedRate << " Hz" << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    if (sampleRate != requestedRate) {
        std::cout << "Note: Requested " << requestedRate << " Hz, using " << sampleRate << " Hz" << std::endl;
    }

    // Set buffer size - use larger buffer to prevent underruns
    // For low latency: 4 periods provides good balance
    snd_pcm_uframes_t periodSize = bufferSize;
    snd_pcm_uframes_t bufferSizeFrames = bufferSize * 4;  // 4 periods

    // Set periods first (ALSA requirement)
    unsigned int periods = 4;
    err = snd_pcm_hw_params_set_periods_near(handle_, params, &periods, 0);
    if (err < 0) {
        std::cerr << "Cannot set periods: " << snd_strerror(err) << std::endl;
        // Not fatal, continue
    }

    err = snd_pcm_hw_params_set_buffer_size_near(handle_, params, &bufferSizeFrames);
    if (err < 0) {
        std::cerr << "Cannot set buffer size: " << snd_strerror(err) << std::endl;
        // Not fatal, continue
    }

    err = snd_pcm_hw_params_set_period_size_near(handle_, params, &periodSize, 0);
    if (err < 0) {
        std::cerr << "Cannot set period size: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }
    bufferSize = periodSize;

    // Write parameters
    err = snd_pcm_hw_params(handle_, params);
    if (err < 0) {
        std::cerr << "Cannot set hardware parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Read back the actual parameters that were set
    snd_pcm_hw_params_get_rate(params, &sampleRate, 0);
    snd_pcm_hw_params_get_period_size(params, &periodSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &bufferSizeFrames);

    sampleRate_ = sampleRate;
    bufferSize_ = periodSize;

    // Final validation
    if (sampleRate_ == 0) {
        std::cerr << "ERROR: Sample rate is 0 after configuration!" << std::endl;
        std::cerr << "This usually means the audio device doesn't support the requested format." << std::endl;
        snd_pcm_close(handle_);
        handle_ = nullptr;
        return false;
    }

    // Allocate interleaved buffer (always float for DSP processing)
    interleavedBuffer_ = new float[bufferSize_ * 2];  // Stereo

    // Allocate hardware buffer if format conversion needed
    if (format_ == SND_PCM_FORMAT_S16_LE) {
        hwBuffer_ = new int16_t[bufferSize_ * 2];  // 16-bit stereo
    } else if (format_ == SND_PCM_FORMAT_S32_LE) {
        hwBuffer_ = new int32_t[bufferSize_ * 2];  // 32-bit stereo
    } else {
        hwBuffer_ = nullptr;  // FLOAT_LE - no conversion needed
    }

    float latencyMs = (float)bufferSize_ / sampleRate_ * 1000.0f;
    float totalLatencyMs = (float)bufferSizeFrames / sampleRate_ * 1000.0f;
    std::cout << "Audio initialized: " << sampleRate_ << " Hz" << std::endl;
    std::cout << "  Period size: " << bufferSize_ << " samples (" << latencyMs << " ms)" << std::endl;
    std::cout << "  Buffer size: " << bufferSizeFrames << " samples (" << totalLatencyMs << " ms)" << std::endl;

    return true;
}

void AudioDriver::shutdown() {
    stop();

    if (interleavedBuffer_) {
        delete[] interleavedBuffer_;
        interleavedBuffer_ = nullptr;
    }

    if (hwBuffer_) {
        if (format_ == SND_PCM_FORMAT_S16_LE) {
            delete[] static_cast<int16_t*>(hwBuffer_);
        } else if (format_ == SND_PCM_FORMAT_S32_LE) {
            delete[] static_cast<int32_t*>(hwBuffer_);
        }
        hwBuffer_ = nullptr;
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

    // Set real-time priority for audio thread
    struct sched_param param;
    param.sched_priority = 80;  // High priority (1-99, higher is more important)
    int result = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (result != 0) {
        std::cerr << "Warning: Could not set real-time priority for audio thread (run as root or adjust system limits)" << std::endl;
        // Continue anyway - will run at normal priority
    } else {
        std::cout << "Audio thread running at real-time priority (SCHED_FIFO, priority 80)" << std::endl;
    }

    driver->runAudioLoop();
    return nullptr;
}

void AudioDriver::runAudioLoop() {
    // Enable denormal flushing to prevent massive CPU slowdown
    // Denormals (numbers very close to zero) can slow down audio processing by 100x
#if defined(__x86_64__) || defined(__i386__)
    // x86/x64: Enable FTZ (Flush-To-Zero) and DAZ (Denormals-Are-Zero)
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#elif defined(__aarch64__)
    // ARM64: Enable flush-to-zero mode via FPCR register
    uint64_t fpcr;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
    __asm__ __volatile__("msr fpcr, %0" :: "r"(fpcr | (1 << 24)));  // Set FZ bit
#elif defined(__arm__)
    // ARM32: Enable flush-to-zero mode via FPSCR register
    uint32_t fpscr;
    __asm__ __volatile__("vmrs %0, fpscr" : "=r"(fpscr));
    __asm__ __volatile__("vmsr fpscr, %0" :: "r"(fpscr | (1 << 24)));  // Set FZ bit
#endif

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

        // Convert to hardware format if needed
        void* writeBuffer;
        if (format_ == SND_PCM_FORMAT_S16_LE) {
            // Convert float (-1.0 to 1.0) to 16-bit signed integer (-32768 to 32767)
            int16_t* s16Buffer = static_cast<int16_t*>(hwBuffer_);
            for (unsigned int i = 0; i < bufferSize_ * 2; ++i) {
                float sample = interleavedBuffer_[i];
                // Clamp to valid range
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                s16Buffer[i] = static_cast<int16_t>(sample * 32767.0f);
            }
            writeBuffer = hwBuffer_;
        } else if (format_ == SND_PCM_FORMAT_S32_LE) {
            // Convert float (-1.0 to 1.0) to 32-bit signed integer
            int32_t* s32Buffer = static_cast<int32_t*>(hwBuffer_);
            for (unsigned int i = 0; i < bufferSize_ * 2; ++i) {
                float sample = interleavedBuffer_[i];
                // Clamp to valid range
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                s32Buffer[i] = static_cast<int32_t>(sample * 2147483647.0f);
            }
            writeBuffer = hwBuffer_;
        } else {
            // FLOAT_LE - no conversion needed
            writeBuffer = interleavedBuffer_;
        }

        // Write to ALSA
        snd_pcm_sframes_t frames = snd_pcm_writei(handle_, writeBuffer, bufferSize_);

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
