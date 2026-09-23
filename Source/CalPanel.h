#pragma once

#include "Theme.h"
#include "MeterSettings.h"

// Small pill-shaped glass button used in the calibration panel.
class PillButton : public juce::Component
{
public:
    PillButton (const Palette& palette, juce::String text);
    std::function<void()> onClick;
    void setLit (bool shouldBeLit) { if (lit != shouldBeLit) { lit = shouldBeLit; repaint(); } }
    int getIdealWidth() const;  // text plus even side padding
    static constexpr int height = 16;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    const Palette& pal;
    juce::String text;
    bool lit = false;
};

// Horizontal slider: gold track, machined thumb, default detent mark. Double-click resets; Shift-drag is fine.
class CalSlider : public juce::Component
{
public:
    // skewForDefault: map the track so the default value sits in the middle (for ranges that crowd one end).
    CalSlider (const Palette& palette, juce::String label, float minValue, float maxValue, float step, float defaultValue,
               std::function<juce::String (float)> format, bool skewForDefault = false);

    std::function<void (float)> onChange;
    void setValue (float v, bool notify);
    float getValue() const { return value; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr float labelWidth = 92.0f, valueWidth = 50.0f;

private:
    juce::Rectangle<float> trackArea() const;
    float valueToX (float v) const;
    float xToValue (float x) const;
    float toProportion (float v) const;
    float fromProportion (float p) const;
    float snapToDefault (float v) const;

    const Palette& pal;
    juce::String label;
    float minV, maxV, step, defaultV, value;
    float skew = 1.0f;
    std::function<juce::String (float)> format;
    float dragStartValue = 0.0f;
};

// Overlay panel for meter calibration: reference level and needle ballistics, with reset.
class CalPanel : public juce::Component
{
public:
    explicit CalPanel (const Palette& palette);

    std::function<void (const MeterSettings&)> onChange;
    std::function<void()> onClose;
    void setSettings (const MeterSettings&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changed();

    const Palette& pal;
    MeterSettings settings;
    CalSlider reference, rise, fall, overshoot;
    PillButton presetMinus20, presetMinus18, presetMinus14, preset0, reset, done;
    PillButton needleVu, needlePeak;
    int hintX = 0, needleLabelX = 0;
};
