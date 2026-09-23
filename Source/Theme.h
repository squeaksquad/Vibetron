#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Modes.h"

// Per-mode palette ("Midnight Glass" direction): the meter backlight and legend colours shift per mode.
struct Palette
{
    juce::Colour glow, glowHi, legend, sub, tick;

    static Palette forMode (int mode)
    {
        using C = juce::Colour;
        switch (mode)
        {
            case 1:  return { C (0xffff8f78), C (0xffffe1d8), C (0xffefb9a4), C (0xff93736a), C (0xffffece6) };
            case 2:  return { C (0xff4fb9ff), C (0xffd9f1ff), C (0xffd9c08a), C (0xff7d8796), C (0xffe8f6ff) };
            default: return { C (0xffffd48a), C (0xfffff1d2), C (0xffe5cc92), C (0xff8f8467), C (0xfffff3da) };
        }
    }

    static Palette lerp (const Palette& a, const Palette& b, float t)
    {
        auto m = [t] (juce::Colour x, juce::Colour y) { return x.interpolatedWith (y, t); };
        return { m (a.glow, b.glow), m (a.glowHi, b.glowHi), m (a.legend, b.legend), m (a.sub, b.sub), m (a.tick, b.tick) };
    }
};

namespace fonts
{
    // macOS faces with graceful fallback elsewhere. Sizes are CSS-style point heights.
    inline juce::Font legend (float pt, float kerning = 0.0f, bool bold = false)
    {
        return juce::Font (juce::FontOptions ("Futura", bold ? "Bold" : "Medium", 12.0f).withPointHeight (pt))
                   .withExtraKerningFactor (kerning);
    }
    inline juce::Font body (float pt)
    {
        return juce::Font (juce::FontOptions ("Avenir Next", "Regular", 12.0f).withPointHeight (pt));
    }
    inline juce::Font script (float pt)
    {
        return juce::Font (juce::FontOptions ("Didot", "Italic", 12.0f).withPointHeight (pt));
    }
}

namespace draw
{
    inline float textWidth (const juce::String& s, const juce::Font& f)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, s, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }

    // Baseline that vertically centres capitals on centreY (cap height of these faces is ~0.7 of the point size).
    inline float capsBaseline (float centreY, float pointSize) { return centreY + 0.35f * pointSize; }

    // Draws a single line of text with its baseline at y.
    inline void text (juce::Graphics& g, const juce::String& s, const juce::Font& f, float x, float baseline,
                      juce::Justification j = juce::Justification::left)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, s, 0.0f, 0.0f);
        const auto w = ga.getBoundingBox (0, -1, true).getWidth();
        const auto dx = j.testFlags (juce::Justification::horizontallyCentred) ? -w * 0.5f
                      : j.testFlags (juce::Justification::right) ? -w : 0.0f;
        ga.moveRangeOfGlyphs (0, -1, x + dx, baseline);
        ga.draw (g);
    }

    // Engraved legend: a light catch on the lower lip, then the ink fill.
    inline void engraved (juce::Graphics& g, const juce::String& s, const juce::Font& f, float x, float baseline,
                          juce::Colour ink, juce::Justification j = juce::Justification::left, float catchAlpha = 0.28f)
    {
        g.setColour (juce::Colours::white.withAlpha (catchAlpha));
        text (g, s, f, x, baseline + 0.7f, j);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        text (g, s, f, x, baseline - 0.4f, j);
        g.setColour (ink);
        text (g, s, f, x, baseline, j);
    }
}
