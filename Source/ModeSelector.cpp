#include "ModeSelector.h"
#include "Textures.h"

namespace
{
    constexpr float labelRadius = 80.0f, lampRadius = 64.0f, grabRadius = 48.0f;
    constexpr float skirtRadius = 42.0f, chamferRadius = 36.4f, faceRadius = 34.6f;
    constexpr float unitsPerDetent = 55.0f;  // drag distance (design units) per position
}

ModeSelector::ModeSelector (const Palette& palette, juce::RangedAudioParameter& modeParam)
    : pal (palette),
      attachment (modeParam, [this] (float v) { setFromParameter (v); }, nullptr)
{
    setWantsKeyboardFocus (true);
    setTitle ("Mode");
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    attachment.sendInitialUpdate();
    angle = detentAngle (mode);
}

void ModeSelector::setFromParameter (float value)
{
    mode = juce::jlimit (0, numModes - 1, juce::roundToInt (value));
    if (onModeChanged)
        onModeChanged (mode);
    repaint();
}

void ModeSelector::select (int newMode)
{
    newMode = juce::jlimit (0, numModes - 1, newMode);
    if (newMode != mode)
        attachment.setValueAsCompleteGesture ((float) newMode);
}

void ModeSelector::advance (double dt)
{
    // Near-critically damped spring: settles into the detent without wobble.
    const float target = detentAngle (mode);
    if (dragging || (std::abs (target - angle) < 1.0e-4f && std::abs (angVel) < 1.0e-3f))
        return;
    constexpr float wn = 30.0f, zeta = 0.95f;
    constexpr int steps = 4;
    const float h = (float) juce::jmin (dt, 0.05) / steps;
    for (int i = 0; i < steps; ++i)
    {
        angVel += (wn * wn * (target - angle) - 2.0f * zeta * wn * angVel) * h;
        angle += angVel * h;
    }
    repaint();
}

void ModeSelector::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto c = centre();

    // Printed scale on the glass.
    for (int i = 0; i <= 12; ++i)
    {
        const float a = degreesToRadians (-46.0f + 92.0f * (float) i / 12.0f);
        const bool major = i % 6 == 0;
        g.setColour (pal.legend.withAlpha (0.55f));
        g.drawLine (Line<float> (c.getPointOnCircumference (51.0f, a), c.getPointOnCircumference (major ? 56.0f : 54.0f, a)), major ? 1.2f : 0.7f);
    }

    // Lamps and numerals.
    for (int i = 0; i < numModes; ++i)
    {
        const auto a = detentAngle (i);
        const auto dot = c.getPointOnCircumference (lampRadius, a);
        if (i == mode)
        {
            g.setGradientFill (ColourGradient (pal.glow.withAlpha (0.75f), dot, pal.glow.withAlpha (0.0f), dot.translated (9, 0), true));
            g.fillEllipse (Rectangle<float> (18, 18).withCentre (dot));
        }
        g.setColour (i == mode ? pal.glow : Colour (0xff222222));
        g.fillEllipse (Rectangle<float> (6, 6).withCentre (dot));

        const auto lp = c.getPointOnCircumference (labelRadius, a);
        g.setColour (pal.legend);
        draw::text (g, kModes[(size_t) i].numeral, fonts::legend (11.0f), lp.x, lp.y + 4.0f, Justification::horizontallyCentred);
    }

    g.setColour (pal.sub);
    draw::text (g, "CHARACTER", fonts::legend (7.0f, 0.34f), c.x, c.y + 66.0f, Justification::horizontallyCentred);

    paintKnob (g, c);

    if (hasKeyboardFocus (false))
    {
        g.setColour (pal.legend.withAlpha (0.6f));
        g.drawEllipse (Rectangle<float> (100, 100).withCentre (c), 0.8f);
    }
}

