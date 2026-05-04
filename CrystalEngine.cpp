#include "CrystalEngine.h"

static constexpr float kTargetRms = 0.063f;
static constexpr float kAutoGainRate = 0.0002f;
static constexpr float kMinGrainMs = 40.0f;
static constexpr float kMaxGrainMs = 400.0f;
static constexpr float kSpawnThresh = 0.005f;

CrystalEngine::CrystalEngine(uint32_t rngSeed)
{
    mRng.seed(rngSeed);
    if (rngSeed != 42) mSpawnCooldown = 430;
}

void CrystalEngine::prepare(double sampleRate, int)
{
    mSampleRate = static_cast<float>(sampleRate);
    mLoopBuffer.prepare(static_cast<int>(sampleRate * 8.0 + 8));

    for (int k = 0; k < kNumAllpass; ++k)
    {
        const int ds = static_cast<int>(kAllpassMs[k] * sampleRate / 1000.0);
        mAllpassFilters[k].prepare(ds, 0.5f);
    }

    mReverb.setSampleRate(sampleRate);
    juce::Reverb::Parameters p;
    p.roomSize = 0.75f; p.damping = 0.4f; p.wetLevel = 1.0f; p.dryLevel = 0.0f; p.width = 1.0f;
    mReverb.setParameters(p);

    mOutputRms.prepare(mSampleRate, 150.0f);
    mInputRms.prepare(mSampleRate, 50.0f);
    mFormantFilter.prepare(mSampleRate);

    clear();
}

void CrystalEngine::clear()
{
    for (auto& g : mGrains) g.active = false;
    mLoopBuffer.clear();
    for (auto& ap : mAllpassFilters) ap.clear();
    mOutputRms.clear(); mInputRms.clear();
    mAutoGain = 1.0f; mSmoothGrainCount = 0.0f;
    mCutterPhase = 0.0f; mTremoloPhase = 0.0f;
    mFeedbackSample = 0.0f; mSilenceCounter = 0; mTailDrain = 1.0f;
    mScanHead = 0.0f; mMacroLfoPhase = 0.0f; mSeqStep = 0; mSeqCounter = 0;
    mPrevInput = 0.0f; mShimmerSample = 0.0f; mShimmerReadHead = 1.0f;
    mFormantFilter.reset(); mReverb.reset();
}

void CrystalEngine::setCrystalParams(float loopTimeSec, float focusFreq, float reverbAmt,
    float reverseAmt, float cutterRate, float tremoloRate) noexcept
{
    mLoopTime = clampf(loopTimeSec, 0.1f, 8.0f);
    mLoopTimeSec = mLoopTime;

    if (mSnapBPM && mBPM > 0.0f)
    {
        const float spb = 60.0f / mBPM;
        const float divs[] = { spb * 0.5f, spb, spb * 2.0f, spb * 4.0f, spb * 8.0f, spb * 16.0f };
        float best = divs[0], bestDist = std::abs(mLoopTime - divs[0]);
        for (float d : divs) { float dist = std::abs(mLoopTime - d); if (dist < bestDist) { bestDist = dist; best = d; } }
        mLoopTimeSec = clampf(best, 0.1f, 8.0f);
    }

    mFocusFreq = clamp01(focusFreq); mReverbAmt = clamp01(reverbAmt);
    mReverseAmt = clamp01(reverseAmt); mCutterRate = clamp01(cutterRate);
    mTremoloRate = clamp01(tremoloRate);
    mFormantFilter.update(mFocusFreq);

    const float tau = mLoopTimeSec * mSampleRate * 0.4f;
    mDecayCoeff = std::exp(-1.0f / (tau + 1.0f));

    mSeqStepSamples = static_cast<int>(mSampleRate * 60.0f / mBPM / 4.0f);
    if (mSeqStepSamples < 100) mSeqStepSamples = 100;
}

void CrystalEngine::setEffectsState(bool focus, bool reverb, bool reverse, bool cutter, bool tremolo) noexcept
{
    mFocusOn = focus; mReverbOn = reverb; mReverseOn = reverse; mCutterOn = cutter; mTremoloOn = tremolo;
}

void CrystalEngine::setBPMSnap(bool snap, float bpm) noexcept
{
    mSnapBPM = snap; mBPM = (bpm > 0.0f) ? bpm : 120.0f;
}

void CrystalEngine::setCrystalSpin(float spinVelocity) noexcept
{
    mCrystalSpin += spinVelocity; mCrystalSpin *= 0.95f; mCrystalAngle += mCrystalSpin;
}

