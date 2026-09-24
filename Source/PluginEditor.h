#pragma once

#include "PluginProcessor.h"
#include "ModeSelector.h"
#include "VUMeter.h"
#include "CalPanel.h"
#include "TopBar.h"

// "VU CAL" pill between the meters: shows the current reference and opens the calibration panel.
class CalButton : public juce::Component
{
public:
    explicit CalButton (const Palette& palette);
    std::function<void()> onClick;
    void setState (float refDbfs, bool peakMode, bool panelOpen);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    const Palette& pal;
    float ref = MeterSettings::defaultRefDbfs;
    bool peakMode = false, open = false;
};

// OPERATE lamp + legend. Lit while the mode gain is engaged; click to bypass just that gain.
class OperateButton : public juce::Component
{
public:
    OperateButton (const Palette& palette, juce::RangedAudioParameter& bypassParam);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    const Palette& pal;
    juce::ParameterAttachment attachment;
    bool bypassed = false;
};

// OUTPUT TAME push button: true-peak limiter (ceiling set in the processor). Deliberately shows no gain reduction.
class TameButton : public juce::Component
{
public:
    TameButton (const Palette& palette, juce::RangedAudioParameter& tameParam);
    bool isOn() const { return on; }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    const Palette& pal;
    juce::ParameterAttachment attachment;
    bool on = false;
};

// The whole instrument face, laid out in an 800 x 400 design space and scaled by the editor.
class FacePlate : public juce::Component, private juce::ChangeListener
{
public:
    static constexpr int designWidth = 800, designHeight = 400;

    explicit FacePlate (VibetronProcessor&);
    ~FacePlate() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    // The live (possibly mid-fade) palette, for chrome outside the faceplate.
    const Palette& getPalette() const { return palette; }
    std::function<void()> onPaletteChanged;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { settingsPollCountdown = 0; }  // undo/redo
    void tick (double timestamp);
    void startThemeChange (int newMode);

    VibetronProcessor& proc;
    Palette palette, fromPalette;
    int themeMode = 0;
    float themeT = 1.0f;

    void applyMeterSettings (const MeterSettings&);

    MeterSettings meterSettings;
    Ballistics ballistics;
    int settingsPollCountdown = 0;

    ModeSelector selector;
    CalButton calButton;
    OperateButton operate;
    TameButton tame;
    VUMeter left, right;
    CalPanel calPanel;

    uint32_t lastBlocks = 0;
    double lastTimestamp = 0.0, lastAudioTime = 0.0;
    juce::VBlankAttachment vblank;
};

class VibetronEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int designWidth = FacePlate::designWidth, designHeight = FacePlate::designHeight + TopBar::height;

    explicit VibetronEditor (VibetronProcessor&);
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;  // clicks outside the About card close it

private:
    void setAboutOpen (bool);

    FacePlate face;
    TopBar bar;
    AboutPanel about;
};
