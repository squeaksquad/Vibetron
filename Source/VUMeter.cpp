#include "VUMeter.h"

namespace
{
    constexpr float maxVu = 3.0f, sweepDeg = 96.0f;
    // Rectified average of a sine is 2/pi of its peak; scale so a tone reads at its peak dBFS level
    // (the usual "0 VU = -18 dBFS" convention, and 0 dBFS full-scale tone = 0 VU at the 0 dBFS reference).
    constexpr float sinePeakFromAverage = juce::MathConstants<float>::halfPi;

    // Radial gradient stretched horizontally into an ellipse about its centre.
    juce::FillType stretched (const juce::ColourGradient& grad, float sx, juce::Point<float> about)
    {
        juce::FillType ft (grad);
        ft.transform = juce::AffineTransform::scale (sx, 1.0f, about.x, about.y);
        return ft;
    }

    float angleFor (float pos) { return juce::degreesToRadians (-sweepDeg * 0.5f + sweepDeg * pos); }
}

VUMeter::VUMeter (const Palette& palette, juce::String channelLabel)
    : pal (palette), label (std::move (channelLabel))
{
    setInterceptsMouseClicks (false, false);
    setTitle (label + " VU meter");
}

float VUMeter::positionForVu (float vuDb)
{
    return std::pow (10.0f, vuDb / 20.0f) / std::pow (10.0f, maxVu / 20.0f);
}

void VUMeter::advance (float rectified, float peak, const MeterSettings& s, const Ballistics& b, double dt)
{
    peakMode = s.peakMode;
    constexpr float peakFallDbPerSec = 20.0f / 1.7f;
    const float peakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
    peakEnvDb = juce::jmax (peakDb, peakEnvDb - peakFallDbPerSec * (float) dt);

    const float levelDb = s.peakMode ? peakEnvDb
                                     : juce::Decibels::gainToDecibels (rectified * sinePeakFromAverage, -100.0f);
    const float vuDb = levelDb - s.refDbfs;
    const bool peakHit = peak > juce::Decibels::decibelsToGain (s.refDbfs);
    const float target = juce::jmin (1.12f, positionForVu (vuDb));

    // Substep so fast ballistics stay stable (keep wn * h well under 1).
    const float span = (float) juce::jmin (dt, 0.05);
    const int steps = juce::jlimit (4, 64, (int) std::ceil (juce::jmax (b.wnRise, b.wnFall) * span * 4.0f));
    const float h = span / (float) steps;
    for (int i = 0; i < steps; ++i)
    {
        const float wn = target >= pos ? b.wnRise : b.wnFall;
        const float acc = wn * wn * (target - pos) - 2.0f * b.zeta * wn * vel;
        vel += acc * h;
        pos += vel * h;
    }
    // Mechanical end stops.
    if (pos < -0.03f) { pos = -0.03f; vel = juce::jmax (0.0f, vel) * 0.3f; }
    if (pos > 1.10f)  { pos = 1.10f;  vel = juce::jmin (0.0f, vel) * 0.3f; }

    peakHold = peakHit ? 0.5f : juce::jmax (0.0f, peakHold - (float) dt);
    repaint();
}

