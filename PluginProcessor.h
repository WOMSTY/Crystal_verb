#pragma once
#include <JuceHeader.h>
#include "CrystalEngine.h"

class NewProjectAudioProcessor : public juce::AudioProcessor
{
public:
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<Grain, CrystalEngine::kMaxGrains> getParticleSnapshot() const
    {
        return mCrystalEngineL.getParticlesSnapshot();
    }
    float getOutputRms() const noexcept
    {
        return (mCrystalEngineL.getOutputRms() + mCrystalEngineR.getOutputRms()) * 0.5f;
    }
    float getLoopTimeSec() const noexcept { return mCrystalEngineL.getLoopTimeSec(); }

    std::atomic<float> mHostBPM{ 120.0f };
    std::atomic<bool> mHostPlaying{ false };

    CrystalEngine& getCrystalEngine() { return mCrystalEngineL; }

private:
    CrystalEngine mCrystalEngineL{ 42 };
    CrystalEngine mCrystalEngineR{ 43 };

    juce::LinearSmoothedValue<float> mSmoothWetDry, mSmoothLoopTime, mSmoothFocusFreq;
    juce::LinearSmoothedValue<float> mSmoothFeedback, mSmoothReverbAmt, mSmoothReverseAmt;
    juce::LinearSmoothedValue<float> mSmoothCutterRate, mSmoothTremoloRate;
    juce::LinearSmoothedValue<float> mSmoothPitch, mSmoothDuck, mSmoothLife;
    juce::LinearSmoothedValue<float> mSmoothShapeAmt, mSmoothShapeRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectAudioProcessor)
};