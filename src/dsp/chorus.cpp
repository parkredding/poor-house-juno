#include "chorus.h"
#include <cstring>

namespace phj {

Chorus::Chorus()
    : sampleRate_(SAMPLE_RATE)
    , mode_(OFF)
    , delayWritePos_(0)
    , lfo1Phase_(0.0f)
    , lfo2Phase_(0.0f)
{
    std::memset(delayBuffer1_, 0, sizeof(delayBuffer1_));
    std::memset(delayBuffer2_, 0, sizeof(delayBuffer2_));
}

void Chorus::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    reset();
}

void Chorus::reset() {
    std::memset(delayBuffer1_, 0, sizeof(delayBuffer1_));
    std::memset(delayBuffer2_, 0, sizeof(delayBuffer2_));
    delayWritePos_ = 0;
    lfo1Phase_ = 0.0f;
    lfo2Phase_ = 0.0f;
}

void Chorus::setMode(Mode mode) {
    mode_ = mode;
}

float Chorus::getLfoValue(float phase) const {
    // Triangle wave LFO (like the Juno-106)
    // Output range: -1.0 to 1.0
    if (phase < 0.5f) {
        return 4.0f * phase - 1.0f;  // Rising: -1 to 1
    } else {
        return 3.0f - 4.0f * phase;  // Falling: 1 to -1
    }
}

Sample Chorus::readDelayLine(const Sample* buffer, float delaySamples) const {
    // Linear interpolation for smooth delay modulation
    float readPos = delayWritePos_ - delaySamples;

    // Wrap around buffer
    while (readPos < 0.0f) {
        readPos += MAX_DELAY_SAMPLES;
    }
    while (readPos >= MAX_DELAY_SAMPLES) {
        readPos -= MAX_DELAY_SAMPLES;
    }

    // Get integer and fractional parts
    int index0 = static_cast<int>(readPos);
    int index1 = (index0 + 1) % MAX_DELAY_SAMPLES;
    float frac = readPos - index0;

    // Linear interpolation
    return buffer[index0] + frac * (buffer[index1] - buffer[index0]);
}

void Chorus::process(Sample input, Sample& leftOut, Sample& rightOut) {
    // If chorus is off, just pass through dry signal
    if (mode_ == OFF) {
        leftOut = input;
        rightOut = input;
        return;
    }

    // Write input to both delay lines
    delayBuffer1_[delayWritePos_] = input;
    delayBuffer2_[delayWritePos_] = input;

    // Update write position
    delayWritePos_ = (delayWritePos_ + 1) % MAX_DELAY_SAMPLES;

    // Process Chorus I (if enabled)
    Sample chorus1Left = 0.0f;
    Sample chorus1Right = 0.0f;

    if (mode_ == MODE_I || mode_ == MODE_BOTH) {
        // Update LFO1
        float lfo1 = getLfoValue(lfo1Phase_);
        lfo1Phase_ += CHORUS_I_RATE_HZ / sampleRate_;
        if (lfo1Phase_ >= 1.0f) {
            lfo1Phase_ -= 1.0f;
        }

        // Calculate modulated delay time in samples
        float baseDelay1 = CHORUS_I_DELAY_MS * sampleRate_ / 1000.0f;
        float depthSamples1 = CHORUS_I_DEPTH_MS * sampleRate_ / 1000.0f;

        // Left and right channels use opposite LFO phases for stereo width
        float delayLeft1 = baseDelay1 + lfo1 * depthSamples1;
        float delayRight1 = baseDelay1 - lfo1 * depthSamples1;

        // Read from delay line with modulated delays
        chorus1Left = readDelayLine(delayBuffer1_, delayLeft1);
        chorus1Right = readDelayLine(delayBuffer1_, delayRight1);
    }

    // Process Chorus II (if enabled)
    Sample chorus2Left = 0.0f;
    Sample chorus2Right = 0.0f;

    if (mode_ == MODE_II || mode_ == MODE_BOTH) {
        // Update LFO2 (offset by 90 degrees from LFO1 for richer sound)
        float lfo2 = getLfoValue(lfo2Phase_);
        lfo2Phase_ += CHORUS_II_RATE_HZ / sampleRate_;
        if (lfo2Phase_ >= 1.0f) {
            lfo2Phase_ -= 1.0f;
        }

        // Calculate modulated delay time in samples
        float baseDelay2 = CHORUS_II_DELAY_MS * sampleRate_ / 1000.0f;
        float depthSamples2 = CHORUS_II_DEPTH_MS * sampleRate_ / 1000.0f;

        // Left and right channels use opposite LFO phases
        float delayLeft2 = baseDelay2 + lfo2 * depthSamples2;
        float delayRight2 = baseDelay2 - lfo2 * depthSamples2;

        // Read from delay line with modulated delays
        chorus2Left = readDelayLine(delayBuffer2_, delayLeft2);
        chorus2Right = readDelayLine(delayBuffer2_, delayRight2);
    }

    // Mix dry and wet signals
    // Juno-106 chorus has a subtle mix (approximately 80% dry, 20% wet per stage)
    constexpr float dryLevel = 0.8f;
    constexpr float wetLevel = 0.2f;

    if (mode_ == MODE_I) {
        leftOut = dryLevel * input + wetLevel * chorus1Left;
        rightOut = dryLevel * input + wetLevel * chorus1Right;
    } else if (mode_ == MODE_II) {
        leftOut = dryLevel * input + wetLevel * chorus2Left;
        rightOut = dryLevel * input + wetLevel * chorus2Right;
    } else if (mode_ == MODE_BOTH) {
        // When both are active, mix all signals
        // Reduce wet level slightly to prevent buildup
        constexpr float bothWetLevel = 0.15f;
        leftOut = dryLevel * input + bothWetLevel * (chorus1Left + chorus2Left);
        rightOut = dryLevel * input + bothWetLevel * (chorus1Right + chorus2Right);
    }
}

} // namespace phj