void VUMeter::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();
    const Colour red (0xffff5a45), redText (0xffff6a55);

    // Black bezel with a faint machined edge.
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

    const auto f = b.reduced (3.0f);
    const float w = f.getWidth(), h = f.getHeight();
    const Point<float> pivot (f.getCentreX(), f.getY() + h * 1.08f);
    const float r = h * 0.82f;

    Graphics::ScopedSaveState save (g);
    Path clip;
    clip.addRoundedRectangle (f, 6.0f);
    g.reduceClipRegion (clip);

    // Dark face lit from below through smoked glass (elliptical glow, scaled to the face).
    g.setColour (Colour (0xff07080a));
    g.fillRect (f);
    const Point<float> glowCentre (f.getCentreX(), f.getBottom() - h * 0.05f);
    ColourGradient glow (pal.glow.withAlpha (0.95f), glowCentre, pal.glow.withAlpha (0.08f), glowCentre.translated (h * 0.9f, 0), true);
    glow.addColour (0.55, pal.glow.withAlpha (0.45f));
    g.setFillType (stretched (glow, w / h, glowCentre));
    g.fillRect (f);

    auto arc = [&] (float radius, float a0, float a1, float thickness, Colour c)
    {
        Path p;
        p.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, a0, a1, true);
        g.setColour (c);
        g.strokePath (p, PathStrokeType (thickness));
    };
    auto tick = [&] (float tickPos, float r0, float r1, float thickness, Colour c)
    {
        const auto a = angleFor (tickPos);
        g.setColour (c);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (r0, a), pivot.getPointOnCircumference (r1, a)), thickness);
    };

    const float aMin = angleFor (positionForVu (-20.0f)), aZero = angleFor (positionForVu (0.0f)), aMax = angleFor (1.0f);
    arc (r, aMin, aZero, 1.4f, pal.tick);
    arc (r + 3.0f, aZero, aMax, 7.0f, red);
    arc (r, aZero, aMax, 1.4f, red);

    for (float db : { -15.0f, -8.0f, -6.0f, -4.0f, -2.5f, -1.5f, -0.5f, 0.5f, 1.5f, 2.5f })
        tick (positionForVu (db), r, r + 5.0f, 0.9f, db > 0 ? red : pal.tick);

    for (float db : { -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f })
    {
        const auto p = positionForVu (db);
        tick (p, r - 1.0f, r + 10.0f, 1.6f, db > 0 ? red : pal.tick);
        const auto t = pivot.getPointOnCircumference (r + 19.0f, angleFor (p));
        g.setColour (db > 0 ? redText : pal.tick);
        draw::text (g, db > 0 ? "+" + String ((int) db) : String ((int) std::abs (db)),
                    fonts::legend (db == 0.0f ? 11.0f : 9.5f), t.x, t.y + 3.5f, Justification::horizontallyCentred);
    }

    for (int pct = 0; pct <= 100; pct += 20)
    {
        const auto p = (float) pct / 100.0f * positionForVu (0.0f);
        tick (p, r - 6.0f, r - 1.0f, 0.8f, pal.tick.withAlpha (0.7f));
        const auto t = pivot.getPointOnCircumference (r - 14.0f, angleFor (p));
        g.setColour (pal.tick.withAlpha (0.7f));
        draw::text (g, String (pct), fonts::legend (6.5f), t.x, t.y + 3.0f, Justification::horizontallyCentred);
    }

    g.setColour (pal.tick);
    draw::text (g, peakMode ? "PEAK" : "VU", fonts::script (h * 0.15f), f.getCentreX(), f.getY() + h * 0.66f, Justification::horizontallyCentred);
    g.setColour (pal.tick.withAlpha (0.75f));
    draw::text (g, label, fonts::legend (6.5f, 0.34f), f.getCentreX(), f.getY() + h * 0.79f, Justification::horizontallyCentred);

    // Peak lamp.
    const Point<float> lamp (f.getRight() - 16.0f, f.getBottom() - 16.0f);
    if (peakHold > 0.0f)
    {
        g.setGradientFill (ColourGradient (Colour (0x90ff3b24), lamp, Colour (0x00ff3b24), lamp.translated (12, 0), true));
        g.fillEllipse (Rectangle<float> (24, 24).withCentre (lamp));
    }
    g.setColour (peakHold > 0.0f ? Colour (0xffff4a30) : Colour (0xff2a0d0a));
    g.fillEllipse (Rectangle<float> (7.2f, 7.2f).withCentre (lamp));
    g.setColour (Colours::white.withAlpha (0.3f));
    g.fillEllipse (Rectangle<float> (2.2f, 2.2f).withCentre (lamp.translated (-1.2f, -1.4f)));
    g.setColour (pal.tick.withAlpha (0.7f));
    draw::text (g, "PEAK", fonts::legend (5.5f, 0.25f), lamp.x - 8.0f, lamp.y + 2.5f, Justification::right);

    // Needle and its shadow on the lit face.
    const auto a = angleFor (pos);
    auto needle = [&] (float alphaScale, Colour c, Point<float> o)
    {
        g.setColour (c.withMultipliedAlpha (alphaScale));
        g.drawLine (Line<float> (pivot.getPointOnCircumference (22.0f, a) + o, pivot.getPointOnCircumference (r + 8.0f, a) + o), 1.5f);
        g.drawLine (Line<float> (pivot + o, pivot.getPointOnCircumference (24.0f, a) + o), 3.0f);
    };
    needle (0.3f, Colours::black, { 3.0f, 4.0f });
    needle (1.0f, Colour (0xff101010), {});

    // Smoked glass: faint top reflection and a heavy vignette.
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.12f), 0, f.getY(), Colours::white.withAlpha (0.0f), 0, f.getY() + h * 0.45f, false));
    g.fillRect (f.withHeight (h * 0.45f));
    const Point<float> vc (f.getCentreX(), f.getY() + h * 0.7f);
    ColourGradient vig (Colours::transparentBlack, vc, Colours::black.withAlpha (0.75f), vc.translated (h * 0.8f, 0), true);
    vig.addColour (0.45, Colours::transparentBlack);
    g.setFillType (stretched (vig, w / h, vc));
    g.fillRect (f);
}
