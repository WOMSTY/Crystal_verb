#include "PluginProcessor.h"
#include "PluginEditor.h"

FacetLAF::FacetLAF()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd0e0ff));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, juce::Colour(0xffa0b0c0));
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff1a2a3a));
    setColour(juce::TooltipWindow::textColourId, juce::Colour(0xffe0e0e0));
}

void FacetLAF::drawLabel(juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited()) { g.fillAll(juce::Colour(0xff2a3a4a)); g.setColour(juce::Colour(0xffd0e0ff)); }
    else { g.setColour(label.findColour(juce::Label::textColourId)); }
    g.setFont(label.getFont());
    g.drawFittedText(label.getText(), label.getLocalBounds().reduced(2), juce::Justification::centred, 1);
}

void FacetLAF::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f);
    g.setColour(findColour(juce::TooltipWindow::textColourId));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawFittedText(text, 6, 4, width - 12, height - 8, juce::Justification::centred, 3);
}

void FacetLAF::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float fx = (float)x, fw = (float)width;
    const float fy = (float)y + (float)height * 0.5f, tH = 3.0f;
    g.setColour(juce::Colour(0xff151a20));
    g.fillRoundedRectangle(fx, fy - tH * 0.5f, fw, tH, 1.5f);
    const float fill = juce::jmax(0.0f, sliderPos - fx);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff6e4eff), fx, 0.0f,
        juce::Colour(0xff00e0ff), fx + fw, 0.0f, false));
    g.fillRoundedRectangle(fx, fy - tH * 0.5f, fill, tH, 1.5f);
    const juce::Rectangle<float> thumb(sliderPos - 3.0f, fy - 8.0f, 6.0f, 16.0f);
    g.setColour(juce::Colour(0x337df4ff));
    g.fillRoundedRectangle(thumb.expanded(5.0f, 5.0f), 6.0f);
    g.setColour(juce::Colour(0xffd0e0ff));
    g.fillRoundedRectangle(thumb, 3.0f);
    g.setColour(juce::Colour(0xff4a6aff));
    g.drawRoundedRectangle(thumb, 3.0f, 1.0f);
}

CrystalVisualizer::CrystalVisualizer(NewProjectAudioProcessor& p) : mProcessor(p)
{
    initializeGeometry();
    mLastTimerMs = juce::Time::currentTimeMillis();
    startTimerHz(60);
}

void CrystalVisualizer::initializeGeometry()
{
    vertices = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
    faces = {
        {{0,2,4}, juce::Colour::fromHSV(0.00f,0.8f,0.9f,0.5f)}, {{2,1,4}, juce::Colour::fromHSV(0.12f,0.8f,0.9f,0.5f)},
        {{1,3,4}, juce::Colour::fromHSV(0.25f,0.8f,0.9f,0.5f)}, {{3,0,4}, juce::Colour::fromHSV(0.37f,0.8f,0.9f,0.5f)},
        {{0,3,5}, juce::Colour::fromHSV(0.50f,0.8f,0.9f,0.5f)}, {{3,1,5}, juce::Colour::fromHSV(0.62f,0.8f,0.9f,0.5f)},
        {{1,2,5}, juce::Colour::fromHSV(0.75f,0.8f,0.9f,0.5f)}, {{2,0,5}, juce::Colour::fromHSV(0.87f,0.8f,0.9f,0.5f)}
    };
}

CrystalVisualizer::Point3D CrystalVisualizer::rotatePoint(Point3D p, float ax, float ay)
{
    const float x1 = p.x * std::cos(ay) - p.z * std::sin(ay);
    const float z1 = p.x * std::sin(ay) + p.z * std::cos(ay);
    const float y2 = p.y * std::cos(ax) - z1 * std::sin(ax);
    const float z2 = p.y * std::sin(ax) + z1 * std::cos(ax);
    return { x1, y2, z2 };
}