void ModeSelector::paintKnob (juce::Graphics& g, juce::Point<float> c) const
{
    using namespace juce;
    auto circle = [c] (float r) { return Rectangle<float> (r * 2, r * 2).withCentre (c); };

    // Seat and cast shadow.
    for (int i = 6; i > 0; --i)
    {
        g.setColour (Colours::black.withAlpha (0.11f));
        g.fillEllipse (Rectangle<float> (90.0f + (float) i * 2.5f, 86.0f + (float) i * 2.5f).withCentre (c.translated (2, 9)));
    }
    g.setColour (Colour (0xff040405));
    g.fillEllipse (circle (47.5f));
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.9f), 0, c.y - 47.5f, Colours::white.withAlpha (0.22f), 0, c.y + 47.5f, false));
    g.drawEllipse (circle (47.0f), 1.0f);

    // Anodised skirt with fine two-tone knurling (rotates with the knob).
    ColourGradient skirt (Colour (0xff4b4f56), c.x - 4, c.y - 8, Colour (0xff0f1113), c.x + skirtRadius * 1.2f, c.y, true);
    skirt.addColour (0.85, Colour (0xff25282d));
    g.setGradientFill (skirt);
    g.fillEllipse (circle (skirtRadius));
    for (int i = 0; i < 120; ++i)
    {
        const float a = angle + MathConstants<float>::twoPi * (float) i / 120.0f;
        const float a2 = a + degreesToRadians (1.3f);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawLine (Line<float> (c.getPointOnCircumference (skirtRadius - 5.5f, a), c.getPointOnCircumference (skirtRadius - 0.4f, a)), 0.9f);
        g.setColour (Colours::white.withAlpha (0.22f));
        g.drawLine (Line<float> (c.getPointOnCircumference (skirtRadius - 5.5f, a2), c.getPointOnCircumference (skirtRadius - 0.4f, a2)), 0.55f);
    }

    // Fixed lighting over the knurling: lit from the top left.
    ColourGradient skirtLight (Colours::white.withAlpha (0.5f), c.x - skirtRadius * 0.7f, c.y - skirtRadius,
                               Colours::black.withAlpha (0.6f), c.x + skirtRadius * 0.1f, c.y + skirtRadius, false);
    skirtLight.addColour (0.42, Colours::transparentWhite);
    skirtLight.addColour (0.60, Colours::transparentBlack);
    g.setGradientFill (skirtLight);
    g.drawEllipse (circle (skirtRadius - 2.8f), 5.8f);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawEllipse (circle (skirtRadius), 0.8f);

    // Polished chamfer, with the meter glow reflected on its lower-left edge.
    ColourGradient chamfer (Colours::white, c.x - chamferRadius * 0.6f, c.y - chamferRadius,
                            Colour (0xff4d5054), c.x + chamferRadius * 0.6f, c.y + chamferRadius, false);
    chamfer.addColour (0.45, Colour (0xffc3c6ca));
    g.setGradientFill (chamfer);
    g.fillEllipse (circle (chamferRadius));
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (circle (chamferRadius), 0.5f);
    {
        Path kiss;
        kiss.addCentredArc (c.x, c.y, 35.6f, 35.6f, 0.0f, degreesToRadians (200.0f), degreesToRadians (262.0f), true);
        g.setColour (pal.glow.withAlpha (0.25f));
        g.strokePath (kiss, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (pal.glow.withAlpha (0.55f));
        g.strokePath (kiss, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Spun face (sheen stays put while the knob turns), shallow dish, rim catch, specular.
    g.setOpacity (1.0f);
    g.drawImage (textures::spunAluminium(), circle (faceRadius), RectanglePlacement::stretchToFit);
    const Point<float> dishCentre (c.x, c.y + faceRadius * 0.2f);
    ColourGradient dish (Colours::transparentBlack, dishCentre, Colours::black.withAlpha (0.32f), dishCentre.translated (faceRadius * 1.24f, 0), true);
    dish.addColour (0.72, Colours::transparentBlack);
    g.setGradientFill (dish);
    g.fillEllipse (circle (faceRadius));
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawEllipse (circle (faceRadius), 0.5f);
    {
        Graphics::ScopedSaveState s (g);
        g.addTransform (AffineTransform::rotation (degreesToRadians (-38.0f), c.x - 11, c.y - 15));
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.55f), c.x - 11, c.y - 15, Colours::transparentWhite, c.x + 4, c.y - 15, true));
        g.fillEllipse (Rectangle<float> (30, 14).withCentre ({ c.x - 11, c.y - 15 }));
    }

    // Milled pointer groove and jewel (rotate with the knob).
    const auto rot = AffineTransform::rotation (angle, c.x, c.y);
    Path groove, catchLine;
    groove.addRoundedRectangle (c.x - 1.7f, c.y - 29.0f, 3.4f, 17.0f, 1.7f);
    catchLine.addRoundedRectangle (c.x + 0.6f, c.y - 28.2f, 0.8f, 15.4f, 0.4f);
    g.setColour (Colour (0xff17181a));
    g.fillPath (groove, rot);
    g.setColour (Colours::white.withAlpha (0.55f));
    g.fillPath (catchLine, rot);

    const auto jewel = c.getPointOnCircumference (31.2f, angle);
    g.setGradientFill (ColourGradient (pal.glow.withAlpha (0.8f), jewel, pal.glow.withAlpha (0.0f), jewel.translated (5, 0), true));
    g.fillEllipse (Rectangle<float> (10, 10).withCentre (jewel));
    g.setColour (pal.glowHi);
    g.fillEllipse (Rectangle<float> (3.2f, 3.2f).withCentre (jewel));
}

