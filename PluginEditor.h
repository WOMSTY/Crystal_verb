#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FacetLAF : public juce::LookAndFeel_V4
{
public:
    FacetLAF();
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        juce::Slider::SliderStyle, juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawTooltip(juce::Graphics&, const juce::String& text, int w, int h) override;
};

class CrystalVisualizer : public juce::Component, public juce::Timer
{
public:
    explicit CrystalVisualizer(NewProjectAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void timerCallback() override;
private:
    NewProjectAudioProcessor& mProcessor;
    struct Point3D { float x, y, z; };
    struct Face3D { int v[3]; juce::Colour color; };
    std::vector<Point3D> vertices;
    std::vector<Face3D> faces;
    float angleX = 0.0f, angleY = 0.0f, spinX = 0.0f, spinY = 0.0f;
    juce::Point<int> lastMousePos;
    float mLoopPhase = 0.0f; juce::int64 mLastTimerMs = 0; float mSmoothedRms = 0.0f;
    void initializeGeometry();
    Point3D rotatePoint(Point3D p, float ax, float ay);
};

class NewProjectAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NewProjectAudioProcessorEditor(NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    NewProjectAudioProcessor& mProcessor;
    FacetLAF mLAF;
    CrystalVisualizer mCrystalVis;

    juce::Slider mSlWetDry, mSlLoopTime, mSlFocusFreq, mSlFeedback;
    juce::Slider mSlReverbAmt, mSlReverseAmt, mSlCutterRate, mSlTremoloRate;
    juce::Slider mSlBPM, mSlPitch, mSlDuck, mSlLife, mSlShapeAmt, mSlShapeRate;

    juce::Label mLblWetDry{ {}, "WET / DRY" };
    juce::Label mLblLoopTime{ {}, "LOOP TIME" };
    juce::Label mLblLoopTimeSub{ {}, "" };
    juce::Label mLblFocusFreq{ {}, "FOCUS FREQ" };
    juce::Label mLblFeedback{ {}, "FEEDBACK" };
    juce::Label mLblReverbAmt{ {}, "REVERB" };
    juce::Label mLblReverseAmt{ {}, "REVERSE" };
    juce::Label mLblCutterRate{ {}, "CUTTER" };
    juce::Label mLblTremoloRate{ {}, "TREMOLO" };
    juce::Label mLblBPM{ {}, "BPM" };
    juce::Label mLblPitch{ {}, "PITCH" };
    juce::Label mLblDuck{ {}, "DUCK" };
    juce::Label mLblLife{ {}, "LIFE" };
    juce::Label mLblShapeAmt{ {}, "SHAPE" };
    juce::Label mLblShapeRate{ {}, "RATE" };

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> mAttWetDry, mAttLoopTime, mAttFocusFreq, mAttFeedback;
    std::unique_ptr<Attachment> mAttReverbAmt, mAttReverseAmt, mAttCutterRate, mAttTremoloRate;
    std::unique_ptr<Attachment> mAttBPM, mAttPitch, mAttDuck, mAttLife, mAttShapeAmt, mAttShapeRate;

    juce::TextButton mBtnFocus{ "ON" }, mBtnReverb{ "ON" }, mBtnReverse{ "ON" };
    juce::TextButton mBtnCutter{ "ON" }, mBtnTremolo{ "ON" }, mBtnSnapBPM{ "SNAP" };
    juce::TextButton mBtnFreeze{ "*" }, mBtnTransientSync{ "SYNC" };

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> mAttBtnFocus, mAttBtnReverb, mAttBtnReverse;
    std::unique_ptr<ButtonAttachment> mAttBtnCutter, mAttBtnTremolo, mAttBtnSnapBPM;
    std::unique_ptr<ButtonAttachment> mAttBtnFreeze, mAttBtnTransientSync;

    juce::ComboBox mCbPlayMode, mCbSpectral, mCbSeqPattern, mPresetBox;
    juce::Label mLblPlayMode{ {}, "MODE" };
    juce::Label mLblSpectral{ {}, "SPECTRAL" };
    juce::Label mLblSeqPattern{ {}, "PATTERN" };
    juce::Label mLblPreset{ {}, "PRESET" };

    struct Preset {
        juce::String name;
        float wetDry, loopTime, focusFreq, feedback, reverbAmt, reverseAmt;
        float cutterRate, tremoloRate, pitch, duck, bpm, life, shapeAmt, shapeRate;
        int playMode, spectralMode, seqPattern;
        bool focusOn, reverbOn, reverseOn, cutterOn, tremoloOn, snapBPM, freeze, transientSync;
    };
    std::vector<Preset> mPresets;
    void buildPresets();
    void loadPreset(int index);

    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& tooltip);
    void updateLoopTimeSubLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectAudioProcessorEditor)
};