void CrystalVisualizer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto center = bounds.getCentre();
    const float time = (float)juce::Time::getMillisecondCounter() * 0.001f;
    const float colorMix = *mProcessor.apvts.getRawParameterValue("focusFreq");
    const float sizeParam = *mProcessor.apvts.getRawParameterValue("loopTime");
    const float shimmer = *mProcessor.apvts.getRawParameterValue("reverseAmt");
    const bool isFrozen = mProcessor.apvts.getRawParameterValue("freeze")->load() > 0.5f;
    const int playMode = static_cast<int>(mProcessor.apvts.getRawParameterValue("playMode")->load());

    juce::ColourGradient bg(juce::Colour(0xff060a10), center.x, center.y,
        juce::Colour(0xff020305), center.x, center.y + bounds.getHeight() * 0.7f, false);
    g.setGradientFill(bg); g.fillAll();
    if (isFrozen) { g.setColour(juce::Colour(0x220055ff)); g.fillAll(); }

    angleY += std::sin(time * 0.3f) * 0.0015f;
    const float baseScale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    const float loopNorm = juce::jlimit(0.0f, 1.0f, (std::log(sizeParam / 0.1f) / std::log(80.0f)));
    const float scale = baseScale * (0.8f + loopNorm * 0.4f);

    struct ProjVertex { float x, y, z; };
    std::vector<ProjVertex> proj;
    for (const auto& v : vertices)
    {
        Point3D r = rotatePoint(v, angleX, angleY);
        float depth = (r.z + 3.0f) * 0.333f;
        proj.push_back({ r.x * scale / depth + center.x, r.y * scale / depth + center.y, r.z });
    }

    struct PF { juce::Point<float> p[3]; juce::Colour color; float zDepth; };
    std::vector<PF> pFaces;
    for (const auto& f : faces)
    {
        const auto& p0 = proj[f.v[0]]; const auto& p1 = proj[f.v[1]]; const auto& p2 = proj[f.v[2]];
        float zDepth = (p0.z + p1.z + p2.z) / 3.0f;
        juce::Colour col = f.color.withSaturation(0.3f + colorMix * 0.7f);
        if (isFrozen) col = col.interpolatedWith(juce::Colour(0xff2255ff), 0.4f);
        if (playMode == 1) col = col.interpolatedWith(juce::Colour(0xffffaa00), 0.3f); // Scan = orange
        if (playMode == 2) col = col.interpolatedWith(juce::Colour(0xff00ff88), 0.3f); // Seq = green
        pFaces.push_back({ {juce::Point<float>(p0.x,p0.y), juce::Point<float>(p1.x,p1.y), juce::Point<float>(p2.x,p2.y)}, col, zDepth });
    }
    std::sort(pFaces.begin(), pFaces.end(), [](const PF& a, const PF& b) { return a.zDepth < b.zDepth; });

    for (const auto& pf : pFaces)
    {
        juce::Path path; path.startNewSubPath(pf.p[0]); path.lineTo(pf.p[1]); path.lineTo(pf.p[2]); path.closeSubPath();
        const float depthMod = juce::jlimit(0.15f, 1.0f, (pf.zDepth + 1.0f) * 0.5f);
        const float alphaFill = 0.08f + depthMod * 0.12f + shimmer * 0.15f;
        g.setColour(pf.color.withAlpha(alphaFill)); g.fillPath(path);
        g.setColour(pf.color.withAlpha(0.3f + depthMod * 0.6f));
        g.strokePath(path, juce::PathStrokeType(1.0f + depthMod * 1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    {
        const float rmsNorm = juce::jlimit(0.0f, 1.0f, mSmoothedRms * 16.0f);
        const float haloR = scale * (0.7f + rmsNorm * 0.8f);
        for (int ring = 3; ring >= 1; --ring)
        {
            const float ringR = haloR * (0.7f + ring * 0.12f);
            const float alpha = rmsNorm * (0.25f / ring);
            g.setColour(juce::Colour(0xffffc44a).withAlpha(alpha));
            g.drawEllipse(center.x - ringR, center.y - ringR, ringR * 2.0f, ringR * 2.0f, 1.5f + ring * 0.5f);
        }
    }

    {
        const float arcR = scale * 1.1f;
        const float startAng = -juce::MathConstants<float>::halfPi;
        const float sweepAng = mLoopPhase * juce::MathConstants<float>::twoPi;
        juce::Path arcPath;
        arcPath.addArc(center.x - arcR, center.y - arcR, arcR * 2.0f, arcR * 2.0f, startAng, startAng + sweepAng, true);
        const juce::Colour arcColour = isFrozen ? juce::Colour(0xff22aaff) : juce::Colour(0xff00e5ff);
        g.setColour(arcColour.withAlpha(0.85f));
        g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float endX = center.x + arcR * std::cos(startAng + sweepAng);
        const float endY = center.y + arcR * std::sin(startAng + sweepAng);
        g.setColour(arcColour); g.fillEllipse(endX - 4.0f, endY - 4.0f, 8.0f, 8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.9f)); g.fillEllipse(endX - 2.0f, endY - 2.0f, 4.0f, 4.0f);
    }

    const auto particles = mProcessor.getParticleSnapshot();
    for (const auto& p : particles)
    {
        if (!p.active) continue;
        Point3D r = rotatePoint({ p.x, p.y, p.z }, angleX, angleY);
        const float depth = (r.z + 3.0f) * 0.333f;
        const float px = center.x + r.x * scale / depth;
        const float py = center.y + r.y * scale / depth;
        const float en = juce::jlimit(0.0f, 1.0f, p.energy);
        const float rad = 2.0f + en * 4.0f;
        juce::Colour pc = juce::Colours::white;
        if (p.lastFaceHit >= 0 && p.lastFaceHit < (int)faces.size()) pc = faces[p.lastFaceHit].color;
        g.setColour(pc.withAlpha(0.2f + en * 0.4f));
        g.fillEllipse(px - rad * 2.0f, py - rad * 2.0f, rad * 4.0f, rad * 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f);
    }

    if (isFrozen)
    {
        g.setColour(juce::Colour(0xcc22aaff));
        g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f).withStyle("Bold")));
        g.drawFittedText("* FROZEN", bounds.reduced(10.0f).toNearestInt(), juce::Justification::topRight, 1);
    }
    if (mProcessor.mHostPlaying.load())
    {
        const float hostBPM = mProcessor.mHostBPM.load();
        g.setColour(juce::Colour(0xffffc44a).withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawFittedText("DAW " + juce::String(hostBPM, 1) + " BPM", bounds.reduced(10.0f).toNearestInt(), juce::Justification::topLeft, 1);
    }
}

