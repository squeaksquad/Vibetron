#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

// Three-position machined aluminium rotary with a printed scale, indicator lamps and legends.
// Drag (up/right = clockwise), click a legend, scroll, or use arrow keys.
class ModeSelector : public juce::Component
{
public:
    ModeSelector (const Palette& palette, juce::RangedAudioParameter& modeParam);

    std::function<void (int)> onModeChanged;
    int getMode() const { return mode; }
    void advance (double dt);  // snap-to-detent animation

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void select (int newMode);
    void setFromParameter (float value);
    void paintKnob (juce::Graphics&, juce::Point<float> c) const;
    juce::Point<float> centre() const { return getLocalBounds().toFloat().getCentre(); }
    static float detentAngle (int m) { return juce::degreesToRadians (-46.0f + 46.0f * (float) m); }

    const Palette& pal;
    juce::ParameterAttachment attachment;
    int mode = 0;
    float angle = detentAngle (0), angVel = 0.0f;
    float dragStartAngle = 0.0f;
    bool dragging = false;
};
