#include "PluginProcessor.h"
#include "PluginEditor.h"

static std::unique_ptr<juce::AudioParameterFloat> makeFloatParam(
    const juce::String& id, const juce::String& name,
    juce::NormalisableRange<float> range, float defaultV,
    const juce::String& unit = "",
    std::function<juce::String(float, int)> toString = nullptr,
    std::function<float(const juce::String&)> fromString = nullptr)
{
    auto attrs = juce::AudioParameterFloatAttributes().withLabel(unit);
    if (toString) attrs = attrs.withStringFromValueFunction(toString);
    if (fromString) attrs = attrs.withValueFromStringFunction(fromString);
    return std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name, range, defaultV, attrs);
}

juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto pct = [](float v, int) { return juce::String(v * 100.0f, 0) + "%"; };
    auto frm = [](const juce::String& s) { return s.getFloatValue() / 100.0f; };
    juce::NormalisableRange<float> zeroOne(0.0f, 1.0f, 0.01f);

    auto loopRange = juce::NormalisableRange<float>(0.1f, 8.0f, 0.0f, 0.35f);
    auto loopToStr = [](float v, int) {
        return v < 1.0f ? juce::String(v * 1000.0f, 0) + " ms" : juce::String(v, 2) + " s";
        };
    auto loopFromStr = [](const juce::String& s) { return s.getFloatValue(); };

    auto bpmRange = juce::NormalisableRange<float>(60.0f, 200.0f, 1.0f);
    auto bpmToStr = [](float v, int) { return juce::String(static_cast<int>(v)) + " BPM"; };
    auto bpmFromStr = [](const juce::String& s) { return s.getFloatValue(); };

    auto pitchRange = juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f);
    auto pitchToStr = [](float v, int) {
        if (std::abs(v) < 0.5f) return juce::String("0 (unis.)");
        return (v > 0 ? juce::String("+") : juce::String("")) + juce::String(static_cast<int>(v)) + " st";
        };
    auto pitchFromStr = [](const juce::String& s) { return s.getFloatValue(); };

    params.push_back(makeFloatParam("wetDry", "Wet / Dry", zeroOne, 0.50f, "%", pct, frm));
    params.push_back(makeFloatParam("loopTime", "Loop Time", loopRange, 1.00f, "s", loopToStr, loopFromStr));
    params.push_back(makeFloatParam("focusFreq", "Focus Freq", zeroOne, 0.50f, "%", pct, frm));
    params.push_back(makeFloatParam("feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 0.90f, 0.01f), 0.20f, "%", pct, frm));
    params.push_back(makeFloatParam("reverbAmt", "Reverb", zeroOne, 0.20f, "%", pct, frm));
    params.push_back(makeFloatParam("reverseAmt", "Reverse", zeroOne, 0.00f, "%", pct, frm));
    params.push_back(makeFloatParam("cutterRate", "Cutter", zeroOne, 0.00f, "%", pct, frm));
    params.push_back(makeFloatParam("tremoloRate", "Tremolo", zeroOne, 0.00f, "%", pct, frm));
    params.push_back(makeFloatParam("bpm", "BPM", bpmRange, 120.0f, "BPM", bpmToStr, bpmFromStr));
    params.push_back(makeFloatParam("pitchSemitones", "Pitch", pitchRange, 0.0f, "st", pitchToStr, pitchFromStr));
    params.push_back(makeFloatParam("duckAmt", "Duck", zeroOne, 0.00f, "%", pct, frm));
    params.push_back(makeFloatParam("life", "Life", zeroOne, 0.30f, "%", pct, frm));
    params.push_back(makeFloatParam("shapeAmt", "Shape Amt", zeroOne, 0.00f, "%", pct, frm));
    params.push_back(makeFloatParam("shapeRate", "Shape Rate", juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f), 1.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("playMode", 1), "Play Mode",
        juce::StringArray{ "Cloud", "Scan", "Sequence", "Resonant" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("spectralMode", 1), "Spectral",
        juce::StringArray{ "Harmonic", "Inharmonic", "Octave", "Chaos" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("seqPattern", 1), "Pattern",
        juce::StringArray{ "All", "1/8", "1/4", "1/2", "Groove A", "Offbeat", "Shuffle", "Triplet",
                           "Break", "Complex", "Syncop", "Sparse", "Blocks", "Build", "Var", "Tribal" }, 0));

    auto makeBool = [](const char* id, const char* name, bool def) {
        return std::make_unique<juce::AudioParameterBool>(juce::ParameterID(id, 1), name, def);
        };
    params.push_back(makeBool("focusOn", "Focus On", true));
    params.push_back(makeBool("reverbOn", "Reverb On", true));
    params.push_back(makeBool("reverseOn", "Reverse On", true));
    params.push_back(makeBool("cutterOn", "Cutter On", true));
    params.push_back(makeBool("tremoloOn", "Tremolo On", true));
    params.push_back(makeBool("snapBPM", "Snap BPM", false));
    params.push_back(makeBool("freeze", "Freeze", false));
    params.push_back(makeBool("transientSync", "Sync", false));

    return { params.begin(), params.end() };
}

NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#else
    :
#endif
    apvts(*this, nullptr, "CrystalVerbState", createParameterLayout())
{}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {}

const juce::String NewProjectAudioProcessor::getName() const { return JucePlugin_Name; }
bool NewProjectAudioProcessor::acceptsMidi() const { return false; }
bool NewProjectAudioProcessor::producesMidi() const { return false; }
bool NewProjectAudioProcessor::isMidiEffect() const { return false; }
double NewProjectAudioProcessor::getTailLengthSeconds() const { return 3.0; }
int NewProjectAudioProcessor::getNumPrograms() { return 1; }
int NewProjectAudioProcessor::getCurrentProgram() { return 0; }
void NewProjectAudioProcessor::setCurrentProgram(int) {}
const juce::String NewProjectAudioProcessor::getProgramName(int) { return {}; }
void NewProjectAudioProcessor::changeProgramName(int, const juce::String&) {}

void NewProjectAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mCrystalEngineL.prepare(sampleRate, samplesPerBlock);
    mCrystalEngineR.prepare(sampleRate, samplesPerBlock);

    mSmoothLoopTime.reset(sampleRate, 0.20); mSmoothLoopTime.setCurrentAndTargetValue(1.0f);
    mSmoothPitch.reset(sampleRate, 0.05); mSmoothPitch.setCurrentAndTargetValue(0.0f);
    mSmoothDuck.reset(sampleRate, 0.02); mSmoothDuck.setCurrentAndTargetValue(0.0f);

    const double ramp = 0.02;
    auto init = [&](juce::LinearSmoothedValue<float>& sv, float v) {
        sv.reset(sampleRate, ramp); sv.setCurrentAndTargetValue(v);
        };
    init(mSmoothWetDry, 0.50f); init(mSmoothFocusFreq, 0.50f); init(mSmoothFeedback, 0.20f);
    init(mSmoothReverbAmt, 0.20f); init(mSmoothReverseAmt, 0.00f); init(mSmoothCutterRate, 0.00f);
    init(mSmoothTremoloRate, 0.00f); init(mSmoothLife, 0.30f);
    init(mSmoothShapeAmt, 0.00f); init(mSmoothShapeRate, 1.0f);
}

void NewProjectAudioProcessor::releaseResources()
{
    mCrystalEngineL.clear(); mCrystalEngineR.clear();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}
#endif

void NewProjectAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int N = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch) buffer.clear(ch, 0, N);

    float effectiveBPM = apvts.getRawParameterValue("bpm")->load();
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm().hasValue())
            {
                effectiveBPM = static_cast<float>(*pos->getBpm());
                mHostBPM.store(effectiveBPM);
            }
            hostPlaying = pos->getIsPlaying();
            mHostPlaying.store(hostPlaying);
        }
    }

    mSmoothWetDry.setTargetValue(*apvts.getRawParameterValue("wetDry"));
    mSmoothLoopTime.setTargetValue(*apvts.getRawParameterValue("loopTime"));
    mSmoothFocusFreq.setTargetValue(*apvts.getRawParameterValue("focusFreq"));
    mSmoothFeedback.setTargetValue(*apvts.getRawParameterValue("feedback"));
    mSmoothReverbAmt.setTargetValue(*apvts.getRawParameterValue("reverbAmt"));
    mSmoothReverseAmt.setTargetValue(*apvts.getRawParameterValue("reverseAmt"));
    mSmoothCutterRate.setTargetValue(*apvts.getRawParameterValue("cutterRate"));
    mSmoothTremoloRate.setTargetValue(*apvts.getRawParameterValue("tremoloRate"));
    mSmoothPitch.setTargetValue(*apvts.getRawParameterValue("pitchSemitones"));
    mSmoothDuck.setTargetValue(*apvts.getRawParameterValue("duckAmt"));
    mSmoothLife.setTargetValue(*apvts.getRawParameterValue("life"));
    mSmoothShapeAmt.setTargetValue(*apvts.getRawParameterValue("shapeAmt"));
    mSmoothShapeRate.setTargetValue(*apvts.getRawParameterValue("shapeRate"));

    const bool focusOn = apvts.getRawParameterValue("focusOn")->load() > 0.5f;
    const bool reverbOn = apvts.getRawParameterValue("reverbOn")->load() > 0.5f;
    const bool reverseOn = apvts.getRawParameterValue("reverseOn")->load() > 0.5f;
    const bool cutterOn = apvts.getRawParameterValue("cutterOn")->load() > 0.5f;
    const bool tremoloOn = apvts.getRawParameterValue("tremoloOn")->load() > 0.5f;
    const bool snapBPM = apvts.getRawParameterValue("snapBPM")->load() > 0.5f;
    const bool freeze = apvts.getRawParameterValue("freeze")->load() > 0.5f;
    const bool transientSync = apvts.getRawParameterValue("transientSync")->load() > 0.5f;

    const int playMode = static_cast<int>(apvts.getRawParameterValue("playMode")->load());
    const int spectralMode = static_cast<int>(apvts.getRawParameterValue("spectralMode")->load());
    const int seqPattern = static_cast<int>(apvts.getRawParameterValue("seqPattern")->load());

    {
        const float loopT = mSmoothLoopTime.getTargetValue();
        const float foc = mSmoothFocusFreq.getTargetValue();
        const float rev = mSmoothReverbAmt.getTargetValue();
        const float rvrse = mSmoothReverseAmt.getTargetValue();
        const float cutter = mSmoothCutterRate.getTargetValue();
        const float tremolo = mSmoothTremoloRate.getTargetValue();
        const float pitch = mSmoothPitch.getTargetValue();
        const float duck = mSmoothDuck.getTargetValue();
        const float life = mSmoothLife.getTargetValue();
        const float shapeAmt = mSmoothShapeAmt.getTargetValue();
        const float shapeRate = mSmoothShapeRate.getTargetValue();

        for (auto* eng : { &mCrystalEngineL, &mCrystalEngineR })
        {
            eng->setCrystalParams(loopT, foc, rev, rvrse, cutter, tremolo);
            eng->setEffectsState(focusOn, reverbOn, reverseOn, cutterOn, tremoloOn);
            eng->setBPMSnap(snapBPM, effectiveBPM);
            eng->setFrozen(freeze);
            eng->setPitchSemitones(pitch);
            eng->setDuckAmt(duck);
            eng->setLife(life);
            eng->setPlayMode(static_cast<CrystalEngine::PlayMode>(playMode));
            eng->setSpectralMode(static_cast<CrystalEngine::SpectralMode>(spectralMode));
            eng->setTransientSync(transientSync);
            eng->setShapeAmt(shapeAmt);
            eng->setShapeRate(shapeRate);
            eng->setSeqPattern(seqPattern);
        }
    }

    for (int s = 0; s < N; ++s)
    {
        const float wetDry = mSmoothWetDry.getNextValue();
        const float feedback = mSmoothFeedback.getNextValue();

        mSmoothLoopTime.getNextValue(); mSmoothFocusFreq.getNextValue();
        mSmoothReverbAmt.getNextValue(); mSmoothReverseAmt.getNextValue();
        mSmoothCutterRate.getNextValue(); mSmoothTremoloRate.getNextValue();
        mSmoothPitch.getNextValue(); mSmoothDuck.getNextValue();
        mSmoothLife.getNextValue(); mSmoothShapeAmt.getNextValue(); mSmoothShapeRate.getNextValue();

        const float inL = (numIn > 0) ? buffer.getReadPointer(0)[s] : 0.0f;
        const float inR = (numIn > 1) ? buffer.getReadPointer(1)[s] : inL;

        float wetL_L = 0.0f, wetL_R = 0.0f;
        float wetR_L = 0.0f, wetR_R = 0.0f;
        mCrystalEngineL.process(inL, 0.5f, 0.5f, feedback, wetL_L, wetL_R);
        mCrystalEngineR.process(inR, 0.5f, 0.5f, feedback, wetR_L, wetR_R);

        const float wetL = wetL_L + wetR_L * 0.25f;
        const float wetR = wetL_R * 0.25f + wetR_R;

        const float wetGain = std::sin(wetDry * 1.5707963f);
        const float dryGain = std::cos(wetDry * 1.5707963f);

        if (numOut > 0) buffer.getWritePointer(0)[s] = inL * dryGain + wetL * wetGain;
        if (numOut > 1) buffer.getWritePointer(1)[s] = inR * dryGain + wetR * wetGain;
    }
}

bool NewProjectAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor(*this);
}

void NewProjectAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}