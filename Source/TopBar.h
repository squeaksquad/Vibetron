#pragma once

#include "PluginProcessor.h"
#include "CalPanel.h"

// Small glyph/text button for the top bar.
class BarButton : public juce::Component
{
public:
    enum class Kind { undo, redo, about };

    BarButton (const Palette& palette, Kind kind);
    std::function<void()> onClick;
    void setDot (bool shouldShow)  { if (dot != shouldShow) { dot = shouldShow; repaint(); } }
    void setLit (bool shouldBeLit) { if (lit != shouldBeLit) { lit = shouldBeLit; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    const Palette& pal;
    Kind kind;
    bool dot = false, lit = false;
};

// Strip above the faceplate: undo/redo on the left, ABOUT (with an update dot) on the right.
class TopBar : public juce::Component, private juce::ChangeListener
{
public:
    static constexpr int height = 24;

    TopBar (VibetronProcessor&, const Palette&);
    ~TopBar() override;

    std::function<void()> onAbout;
    void setAboutOpen (bool open) { about.setLit (open); }
    juce::Component& aboutButton() { return about; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    VibetronProcessor& proc;
    const Palette& pal;
    BarButton undo, redo, about;
};

// Card under ABOUT: version, update status, check and download.
class AboutPanel : public juce::Component, private juce::ChangeListener
{
public:
    static constexpr int width = 280, height = 112;

    AboutPanel (UpdateChecker&, const Palette&);
    ~AboutPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::String statusText() const;

    UpdateChecker& updates;
    const Palette& pal;
    PillButton check, download;
};