void ModeSelector::mouseDown (const juce::MouseEvent& e)
{
    dragging = e.position.getDistanceFrom (centre()) < grabRadius;
    if (dragging)
    {
        dragStartAngle = detentAngle (mode);
        angVel = 0.0f;
        return;
    }

    // Nearest legend within reach.
    int best = -1;
    float bestDist = 18.0f;
    for (int i = 0; i < numModes; ++i)
    {
        const auto d = e.position.getDistanceFrom (centre().getPointOnCircumference (labelRadius, detentAngle (i)));
        if (d < bestDist) { bestDist = d; best = i; }
    }
    if (best >= 0)
        select (best);
}

void ModeSelector::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    // Linear drag: up or right turns clockwise; the knob follows the pointer continuously.
    const auto delta = e.getOffsetFromDragStart();
    const float step = detentAngle (1) - detentAngle (0);
    angle = juce::jlimit (detentAngle (0), detentAngle (numModes - 1),
                          dragStartAngle + (float) (delta.x - delta.y) / unitsPerDetent * step);
    select (juce::roundToInt ((angle - detentAngle (0)) / step));
    repaint();
}

void ModeSelector::mouseUp (const juce::MouseEvent&)
{
    dragging = false;  // advance() now eases into the detent
}

void ModeSelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const float d = std::abs (w.deltaY) > std::abs (w.deltaX) ? w.deltaY : -w.deltaX;
    if (std::abs (d) > 0.01f)
        select (mode + (d > 0 ? 1 : -1));
}

bool ModeSelector::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::rightKey || k == juce::KeyPress::upKey)   { select (mode + 1); return true; }
    if (k == juce::KeyPress::leftKey  || k == juce::KeyPress::downKey) { select (mode - 1); return true; }
    if (k == juce::KeyPress::spaceKey || k == juce::KeyPress::returnKey) { select ((mode + 1) % numModes); return true; }
    return false;
}

std::unique_ptr<juce::AccessibilityHandler> ModeSelector::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (
        *this, juce::AccessibilityRole::button,
        juce::AccessibilityActions().addAction (juce::AccessibilityActionType::press, [this] { select ((mode + 1) % numModes); }));
}
