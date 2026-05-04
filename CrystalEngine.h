#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

static inline float clamp01(float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static inline float clampf(float v, float lo, float hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

class CircularBuffer
{
public:
    void prepare(int size)
    {
        mBuffer.assign(static_cast<size_t>(size + 4), 0.0f);
        mWritePos = 0;
        mSize = size;
    }
    void push(float sample) noexcept
    {
        mBuffer[static_cast<size_t>(mWritePos)] = sample;
        if (++mWritePos >= mSize) mWritePos = 0;
    }
    float read(float delaySamples) const noexcept
    {
        if (mSize <= 4) return 0.0f;
        const float maxD = static_cast<float>(mSize - 2);
        delaySamples = clampf(delaySamples, 1.0f, maxD);
        float readPos = static_cast<float>(mWritePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float>(mSize);
        const int i1 = static_cast<int>(readPos) % mSize;
        const int i0 = (i1 - 1 + mSize) % mSize;
        const int i2 = (i1 + 1) % mSize;
        const int i3 = (i1 + 2) % mSize;
        const float t = readPos - std::floor(readPos);
        const float y0 = mBuffer[i0], y1 = mBuffer[i1];
        const float y2 = mBuffer[i2], y3 = mBuffer[i3];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }
    void clear() noexcept { std::fill(mBuffer.begin(), mBuffer.end(), 0.0f); mWritePos = 0; }
    int size() const noexcept { return mSize; }
private:
    std::vector<float> mBuffer;
    int mWritePos = 0, mSize = 0;
};

class AllpassFilter
{
public:
    void prepare(int delaySamples, float gain = 0.5f)
    {
        mGain = gain;
        mDelaySamples = static_cast<float>(delaySamples);
        mBuffer.prepare(delaySamples + 4);
    }
    float process(float input) noexcept
    {
        const float d = mBuffer.read(mDelaySamples);
        const float w = input + mGain * d;
        const float out = d - mGain * w;
        mBuffer.push(w);
        return out;
    }
    void clear() noexcept { mBuffer.clear(); }
private:
    CircularBuffer mBuffer;
    float mDelaySamples = 0.0f, mGain = 0.5f;
};

class OnePoleLP
{
public:
    void setCutoff(float fc, float fs) noexcept
    {
        const float w = 2.0f * 3.14159265f * fc / fs;
        mA = std::exp(-w);
        mB = 1.0f - mA;
    }
    float process(float x) noexcept { return (mZ = mB * x + mA * mZ); }
    void clear() noexcept { mZ = 0.0f; }
private:
    float mA = 0.0f, mB = 1.0f, mZ = 0.0f;
};

class RmsFollower
{
public:
    void prepare(float sampleRate, float windowMs = 150.0f) noexcept
    {
        const float tc = windowMs * 0.001f * sampleRate;
        mCoeff = std::exp(-1.0f / tc);
        mRms = 0.0f;
    }
    void push(float s) noexcept { mRms = mCoeff * mRms + (1.0f - mCoeff) * s * s; }
    float rms() const noexcept { return std::sqrt(mRms + 1e-12f); }
    void clear() noexcept { mRms = 0.0f; }
private:
    float mCoeff = 0.99f, mRms = 0.0f;
};

class FormantFilter
{
public:
    void prepare(float sr) { mSampleRate = sr; update(0.5f); }
    void update(float focus01)
    {
        const float f1 = 600.0f + focus01 * 400.0f;
        const float f2 = 1400.0f - focus01 * 600.0f;
        mF1.setCoefficients(juce::IIRCoefficients::makeBandPass(mSampleRate, f1, 4.0f));
        mF2.setCoefficients(juce::IIRCoefficients::makeBandPass(mSampleRate, f2, 5.0f));
    }
    float process(float x) noexcept
    {
        return mF1.processSingleSampleRaw(x) * 0.5f + mF2.processSingleSampleRaw(x) * 0.5f;
    }
    void reset() { mF1.reset(); mF2.reset(); }
private:
    float mSampleRate = 44100.0f;
    juce::IIRFilter mF1, mF2;
};

struct Grain
{
    float readHead = 0.0f, speed = 1.0f, baseSpeed = 1.0f, gain = 1.0f;
    float env = 0.0f, energy = 0.0f;
    int age = 0, lifetime = 0;
    bool active = false;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    int lastFaceHit = -1;
    float driftRate = 0.0f, driftPhase = 0.0f, windowSkew = 0.5f;
    float pan = 0.0f, panSpeed = 0.0f;
};

class CrystalEngine
{
public:
    static constexpr int kMaxGrains = 32;
    static constexpr int kNumAllpass = 4;

    enum class PlayMode { Cloud = 0, Scan, Sequence, Resonant };
    enum class SpectralMode { Harmonic = 0, Inharmonic, Octave, Chaos };

    explicit CrystalEngine(uint32_t rngSeed = 42);

    void prepare(double sampleRate, int samplesPerBlock);
    void clear();

    void setCrystalParams(float loopTimeSec, float focusFreq, float reverbAmt,
        float reverseAmt, float cutterRate, float tremoloRate) noexcept;
    void setBPMSnap(bool snap, float bpm) noexcept;
    void setEffectsState(bool focus, bool reverb, bool reverse, bool cutter, bool tremolo) noexcept;
    void setFrozen(bool frozen) noexcept { mFrozen = frozen; }
    bool isFrozen() const noexcept { return mFrozen; }

    void setPitchSemitones(float semitones) noexcept { mPitchSemitones = semitones; }
    void setDuckAmt(float duck) noexcept { mDuckAmt = clamp01(duck); }
    void setLife(float life) noexcept { mLife = clamp01(life); }

    void setPlayMode(PlayMode mode) noexcept { mPlayMode = mode; }
    void setSpectralMode(SpectralMode mode) noexcept { mSpectralMode = mode; }
    void setTransientSync(bool sync) noexcept { mTransientSync = sync; }
    void setShapeAmt(float amt) noexcept { mShapeAmt = clamp01(amt); }
    void setShapeRate(float rate) noexcept { mShapeRate = clampf(rate, 0.1f, 20.0f); }
    void setSeqPattern(int pattern) noexcept { mSeqPatternIdx = pattern & 15; }

    void setCrystalSpin(float spinVelocity) noexcept;
    float getCrystalAngle() const noexcept { return mCrystalAngle; }
    float getLoopTimeSec() const noexcept { return mLoopTimeSec; }
    float getOutputRms() const noexcept { return mOutputRms.rms(); }

    float process(float inputSample, float size, float diffusion, float feedback,
        float& outL, float& outR) noexcept;

    std::array<Grain, kMaxGrains> getParticlesSnapshot() const { return mGrains; }

private:
    void spawnGrain(float energy, float loopSamples) noexcept;
    float computeOrganicWindow(const Grain& g, float ageNorm) const noexcept;
    float applyFocus(float x) noexcept;
    float applyCutter(float x) noexcept;
    float applyTremolo(float x) noexcept;
    float applyAllpass(float x) noexcept;
    float applyReverb(float x) noexcept;
    float getGrainSpeed() noexcept;

    float mSampleRate = 44100.0f;
    float mCrystalAngle = 0.0f, mCrystalSpin = 0.0f;

    float mLoopTime = 1.0f, mLoopTimeSec = 1.0f, mPrevLoopSamples = 0.0f;
    float mFocusFreq = 0.5f, mReverbAmt = 0.2f, mReverseAmt = 0.0f;
    float mCutterRate = 0.0f, mTremoloRate = 0.0f;
    float mPitchSemitones = 0.0f, mDuckAmt = 0.0f, mLife = 0.0f;

    bool mFocusOn = true, mReverbOn = true, mReverseOn = true;
    bool mCutterOn = true, mTremoloOn = true, mFrozen = false;

    PlayMode mPlayMode = PlayMode::Cloud;
    SpectralMode mSpectralMode = SpectralMode::Inharmonic;
    bool mTransientSync = false;
    float mShapeAmt = 0.0f, mShapeRate = 1.0f;
    float mMacroLfoPhase = 0.0f;
    float mScanHead = 0.0f;
    int mSeqPatternIdx = 0, mSeqStep = 0, mSeqCounter = 0;
    int mSeqStepSamples = 2205;
    float mPrevInput = 0.0f;
    static constexpr float mTransientThresh = 0.08f;

    CircularBuffer mLoopBuffer;
    std::array<Grain, kMaxGrains> mGrains{};
    int mSpawnCooldown = 0;
    float mSmoothGrainCount = 0.0f;

    static constexpr int kAllpassMs[kNumAllpass] = { 13, 23, 37, 53 };
    std::array<AllpassFilter, kNumAllpass> mAllpassFilters;

    juce::Reverb mReverb;
    FormantFilter mFormantFilter;

    float mCutterPhase = 0.0f, mTremoloPhase = 0.0f;
    RmsFollower mOutputRms;
    float mAutoGain = 1.0f;
    RmsFollower mInputRms;

    std::mt19937 mRng;
    std::uniform_real_distribution<float> mDist{ -1.0f, 1.0f };

    bool mSnapBPM = false;
    float mBPM = 120.0f;
    float mDecayCoeff = 0.9998f;  
    float mFeedbackSample = 0.0f;
    float mShimmerSample = 0.0f;
    float mShimmerReadHead = 1.0f;
    float mTailDrain = 1.0f;
    int mSilenceCounter = 0;

    static constexpr uint16_t kSeqPatterns[16] = {
        0xFFFF, 0xAAAA, 0x8888, 0x8080, 0xB3AC, 0xCCCC, 0x9248, 0xEEEE,
        0x8A4A, 0xA5A5, 0xD9B3, 0x8000, 0xF0F0, 0xCCF0, 0xAACC, 0x9999
    };
};