void CrystalVisualizer::resized() {}
void CrystalVisualizer::mouseDown(const juce::MouseEvent& e) { lastMousePos = e.getPosition(); spinX = spinY = 0.0f; }
void CrystalVisualizer::mouseDrag(const juce::MouseEvent& e)
{
    const auto delta = e.getPosition() - lastMousePos;
    angleY += (float)delta.x * 0.008f; angleX += (float)delta.y * 0.008f;
    lastMousePos = e.getPosition();
    mProcessor.getCrystalEngine().setCrystalSpin((float)delta.x * 0.003f);
}
void CrystalVisualizer::mouseUp(const juce::MouseEvent& e)
{
    const auto delta = e.getPosition() - lastMousePos;
    spinY = (float)delta.x * 0.012f; spinX = (float)delta.y * 0.012f;
}
void CrystalVisualizer::timerCallback()
{
    angleX += spinX; angleY += spinY; spinX *= 0.92f; spinY *= 0.92f;
    if (std::abs(spinX) < 0.001f && std::abs(spinY) < 0.001f)
        spinY = std::sin((float)juce::Time::getMillisecondCounter() * 0.001f) * 0.004f;

    const juce::int64 nowMs = juce::Time::currentTimeMillis();
    const float dtSec = (float)(nowMs - mLastTimerMs) * 0.001f;
    mLastTimerMs = nowMs;
    const float loopTimeSec = mProcessor.getLoopTimeSec();
    if (loopTimeSec > 0.01f) { mLoopPhase += dtSec / loopTimeSec; if (mLoopPhase >= 1.0f) mLoopPhase -= 1.0f; }

    const float targetRms = mProcessor.getOutputRms();
    mSmoothedRms += 0.15f * (targetRms - mSmoothedRms);
    repaint();
}

