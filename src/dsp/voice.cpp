#include "voice.h"
#include <cmath>

namespace phj {

Voice::Voice()
    : currentNote_(-1)
    , velocity_(0.0f)
    , age_(0.0f)
    , noteActive_(false)
    , sustained_(false)  // M16: Initialize sustain state
    , sampleRate_(SAMPLE_RATE)
    , lfoValue_(0.0f)
    , pitchBend_(0.0f)
    , pitchBendRange_(2.0f)
    , portamentoTime_(0.0f)
    , targetNote_(-1)
    , currentFreq_(440.0f)
    , targetFreq_(440.0f)
    , glideRate_(0.0f)
    , vcaMode_(0)  // M13: Default to ENV mode
    , filterEnvPolarity_(0)  // M13: Default to Normal polarity
    , vcaLevel_(0.8f)  // M14: Default VCA level
    , velocityToFilter_(0.0f)  // M14: Default no velocity to filter
    , velocityToAmp_(1.0f)  // M14: Default full velocity to amp
    , masterTune_(0.0f)  // M14: Default to no tuning offset
{
    dco_.setSampleRate(sampleRate_);
    filter_.setSampleRate(sampleRate_);
    filterEnv_.setSampleRate(sampleRate_);
    ampEnv_.setSampleRate(sampleRate_);
}

void Voice::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    dco_.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
}

void Voice::setParameters(const DcoParams& dcoParams,
                         const FilterParams& filterParams,
                         const EnvelopeParams& filterEnvParams,
                         const EnvelopeParams& ampEnvParams) {
    dco_.setParameters(dcoParams);
    filter_.setParameters(filterParams);
    filterEnv_.setParameters(filterEnvParams);
    ampEnv_.setParameters(ampEnvParams);
}

void Voice::noteOn(int midiNote, float velocity) {
    currentNote_ = midiNote;
    velocity_ = clamp(velocity, 0.0f, 1.0f);
    age_ = 0.0f;
    noteActive_ = true;

    // M11: Setup portamento (glide)
    targetNote_ = midiNote;
    targetFreq_ = midiNoteToFrequency(midiNote);

    // If portamento is enabled and this is a legato note (voice was already active)
    if (portamentoTime_ > 0.0f && isActive()) {
        // Start gliding from current frequency to target
        float glideTimeSamples = portamentoTime_ * sampleRate_;
        glideRate_ = (targetFreq_ - currentFreq_) / glideTimeSamples;
    } else {
        // No glide - jump immediately to target
        currentFreq_ = targetFreq_;
        glideRate_ = 0.0f;
    }

    // Set frequency for filter (use target for filter tracking)
    filter_.setNoteFrequency(targetFreq_);

    // Trigger envelopes and oscillator
    dco_.noteOn();
    filterEnv_.noteOn();
    ampEnv_.noteOn();
}

void Voice::noteOff() {
    // M16: Note is no longer being held by key
    noteActive_ = false;

    // M16: Only release envelopes if NOT sustained by pedal
    // If sustained, mark the voice as sustained but don't release yet
    if (!sustained_) {
        dco_.noteOff();
        filterEnv_.noteOff();
        ampEnv_.noteOff();
    }
}

void Voice::reset() {
    currentNote_ = -1;
    velocity_ = 0.0f;
    age_ = 0.0f;
    noteActive_ = false;
    sustained_ = false;  // M16: Clear sustain state
    lfoValue_ = 0.0f;
    pitchBend_ = 0.0f;
    targetNote_ = -1;
    currentFreq_ = 440.0f;
    targetFreq_ = 440.0f;
    glideRate_ = 0.0f;

    dco_.reset();
    filter_.reset();
    filterEnv_.reset();
    ampEnv_.reset();
}

void Voice::setLfoValue(float lfoValue) {
    lfoValue_ = lfoValue;
    dco_.setLfoValue(lfoValue);
    filter_.setLfoValue(lfoValue);
}

void Voice::setPitchBend(float pitchBend, float pitchBendRange) {
    pitchBend_ = clamp(pitchBend, -1.0f, 1.0f);
    pitchBendRange_ = pitchBendRange;
}

void Voice::setPortamentoTime(float portamentoTime) {
    portamentoTime_ = clamp(portamentoTime, 0.0f, 10.0f);
}