float CrystalEngine::computeOrganicWindow(const Grain& g, float ageNorm) const noexcept
{
    const float skew = g.windowSkew;
    float env;
    if (ageNorm < skew)
        env = std::sin((ageNorm / skew) * juce::MathConstants<float>::halfPi);
    else
        env = std::cos(((ageNorm - skew) / (1.0f - skew)) * juce::MathConstants<float>::halfPi);
    env *= 0.9f + 0.1f * std::sin(ageNorm * juce::MathConstants<float>::pi);
    return env;
}

float CrystalEngine::getGrainSpeed() noexcept
{
    float baseSpeed = 1.0f;
    switch (mSpectralMode)
    {
    case SpectralMode::Harmonic:
    {
        static constexpr float speeds[] = { 0.5f, 1.0f, 1.0f, 2.0f, 3.0f, 4.0f, 0.25f, 0.333f };
        baseSpeed = speeds[static_cast<int>(std::abs(mDist(mRng)) * 8.0f) % 8];
        break;
    }
    case SpectralMode::Octave:
    {
        static constexpr float speeds[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        baseSpeed = speeds[static_cast<int>(std::abs(mDist(mRng)) * 5.0f) % 5];
        break;
    }
    case SpectralMode::Inharmonic:
    {
        static constexpr float speeds[] = { 0.618f, 0.707f, 1.0f, 1.414f, 1.618f, 2.236f, 3.142f, 0.382f };
        baseSpeed = speeds[static_cast<int>(std::abs(mDist(mRng)) * 8.0f) % 8];
        break;
    }
    case SpectralMode::Chaos:
    default:
    {
        baseSpeed = 0.1f + std::abs(mDist(mRng)) * 3.9f;
        if (mRng() % 2 == 0) baseSpeed = -baseSpeed;
        break;
    }
    }
    return baseSpeed;
}

void CrystalEngine::spawnGrain(float energy, float loopSamples) noexcept
{
    for (auto& g : mGrains)
    {
        if (g.active) continue;

        float baseSpeed = getGrainSpeed();

        if (mReverseOn && mRng() % 4 == 0)
        {
            const float r = (mDist(mRng) + 1.0f) * 0.5f;
            if (r < mReverseAmt) baseSpeed = -baseSpeed;
        }

        const float pitchRatio = std::pow(2.0f, mPitchSemitones / 12.0f);
        g.speed = baseSpeed * pitchRatio;
        g.baseSpeed = baseSpeed;

        if (mPlayMode == PlayMode::Scan)
        {
            const float spread = loopSamples * 0.08f;
            g.readHead = mScanHead + mDist(mRng) * spread;
            if (g.readHead > loopSamples) g.readHead -= loopSamples;
            if (g.readHead < 1.0f) g.readHead += loopSamples;
        }
        else
        {
            g.readHead = std::abs(mDist(mRng)) * (loopSamples - 1.0f) + 1.0f;
        }

        const float grainDurMs = kMinGrainMs + std::abs(mDist(mRng)) * (kMaxGrainMs - kMinGrainMs);
        g.lifetime = static_cast<int>(grainDurMs * mSampleRate / 1000.0f);
        g.age = 0; g.gain = clamp01(energy * 2.0f + 0.3f);
        g.active = true; g.energy = clamp01(energy);
        g.lastFaceHit = static_cast<int>(std::abs(mDist(mRng)) * 8.0f) % 8;
        g.driftRate = 0.1f + std::abs(mDist(mRng)) * 2.0f;
        g.driftPhase = mDist(mRng) * juce::MathConstants<float>::twoPi;
        g.windowSkew = 0.2f + std::abs(mDist(mRng)) * 0.6f;
        g.pan = mDist(mRng) * 0.9f;
        g.panSpeed = mDist(mRng) * 0.015f;

        g.x = mDist(mRng) * 0.8f; g.y = mDist(mRng) * 0.8f; g.z = mDist(mRng) * 0.8f;
        g.vx = mDist(mRng) * 0.01f; g.vy = mDist(mRng) * 0.01f; g.vz = mDist(mRng) * 0.01f;
        return;
    }
}

float CrystalEngine::applyFocus(float x) noexcept { return mFocusOn ? mFormantFilter.process(x) : x; }
float CrystalEngine::applyCutter(float x) noexcept
{
    if (!mCutterOn || mCutterRate < 0.01f) return x;
    mCutterPhase += (0.5f + mCutterRate * 19.5f) / mSampleRate;
    if (mCutterPhase >= 1.0f) mCutterPhase -= 1.0f;
    return (mCutterPhase < 0.5f) ? x : 0.0f;
}
float CrystalEngine::applyTremolo(float x) noexcept
{
    if (!mTremoloOn || mTremoloRate < 0.01f) return x;
    mTremoloPhase += (0.1f + mTremoloRate * 14.9f) / mSampleRate;
    if (mTremoloPhase >= 1.0f) mTremoloPhase -= 1.0f;
    const float mod = 0.5f + 0.5f * std::sin(mTremoloPhase * juce::MathConstants<float>::twoPi);
    return x * mod;
}
float CrystalEngine::applyAllpass(float x) noexcept { for (auto& ap : mAllpassFilters) x = ap.process(x); return x; }
float CrystalEngine::applyReverb(float x) noexcept
{
    if (!mReverbOn || mReverbAmt < 0.01f) return x;
    float r = x; mReverb.processMono(&r, 1);
    return x * (1.0f - mReverbAmt) + r * mReverbAmt;
}

float CrystalEngine::process(float inputSample, float size, float diffusion, float feedback,
    float& outL, float& outR) noexcept
{
    static constexpr float kSilenceThresh = 0.0005f;
    static constexpr float kSilenceOnsetMs = 500.0f;
    static constexpr float kDrainRate = 0.9994f;
    const int silenceOnsetSamples = static_cast<int>(kSilenceOnsetMs * mSampleRate / 1000.0f);

    if (std::abs(inputSample) > kSilenceThresh) { mSilenceCounter = 0; mTailDrain = 1.0f; }
    else { if (mSilenceCounter < silenceOnsetSamples) ++mSilenceCounter; else mTailDrain = kDrainRate; }

    const float loopSamples = clampf(mLoopTimeSec * mSampleRate, 0.1f * mSampleRate, 8.0f * mSampleRate);

    // ---- Macro LFO (Shape) ----
    mMacroLfoPhase += mShapeRate / mSampleRate;
    if (mMacroLfoPhase >= 1.0f) mMacroLfoPhase -= 1.0f;
    const float macroSine = std::sin(mMacroLfoPhase * juce::MathConstants<float>::twoPi);
    const float macroMod = 1.0f + macroSine * mShapeAmt * 0.5f; // 0.5x - 1.5x densité
    const float macroPitch = macroSine * mShapeAmt * 1.0f; // ±1 st

    // ---- Shimmer + écriture ----
    const float shimmerFeedback = mShimmerSample * mReverbAmt * 0.25f;
    const float toWrite = inputSample + mFeedbackSample * feedback + shimmerFeedback;
    const float drive = 1.0f + mLife * 1.5f;
    const float toWriteSat = std::tanh(toWrite * drive) * (1.0f / (1.0f + mLife * 0.3f));
    if (!mFrozen) mLoopBuffer.push(toWriteSat);
    mInputRms.push(inputSample);

    // ---- Scan head update ----
    if (mPlayMode == PlayMode::Scan)
    {
        mScanHead += (1.0f + mShapeAmt) * 0.5f;
        if (mScanHead > loopSamples) mScanHead -= loopSamples;
    }

    // ---- Transient detection ----
    const float deriv = std::abs(inputSample - mPrevInput);
    mPrevInput = inputSample;
    const bool transientDetected = mTransientSync && (deriv > mTransientThresh);

    // ---- Spawn ----
    const float absIn = std::abs(inputSample);
    const float bufEnergy = std::abs(mLoopBuffer.read(clampf(mLoopTime * mSampleRate * 4.0f, 1.0f, mLoopBuffer.size() - 2.0f)));
    const float spawnEnergy = std::max(absIn, bufEnergy * 0.3f);
    const float dynamicThresh = kSpawnThresh * macroMod;

    if ((spawnEnergy > dynamicThresh && mSpawnCooldown <= 0) || transientDetected)
    {
        const int cooldownMs = static_cast<int>((80.0f - size * 65.0f) / macroMod);
        spawnGrain(spawnEnergy, loopSamples);
        mSpawnCooldown = static_cast<int>(cooldownMs * mSampleRate / 1000.0f);
        if (transientDetected) mSpawnCooldown = static_cast<int>(mSpawnCooldown * 0.3f); // favoriser le sync
    }
    if (mSpawnCooldown > 0) --mSpawnCooldown;

    // ---- Accumulation grains (stéréo) ----
    float grainOutL = 0.0f, grainOutR = 0.0f;
    int activeCount = 0;
    const float globalDriftRatio = std::pow(2.0f, macroPitch / 12.0f);

    for (auto& g : mGrains)
    {
        if (!g.active) continue;
        ++activeCount;
        const float ageNorm = static_cast<float>(g.age) / static_cast<float>(g.lifetime);
        if (ageNorm >= 1.0f) { g.active = false; continue; }

        float env = computeOrganicWindow(g, ageNorm);
        g.age++; g.energy = env * g.gain;

        g.driftPhase += g.driftRate / mSampleRate;
        const float driftFactor = 1.0f + 0.03f * std::sin(g.driftPhase);
        const float currentSpeed = g.speed * driftFactor * globalDriftRatio;

        const float sample = mLoopBuffer.read(clampf(g.readHead, 1.0f, loopSamples - 1.0f));
        const float grainSig = sample * env * g.gain;

        g.pan += g.panSpeed;
        if (g.pan > 1.0f) { g.pan = 1.0f; g.panSpeed = -g.panSpeed; }
        if (g.pan < -1.0f) { g.pan = -1.0f; g.panSpeed = -g.panSpeed; }
        const float panAngle = (g.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        grainOutL += grainSig * std::cos(panAngle);
        grainOutR += grainSig * std::sin(panAngle);

        g.readHead += currentSpeed;
        if (g.readHead > loopSamples) g.readHead -= (loopSamples - 1.0f);
        if (g.readHead < 1.0f) g.readHead += (loopSamples - 1.0f);

        g.x += g.vx; g.y += g.vy; g.z += g.vz;
        const float dist = std::sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
        if (dist > 0.9f) { g.vx *= -0.8f; g.vy *= -0.8f; g.vz *= -0.8f; }
    }

    mSmoothGrainCount += 0.005f * (static_cast<float>(activeCount) - mSmoothGrainCount);
    float wetL = 0.0f, wetR = 0.0f;
    if (mSmoothGrainCount > 0.1f)
    {
        const float norm = 1.0f / std::sqrt(mSmoothGrainCount + 1.0f);
        wetL = grainOutL * norm;
        wetR = grainOutR * norm;
    }

    // ---- Diffusion (traitée en mono sum pour la chaîne d'effets) ----
    float wet = (wetL + wetR) * 0.5f;
    const float diffused = applyAllpass(wet);
    wet = wet * (1.0f - diffusion) + diffused * diffusion;

    // ---- Chaîne d'effets ----
    wet = applyFocus(wet);
    wet = applyCutter(wet);
    wet = applyTremolo(wet);
    wet = applyReverb(wet);

    // ---- Ducking ----
    if (mDuckAmt > 0.001f)
    {
        const float inputLevel = clamp01(mInputRms.rms() * 12.0f);
        const float duckGain = 1.0f - mDuckAmt * inputLevel;
        wet *= clamp01(duckGain);
    }

    // ---- Séquenceur Gate (appliqué sur le wet final) ----
    if (mPlayMode == PlayMode::Sequence)
    {
        mSeqCounter++;
        if (mSeqCounter >= mSeqStepSamples)
        {
            mSeqCounter = 0;
            mSeqStep = (mSeqStep + 1) & 15;
        }
        const uint16_t pattern = kSeqPatterns[mSeqPatternIdx];
        const bool stepOn = (pattern >> mSeqStep) & 1;
        if (!stepOn) wet = 0.0f;
    }

    // ---- Feedback ----
    mFeedbackSample = wet * mTailDrain;

    // ---- Shimmer update ----
    if (mReverbAmt > 0.01f)
    {
        mShimmerReadHead += 2.0f;
        if (mShimmerReadHead > loopSamples) mShimmerReadHead -= (loopSamples - 1.0f);
        mShimmerSample = mLoopBuffer.read(mShimmerReadHead);
    }

    // ---- Auto-gain ----
    mOutputRms.push(wet);
    const float rms = mOutputRms.rms();
    if (rms > 1e-6f)
    {
        const float target = kTargetRms / rms;
        const float rate = (target < mAutoGain) ? kAutoGainRate * 8.0f : kAutoGainRate;
        mAutoGain += rate * (target - mAutoGain);
    }
    mAutoGain = clampf(mAutoGain, 0.01f, 5.0f);
    wet *= mAutoGain;

    if (!std::isfinite(wet)) wet = 0.0f;
    wet = clampf(wet, -1.0f, 1.0f);

    // ---- Répartition stéréo finale ----
    outL = wet * (1.0f + (wetL - wetR) * 0.3f);
    outR = wet * (1.0f + (wetR - wetL) * 0.3f);
    const float maxOut = std::max(std::abs(outL), std::abs(outR));
    if (maxOut > 1.0f) { outL /= maxOut; outR /= maxOut; }
    return wet;
}