void NewProjectAudioProcessorEditor::buildPresets()
{
    mPresets = {
        { "Init", 0.50f, 1.00f, 0.50f, 0.20f, 0.20f, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 120.0f, 0.30f, 0.0f, 1.0f, 0, 1, 0, true, true, true, true, true, false, false, false },
        { "Cloud Drone", 0.90f, 5.00f, 0.40f, 0.75f, 0.60f, 0.20f, 0.00f, 0.10f, 0.0f, 0.00f, 90.0f, 0.80f, 0.0f, 0.5f, 0, 1, 0, false, true, false, false, true, false, false, false },
        { "Scan Arp", 0.70f, 1.50f, 0.60f, 0.40f, 0.30f, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 128.0f, 0.20f, 0.4f, 2.0f, 1, 0, 0, true, false, false, false, false, true, false, false },
        { "Seq Glitch", 0.85f, 0.25f, 0.55f, 0.65f, 0.15f, 0.60f, 0.70f, 0.00f, 0.0f, 0.20f, 140.0f, 0.40f, 0.6f, 4.0f, 2, 3, 4, true, false, true, true, false, true, false, false },
        { "Harmonic Pad", 0.80f, 3.00f, 0.35f, 0.70f, 0.50f, 0.00f, 0.00f, 0.15f, -5.0f, 0.00f, 110.0f, 0.50f, 0.2f, 0.3f, 0, 0, 0, true, true, false, false, true, false, false, false },
        { "Octave Bass", 0.75f, 2.00f, 0.25f, 0.55f, 0.10f, 0.00f, 0.00f, 0.00f, -12.0f, 0.30f, 130.0f, 0.10f, 0.0f, 0.1f, 0, 2, 0, true, false, false, false, false, true, false, false },
        { "Transient Slice", 0.65f, 0.50f, 0.70f, 0.30f, 0.20f, 0.40f, 0.20f, 0.00f, 0.0f, 0.10f, 174.0f, 0.60f, 0.3f, 8.0f, 0, 1, 1, true, false, true, true, false, true, false, true },
        { "Resonant String", 0.85f, 4.00f, 0.45f, 0.85f, 0.10f, 0.00f, 0.00f, 0.05f, 0.0f, 0.00f, 100.0f, 0.20f, 0.0f, 0.2f, 3, 0, 0, false, true, false, false, true, false, false, false },
    };

    mPresetBox.clear(juce::dontSendNotification);
    mPresetBox.addItem(juce::String("\u2014 Preset \u2014"), 1);
    for (int i = 0; i < (int)mPresets.size(); ++i) mPresetBox.addItem(mPresets[i].name, i + 2);
    mPresetBox.setSelectedId(1, juce::dontSendNotification);
}

void NewProjectAudioProcessorEditor::loadPreset(int index)
{
    if (index < 0 || index >= (int)mPresets.size()) return;
    const auto& p = mPresets[index];
    auto setParam = [this](const juce::String& id, float value) {
        if (auto* param = mProcessor.apvts.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(value));
            param->endChangeGesture();
        }
        };
    auto setBoolParam = [this](const juce::String& id, bool value) {
        if (auto* param = mProcessor.apvts.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(value ? 1.0f : 0.0f);
            param->endChangeGesture();
        }
        };
    auto setChoice = [this](const juce::String& id, int value) {
        if (auto* param = mProcessor.apvts.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
            param->endChangeGesture();
        }
        };

    setParam("wetDry", p.wetDry); setParam("loopTime", p.loopTime); setParam("focusFreq", p.focusFreq);
    setParam("feedback", p.feedback); setParam("reverbAmt", p.reverbAmt); setParam("reverseAmt", p.reverseAmt);
    setParam("cutterRate", p.cutterRate); setParam("tremoloRate", p.tremoloRate);
    setParam("pitchSemitones", p.pitch); setParam("duckAmt", p.duck); setParam("bpm", p.bpm);
    setParam("life", p.life); setParam("shapeAmt", p.shapeAmt); setParam("shapeRate", p.shapeRate);
    setChoice("playMode", p.playMode); setChoice("spectralMode", p.spectralMode); setChoice("seqPattern", p.seqPattern);
    setBoolParam("focusOn", p.focusOn); setBoolParam("reverbOn", p.reverbOn); setBoolParam("reverseOn", p.reverseOn);
    setBoolParam("cutterOn", p.cutterOn); setBoolParam("tremoloOn", p.tremoloOn);
    setBoolParam("snapBPM", p.snapBPM); setBoolParam("freeze", p.freeze); setBoolParam("transientSync", p.transientSync);
    updateLoopTimeSubLabel();
}

NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor(NewProjectAudioProcessor& p)
    : AudioProcessorEditor(&p), mProcessor(p), mCrystalVis(p)
{
    setLookAndFeel(&mLAF);
    addAndMakeVisible(mCrystalVis);

    setupSlider(mSlWetDry, mLblWetDry, "Blend dry/wet à puissance constante");
    setupSlider(mSlLoopTime, mLblLoopTime, "Durée du buffer de boucle");
    setupSlider(mSlFocusFreq, mLblFocusFreq, "Filtre formant résonant");
    setupSlider(mSlFeedback, mLblFeedback, "Régénération du buffer");
    setupSlider(mSlReverbAmt, mLblReverbAmt, "Mix reverb + shimmer");
    setupSlider(mSlReverseAmt, mLblReverseAmt, "Probabilité de grains inversés");
    setupSlider(mSlCutterRate, mLblCutterRate, "Gate LFO carré");
    setupSlider(mSlTremoloRate, mLblTremoloRate, "Tremolo sinusoïdal");
    setupSlider(mSlBPM, mLblBPM, "Tempo de référence");
    setupSlider(mSlPitch, mLblPitch, "Transposition globale");
    setupSlider(mSlDuck, mLblDuck, "Atténuation auto quand input fort");
    setupSlider(mSlLife, mLblLife, "Intensité organique et chaos");
    setupSlider(mSlShapeAmt, mLblShapeAmt, "Intensité macro-modulation");
    setupSlider(mSlShapeRate, mLblShapeRate, "Vitesse macro-modulation");

    mSlBPM.setRange(60.0, 200.0, 1.0);
    mSlPitch.setRange(-12.0, 12.0, 1.0);

    mAttWetDry = std::make_unique<Attachment>(mProcessor.apvts, "wetDry", mSlWetDry);
    mAttLoopTime = std::make_unique<Attachment>(mProcessor.apvts, "loopTime", mSlLoopTime);
    mAttFocusFreq = std::make_unique<Attachment>(mProcessor.apvts, "focusFreq", mSlFocusFreq);
    mAttFeedback = std::make_unique<Attachment>(mProcessor.apvts, "feedback", mSlFeedback);
    mAttReverbAmt = std::make_unique<Attachment>(mProcessor.apvts, "reverbAmt", mSlReverbAmt);
    mAttReverseAmt = std::make_unique<Attachment>(mProcessor.apvts, "reverseAmt", mSlReverseAmt);
    mAttCutterRate = std::make_unique<Attachment>(mProcessor.apvts, "cutterRate", mSlCutterRate);
    mAttTremoloRate = std::make_unique<Attachment>(mProcessor.apvts, "tremoloRate", mSlTremoloRate);
    mAttBPM = std::make_unique<Attachment>(mProcessor.apvts, "bpm", mSlBPM);
    mAttPitch = std::make_unique<Attachment>(mProcessor.apvts, "pitchSemitones", mSlPitch);
    mAttDuck = std::make_unique<Attachment>(mProcessor.apvts, "duckAmt", mSlDuck);
    mAttLife = std::make_unique<Attachment>(mProcessor.apvts, "life", mSlLife);
    mAttShapeAmt = std::make_unique<Attachment>(mProcessor.apvts, "shapeAmt", mSlShapeAmt);
    mAttShapeRate = std::make_unique<Attachment>(mProcessor.apvts, "shapeRate", mSlShapeRate);

    mLblLoopTimeSub.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    mLblLoopTimeSub.setJustificationType(juce::Justification::centredLeft);
    mLblLoopTimeSub.setColour(juce::Label::textColourId, juce::Colour(0xffffc44a));
    addAndMakeVisible(mLblLoopTimeSub);
    mSlLoopTime.onValueChange = [this] { updateLoopTimeSubLabel(); };
    mSlBPM.onValueChange = [this] { updateLoopTimeSubLabel(); };
    updateLoopTimeSubLabel();

    auto setupBtn = [this](juce::TextButton& btn) {
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2030));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6e4eff));
        addAndMakeVisible(btn);
        };
    setupBtn(mBtnFocus); setupBtn(mBtnReverb); setupBtn(mBtnReverse);
    setupBtn(mBtnCutter); setupBtn(mBtnTremolo);

    mBtnSnapBPM.setClickingTogglesState(true);
    mBtnSnapBPM.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2030));
    mBtnSnapBPM.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffaa00));
    addAndMakeVisible(mBtnSnapBPM);

    mBtnFreeze.setButtonText("*");
    mBtnFreeze.setClickingTogglesState(true);
    mBtnFreeze.setTooltip("Freeze : fige le buffer");
    mBtnFreeze.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2030));
    mBtnFreeze.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff0077ee));
    mBtnFreeze.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff4488bb));
    mBtnFreeze.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(mBtnFreeze);

    mBtnTransientSync.setButtonText("SYNC");
    mBtnTransientSync.setClickingTogglesState(true);
    mBtnTransientSync.setTooltip("Synchronise le spawn des grains sur les transitoires");
    mBtnTransientSync.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2030));
    mBtnTransientSync.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00cc66));
    addAndMakeVisible(mBtnTransientSync);

    mAttBtnFocus = std::make_unique<ButtonAttachment>(mProcessor.apvts, "focusOn", mBtnFocus);
    mAttBtnReverb = std::make_unique<ButtonAttachment>(mProcessor.apvts, "reverbOn", mBtnReverb);
    mAttBtnReverse = std::make_unique<ButtonAttachment>(mProcessor.apvts, "reverseOn", mBtnReverse);
    mAttBtnCutter = std::make_unique<ButtonAttachment>(mProcessor.apvts, "cutterOn", mBtnCutter);
    mAttBtnTremolo = std::make_unique<ButtonAttachment>(mProcessor.apvts, "tremoloOn", mBtnTremolo);
    mAttBtnSnapBPM = std::make_unique<ButtonAttachment>(mProcessor.apvts, "snapBPM", mBtnSnapBPM);
    mAttBtnFreeze = std::make_unique<ButtonAttachment>(mProcessor.apvts, "freeze", mBtnFreeze);
    mAttBtnTransientSync = std::make_unique<ButtonAttachment>(mProcessor.apvts, "transientSync", mBtnTransientSync);

    mBtnSnapBPM.onClick = [this] { updateLoopTimeSubLabel(); resized(); };

    auto setupCombo = [this](juce::ComboBox& cb, juce::Label& lbl, const juce::String&){
        cb.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a2030));
        cb.setColour(juce::ComboBox::textColourId, juce::Colour(0xffd0e0ff));
        cb.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff304050));
        cb.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff6e4eff));
        lbl.setFont(juce::Font(juce::FontOptions().withHeight(10.5f).withStyle("Bold")));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xff90a8c0));
        lbl.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(cb); addAndMakeVisible(lbl);
        };

    setupCombo(mCbPlayMode, mLblPlayMode, "MODE");
    mCbPlayMode.addItem("Cloud", 1); mCbPlayMode.addItem("Scan", 2);
    mCbPlayMode.addItem("Sequence", 3); mCbPlayMode.addItem("Resonant", 4);

    setupCombo(mCbSpectral, mLblSpectral, "SPECTRAL");
    mCbSpectral.addItem("Harmonic", 1); mCbSpectral.addItem("Inharmonic", 2);
    mCbSpectral.addItem("Octave", 3); mCbSpectral.addItem("Chaos", 4);

    setupCombo(mCbSeqPattern, mLblSeqPattern, "PATTERN");
    mCbSeqPattern.addItem("All", 1); mCbSeqPattern.addItem("1/8", 2); mCbSeqPattern.addItem("1/4", 3);
    mCbSeqPattern.addItem("1/2", 4); mCbSeqPattern.addItem("Groove A", 5); mCbSeqPattern.addItem("Offbeat", 6);
    mCbSeqPattern.addItem("Shuffle", 7); mCbSeqPattern.addItem("Triplet", 8); mCbSeqPattern.addItem("Break", 9);
    mCbSeqPattern.addItem("Complex", 10); mCbSeqPattern.addItem("Syncop", 11); mCbSeqPattern.addItem("Sparse", 12);
    mCbSeqPattern.addItem("Blocks", 13); mCbSeqPattern.addItem("Build", 14); mCbSeqPattern.addItem("Var", 15);
    mCbSeqPattern.addItem("Tribal", 16);

    mCbPlayMode.onChange = [this] {
        if (auto* param = mProcessor.apvts.getParameter("playMode"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(mCbPlayMode.getSelectedId() - 1)));
        };
    mCbSpectral.onChange = [this] {
        if (auto* param = mProcessor.apvts.getParameter("spectralMode"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(mCbSpectral.getSelectedId() - 1)));
        };
    mCbSeqPattern.onChange = [this] {
        if (auto* param = mProcessor.apvts.getParameter("seqPattern"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(mCbSeqPattern.getSelectedId() - 1)));
        };

    buildPresets();
    mPresetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a2030));
    mPresetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffd0e0ff));
    mPresetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff304050));
    mPresetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff6e4eff));
    mPresetBox.onChange = [this] { const int selId = mPresetBox.getSelectedId(); if (selId >= 2) loadPreset(selId - 2); };
    addAndMakeVisible(mPresetBox);

    mLblPreset.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    mLblPreset.setColour(juce::Label::textColourId, juce::Colour(0xff6e4eff));
    mLblPreset.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mLblPreset);

    setSize(1200, 740);
    setResizable(false, false);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() { setLookAndFeel(nullptr); }

void NewProjectAudioProcessorEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& tooltip)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setRange(0.0, 1.0, 0.01);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
    s.setTooltip(tooltip);
    addAndMakeVisible(s);
    l.setFont(juce::Font(juce::FontOptions().withHeight(10.5f).withStyle("Bold")));
    l.setJustificationType(juce::Justification::centredLeft);
    l.setColour(juce::Label::textColourId, juce::Colour(0xff90a8c0));
    l.setTooltip(tooltip);
    addAndMakeVisible(l);
}

void NewProjectAudioProcessorEditor::updateLoopTimeSubLabel()
{
    const float loopSec = (float)mSlLoopTime.getValue();
    const bool snapOn = mBtnSnapBPM.getToggleState();
    const float bpm = (float)mSlBPM.getValue();
    juce::String text;
    if (snapOn && bpm > 0.0f)
    {
        const float spb = 60.0f / bpm;
        const float divsSec[] = { spb * 0.5f, spb, spb * 2.0f, spb * 4.0f, spb * 8.0f, spb * 16.0f };
        const char* names[] = { "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
        int best = 0; float bestD = std::abs(loopSec - divsSec[0]);
        for (int i = 1; i < 6; ++i) if (std::abs(loopSec - divsSec[i]) < bestD) { bestD = std::abs(loopSec - divsSec[i]); best = i; }
        text = juce::String("SNAP ") + names[best] + "  @" + juce::String((int)bpm) + " BPM";
    }
    else text = loopSec < 1.0f ? juce::String((int)(loopSec * 1000.0f)) + " ms" : juce::String(loopSec, 2) + " s";
    mLblLoopTimeSub.setText(text, juce::dontSendNotification);
}

void NewProjectAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xff0a121a), 0.0f, 0.0f, juce::Colour(0xff050a10), 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bg); g.fillAll();

    g.setColour(juce::Colour(0x55101820)); g.fillRect(0.0f, 0.0f, bounds.getWidth(), 40.0f);
    g.setColour(juce::Colour(0xffd0e0ff));
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f).withStyle("Bold")));
    g.drawFittedText("CRYSTAL VERB", 20, 8, 160, 24, juce::Justification::left, 1);
    g.setColour(juce::Colour(0xff506880));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawFittedText("Particle Crystal Reverb", 184, 12, 190, 18, juce::Justification::left, 1);
    g.setColour(juce::Colour(0xffffc44a));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.5f).withStyle("Italic")));
    g.drawFittedText("Made by Ely", 378, 13, 110, 14, juce::Justification::left, 1);
    g.setColour(juce::Colour(0x226e8eff)); g.drawLine(0.0f, 40.0f, bounds.getWidth(), 40.0f);

    const int panelTop = (int)bounds.getHeight() - 250;
    g.setColour(juce::Colour(0x33304050)); g.fillRect(0, panelTop, (int)bounds.getWidth(), 1);
    g.setColour(juce::Colour(0xff3a5a7a)); g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText("LOOP & SPACE", 20, panelTop + 4, 150, 14, juce::Justification::left, 1);
    g.drawFittedText("TIMBRE", (int)bounds.getWidth() / 4 + 10, panelTop + 4, 150, 14, juce::Justification::left, 1);
    g.drawFittedText("MODULATION", (int)bounds.getWidth() / 4 * 2 + 10, panelTop + 4, 150, 14, juce::Justification::left, 1);
    g.drawFittedText("MIX & PITCH", (int)bounds.getWidth() / 4 * 3 + 10, panelTop + 4, 150, 14, juce::Justification::left, 1);
    g.setColour(juce::Colour(0x22304050));
    for (int col = 1; col < 4; ++col)
        g.drawLine((float)(col * (int)bounds.getWidth() / 4), (float)panelTop, (float)(col * (int)bounds.getWidth() / 4), bounds.getHeight(), 1.0f);
}

void NewProjectAudioProcessorEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    const int panelH = 250, panelTop = H - panelH;

    mLblPreset.setBounds(W - 292, 10, 56, 20);
    mPresetBox.setBounds(W - 234, 7, 222, 26);
    mBtnFreeze.setBounds(W - 326, 7, 24, 26);

    mCrystalVis.setBounds(0, 40, W, panelTop - 40);

    const int colW = W / 4, padX = 14, startY = panelTop + 20;
    const int labelH = 13, subLH = 11, sliderH = 20, btnH = 14, rowH = labelH + subLH + sliderH + 8;

    // Col 0 : Loop Time + Snap + Reverb + Mode/Spectral
    {
        const int x = padX, w = colW - padX * 2;
        mLblLoopTime.setBounds(x, startY, w - 50, labelH);
        mBtnSnapBPM.setBounds(x + w - 46, startY, 46, labelH);
        mLblLoopTimeSub.setBounds(x, startY + labelH, w, subLH);
        mSlLoopTime.setBounds(x, startY + labelH + subLH, w, sliderH);

        const int y1 = startY + rowH;
        mLblReverbAmt.setBounds(x, y1, w - 22, labelH);
        mBtnReverb.setBounds(x + w - 20, y1, 20, btnH);
        mSlReverbAmt.setBounds(x, y1 + labelH + subLH, w, sliderH);

        const int y2 = startY + rowH * 2;
        mLblPlayMode.setBounds(x, y2, 40, labelH);
        mCbPlayMode.setBounds(x + 42, y2 - 2, w - 44, sliderH);
        const int y3 = y2 + sliderH + 4;
        mLblSpectral.setBounds(x, y3, 40, labelH);
        mCbSpectral.setBounds(x + 42, y3 - 2, w - 44, sliderH);

        const bool snapOn = mBtnSnapBPM.getToggleState();
        mLblBPM.setVisible(snapOn); mSlBPM.setVisible(snapOn);
        if (snapOn)
        {
            const int y4 = y3 + sliderH + 4;
            mLblBPM.setBounds(x, y4, w, labelH);
            mSlBPM.setBounds(x, y4 + labelH + subLH, w, sliderH);
        }
    }

    // Col 1 : Focus + Reverse + Duck + Pattern
    {
        const int x = colW + padX, w = colW - padX * 2;
        mLblFocusFreq.setBounds(x, startY, w - 22, labelH);
        mBtnFocus.setBounds(x + w - 20, startY, 20, btnH);
        mSlFocusFreq.setBounds(x, startY + labelH + subLH, w, sliderH);

        const int y1 = startY + rowH;
        mLblReverseAmt.setBounds(x, y1, w - 22, labelH);
        mBtnReverse.setBounds(x + w - 20, y1, 20, btnH);
        mSlReverseAmt.setBounds(x, y1 + labelH + subLH, w, sliderH);

        const int y2 = startY + rowH * 2;
        mLblDuck.setBounds(x, y2, w, labelH);
        mSlDuck.setBounds(x, y2 + labelH + subLH, w, sliderH);

        const int y3 = y2 + rowH;
        mLblSeqPattern.setBounds(x, y3, 40, labelH);
        mCbSeqPattern.setBounds(x + 42, y3 - 2, w - 44, sliderH);
    }

    // Col 2 : Cutter + Tremolo + Life + Shape
    {
        const int x = colW * 2 + padX, w = colW - padX * 2;
        mLblCutterRate.setBounds(x, startY, w - 22, labelH);
        mBtnCutter.setBounds(x + w - 20, startY, 20, btnH);
        mSlCutterRate.setBounds(x, startY + labelH + subLH, w, sliderH);

        const int y1 = startY + rowH;
        mLblTremoloRate.setBounds(x, y1, w - 22, labelH);
        mBtnTremolo.setBounds(x + w - 20, y1, 20, btnH);
        mSlTremoloRate.setBounds(x, y1 + labelH + subLH, w, sliderH);

        const int y2 = startY + rowH * 2;
        mLblLife.setBounds(x, y2, w, labelH);
        mSlLife.setBounds(x, y2 + labelH + subLH, w, sliderH);

        const int y3 = y2 + rowH;
        mLblShapeAmt.setBounds(x, y3, w / 2 - 2, labelH);
        mLblShapeRate.setBounds(x + w / 2 + 2, y3, w / 2 - 2, labelH);
        mSlShapeAmt.setBounds(x, y3 + labelH + subLH, w / 2 - 2, sliderH);
        mSlShapeRate.setBounds(x + w / 2 + 2, y3 + labelH + subLH, w / 2 - 2, sliderH);
    }

    // Col 3 : Feedback + WetDry + Pitch + Sync
    {
        const int x = colW * 3 + padX, w = colW - padX * 2;
        mLblFeedback.setBounds(x, startY, w - 30, labelH);
        mBtnTransientSync.setBounds(x + w - 28, startY, 28, btnH);
        mSlFeedback.setBounds(x, startY + labelH + subLH, w, sliderH);

        const int y1 = startY + rowH;
        mLblWetDry.setBounds(x, y1, w, labelH);
        mSlWetDry.setBounds(x, y1 + labelH + subLH, w, sliderH);

        const int y2 = startY + rowH * 2;
        mLblPitch.setBounds(x, y2, w, labelH);
        mSlPitch.setBounds(x, y2 + labelH + subLH, w, sliderH);
    }
}