void Voice::setVcaMode(int vcaMode) {
    // M13: Set VCA mode (0=ENV, 1=GATE)
    vcaMode_ = vcaMode;
}

void Voice::setFilterEnvPolarity(int filterEnvPolarity) {
    // M13: Set filter envelope polarity (0=Normal, 1=Inverse)
    filterEnvPolarity_ = filterEnvPolarity;
}

void Voice::setVcaLevel(float vcaLevel) {
    // M14: Set VCA level
    vcaLevel_ = clamp(vcaLevel, 0.0f, 1.0f);
}

void Voice::setVelocitySensitivity(float filterAmount, float ampAmount) {
    // M14: Set velocity sensitivity amounts
    velocityToFilter_ = clamp(filterAmount, 0.0f, 1.0f);
    velocityToAmp_ = clamp(ampAmount, 0.0f, 1.0f);
}

void Voice::setMasterTune(float cents) {
    // M14: Set master tune (±50 cents)
    masterTune_ = clamp(cents, -50.0f, 50.0f);
}

void Voice::setSustained(bool sustained) {
    // M16: Set sustain state
    // If transitioning from sustained to not sustained, release the voice
    if (sustained_ && !sustained && !noteActive_) {
        dco_.noteOff();
        filterEnv_.noteOff();
        ampEnv_.noteOff();
    }
    sustained_ = sustained;
}

Sample Voice::process() {
    // Update voice age if active
    if (isActive()) {
        age_ += 1.0f;
    }

    // M11: Update portamento glide
    updateGlide();

    // M11: Calculate final frequency with pitch bend
    float pitchBendSemitones = pitchBend_ * pitchBendRange_;
    float pitchBendRatio = std::pow(2.0f, pitchBendSemitones / 12.0f);

    // M14: Apply master tune (in cents)
    float masterTuneRatio = std::pow(2.0f, masterTune_ / 1200.0f);

    float finalFreq = currentFreq_ * pitchBendRatio * masterTuneRatio;

    // Update DCO frequency
    dco_.setFrequency(finalFreq);

    // Process envelopes
    float filterEnvValue = filterEnv_.process();
    float ampEnvValue = ampEnv_.process();

    // M13: Apply filter envelope polarity
    float filterModValue = filterEnvValue;
    if (filterEnvPolarity_ == 1) {  // Inverse mode
        filterModValue = 1.0f - filterEnvValue;
    }

    // Set filter envelope modulation
    filter_.setEnvValue(filterModValue);

    // M14: Set filter velocity modulation
    filter_.setVelocityValue(velocity_, velocityToFilter_);

    // Generate oscillator output
    Sample dcoOut = dco_.process();

    // Process through filter
    Sample filtered = filter_.process(dcoOut);

    // M13: Apply VCA mode (ENV or GATE)
    float vcaGain;
    if (vcaMode_ == 1) {  // GATE mode: instant on/off
        vcaGain = noteActive_ ? 1.0f : 0.0f;
    } else {  // ENV mode (default): use amplitude envelope
        vcaGain = ampEnvValue;
    }

    // M14: Apply velocity to amplitude (scaled by velocityToAmp)
    // velocity ranges from 0-1, so we lerp between no effect (1.0) and full effect (velocity)
    float velocityGain = 1.0f - velocityToAmp_ + (velocityToAmp_ * velocity_);

    // M14: Apply VCA level, VCA gain, and velocity
    Sample output = filtered * vcaLevel_ * vcaGain * velocityGain;

    return output;
}

void Voice::process(Sample* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process();
    }
}

bool Voice::isActive() const {
    // Voice is active if either envelope is active
    return filterEnv_.isActive() || ampEnv_.isActive();
}

bool Voice::isReleasing() const {
    // Voice is releasing if note is off but still active
    return !noteActive_ && isActive();
}

void Voice::updateGlide() {
    // M11: Update portamento glide
    if (glideRate_ != 0.0f) {
        // Check if we've reached the target
        if ((glideRate_ > 0.0f && currentFreq_ >= targetFreq_) ||
            (glideRate_ < 0.0f && currentFreq_ <= targetFreq_)) {
            currentFreq_ = targetFreq_;
            glideRate_ = 0.0f;
        } else {
            currentFreq_ += glideRate_;
        }
    }
}

} // namespace phj
