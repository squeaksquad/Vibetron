#include "CalPanel.h"
#include "Textures.h"

namespace
{
    const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");
    constexpr float pillPadding = 11.0f;

    juce::Font pillFont() { return fonts::legend (6.5f, 0.22f); }

    juce::String dbfs (float v)
    {
        const int i = juce::roundToInt (v);
        return (i < 0 ? minus + juce::String (-i) : juce::String (i)) + " dBFS";
    }
}

//==============================================================================
PillButton::PillButton (const Palette& palette, juce::String t)
    : pal (palette), text (std::move (t))
{
    setTitle (text);
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

int PillButton::getIdealWidth() const
{
    return juce::roundToInt (draw::textWidth (text, pillFont()) + 2.0f * pillPadding);
}

void PillButton::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const float r = b.getHeight() * 0.5f;
    g.setColour (lit ? pal.glow.withAlpha (0.16f) : Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (b, r);
    g.setColour (lit ? pal.glow.withAlpha (0.8f) : pal.legend.withAlpha (hasKeyboardFocus (false) ? 0.7f : 0.3f));
    g.drawRoundedRectangle (b, r, 0.8f);
    g.setColour (lit ? pal.glowHi : pal.legend);
    draw::text (g, text, pillFont(), b.getCentreX(), draw::capsBaseline (b.getCentreY(), 6.5f), Justification::horizontallyCentred);
}

void PillButton::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()) && onClick)
        onClick();
}

bool PillButton::keyPressed (const juce::KeyPress& k)
{
    if ((k == juce::KeyPress::returnKey || k == juce::KeyPress::spaceKey) && onClick)
    {
        onClick();
        return true;
    }
    return false;
}

//==============================================================================
CalSlider::CalSlider (const Palette& palette, juce::String l, float minValue, float maxValue, float stepSize, float defaultValue,
                      std::function<juce::String (float)> fmt, bool skewForDefault)
    : pal (palette), label (std::move (l)), minV (minValue), maxV (maxValue), step (stepSize),
      defaultV (defaultValue), value (defaultValue), format (std::move (fmt))
{
    if (skewForDefault)
    {
        const float d = (defaultValue - minValue) / (maxValue - minValue);
        skew = std::log (0.5f) / std::log (d);  // proportion = d^skew puts the default at 0.5
    }
    setTitle (label);
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

void CalSlider::setValue (float v, bool notify)
{
    v = juce::jlimit (minV, maxV, step > 0 ? std::round (v / step) * step : v);
    if (v == value)
        return;
    value = v;
    repaint();
    if (notify && onChange)
        onChange (value);
}

juce::Rectangle<float> CalSlider::trackArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (labelWidth).withTrimmedRight (valueWidth).reduced (8.0f, 0.0f);
}

float CalSlider::toProportion (float v) const
{
    return std::pow (juce::jlimit (0.0f, 1.0f, (v - minV) / (maxV - minV)), skew);
}

float CalSlider::fromProportion (float p) const
{
    return minV + (maxV - minV) * std::pow (juce::jlimit (0.0f, 1.0f, p), 1.0f / skew);
}

float CalSlider::valueToX (float v) const
{
    const auto t = trackArea();
    return t.getX() + t.getWidth() * toProportion (v);
}

float CalSlider::xToValue (float x) const
{
    const auto t = trackArea();
    return fromProportion ((x - t.getX()) / t.getWidth());
}

float CalSlider::snapToDefault (float v) const
{
    // Within ~2% of the track from the default, settle on the default.
    return std::abs (toProportion (v) - toProportion (defaultV)) < 0.02f ? defaultV : v;
}

void CalSlider::paint (juce::Graphics& g)
{
    using namespace juce;
    if (! isEnabled())
        g.beginTransparencyLayer (0.35f);
    const auto b = getLocalBounds().toFloat();
    const float cy = b.getCentreY();

    g.setColour (pal.sub);
    draw::text (g, label, fonts::legend (6.5f, 0.3f), b.getX(), draw::capsBaseline (cy, 6.5f));

    // Track: recessed groove, lit portion in the mode glow, a tick at the default.
    const auto t = trackArea();
    g.setColour (Colours::black.withAlpha (0.8f));
    g.fillRoundedRectangle (t.getX(), cy - 1.5f, t.getWidth(), 3.0f, 1.5f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawLine (t.getX(), cy + 2.0f, t.getRight(), cy + 2.0f, 0.5f);
    const float x = valueToX (value);
    g.setColour (pal.glow.withAlpha (0.75f));
    g.fillRoundedRectangle (t.getX(), cy - 1.0f, x - t.getX(), 2.0f, 1.0f);
    const float dx = valueToX (defaultV);
    g.setColour (pal.legend.withAlpha (0.5f));
    g.drawLine (dx, cy - 6.0f, dx, cy - 3.5f, 0.8f);

    // Machined thumb.
    const auto thumb = Rectangle<float> (11, 11).withCentre ({ x, cy });
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (thumb.translated (0.5f, 1.5f).expanded (0.5f));
    g.setColour (Colour (0xff1b1d21));
    g.fillEllipse (thumb.expanded (1.0f));
    g.setOpacity (1.0f);
    g.drawImage (textures::spunAluminium(), thumb, RectanglePlacement::stretchToFit);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawEllipse (thumb, 0.4f);
    if (hasKeyboardFocus (false))
    {
        g.setColour (pal.glow.withAlpha (0.7f));
        g.drawEllipse (thumb.expanded (3.0f), 0.8f);
    }

    g.setColour (std::abs (value - defaultV) < step * 0.5f ? pal.legend : pal.glowHi);
    draw::text (g, format (value), fonts::legend (7.5f, 0.12f), b.getRight(), draw::capsBaseline (cy, 7.5f), Justification::right);

    if (! isEnabled())
        g.endTransparencyLayer();
}

void CalSlider::mouseDown (const juce::MouseEvent& e)
{
    dragStartValue = value;
    if (std::abs (e.position.x - valueToX (value)) > 7.0f && trackArea().expanded (8.0f, 10.0f).contains (e.position))
    {
        setValue (snapToDefault (xToValue (e.position.x)), true);  // click on the track jumps there
        dragStartValue = value;
    }
}

void CalSlider::mouseDrag (const juce::MouseEvent& e)
{
    // Drag in track proportion so the skewed scale feels even; Shift for fine control.
    const float perPixel = (e.mods.isShiftDown() ? 0.2f : 1.0f) / trackArea().getWidth();
    const float v = fromProportion (toProportion (dragStartValue) + (float) e.getDistanceFromDragStartX() * perPixel);
    setValue (e.mods.isShiftDown() ? v : snapToDefault (v), true);
}

void CalSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    setValue (defaultV, true);
}

bool CalSlider::keyPressed (const juce::KeyPress& k)
{
    const float s = step > 0 ? step : (maxV - minV) / 100.0f;
    if (k == juce::KeyPress::rightKey || k == juce::KeyPress::upKey)   { setValue (value + s, true); return true; }
    if (k == juce::KeyPress::leftKey  || k == juce::KeyPress::downKey) { setValue (value - s, true); return true; }
    return false;
}

//==============================================================================
CalPanel::CalPanel (const Palette& palette)
    : pal (palette),
      reference (palette, "0 VU REFERENCE", -24.0f, 0.0f, 1.0f, MeterSettings::defaultRefDbfs, dbfs),
      rise (palette, "RISE TIME", 50.0f, 1000.0f, 10.0f, MeterSettings::defaultRiseMs, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, true),
      fall (palette, "FALL TIME", 50.0f, 2000.0f, 10.0f, MeterSettings::defaultFallMs, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, true),
      overshoot (palette, "OVERSHOOT", 0.0f, 10.0f, 0.1f, MeterSettings::defaultOvershootPct, [] (float v) { return juce::String (v, 1) + " %"; }, true),
      presetMinus20 (palette, minus + "20"), presetMinus18 (palette, minus + "18"), presetMinus14 (palette, minus + "14"),
      preset0 (palette, "0 dBFS"), reset (palette, "RESET TO DEFAULTS"), done (palette, "DONE"),
      needleVu (palette, "VU"), needlePeak (palette, "PEAK")
{
    setTitle ("Meter calibration");
    setInterceptsMouseClicks (true, true);

    for (auto* c : std::initializer_list<juce::Component*> { &reference, &rise, &fall, &overshoot, &presetMinus20, &presetMinus18,
                                                              &presetMinus14, &preset0, &reset, &done, &needleVu, &needlePeak })
        addAndMakeVisible (c);

    reference.onChange = [this] (float v) { settings.refDbfs = v; changed(); };
    rise.onChange      = [this] (float v) { settings.riseMs = v; changed(); };
    fall.onChange      = [this] (float v) { settings.fallMs = v; changed(); };
    overshoot.onChange = [this] (float v) { settings.overshootPct = v; changed(); };

    presetMinus20.onClick = [this] { reference.setValue (-20.0f, true); };
    presetMinus18.onClick = [this] { reference.setValue (-18.0f, true); };
    presetMinus14.onClick = [this] { reference.setValue (-14.0f, true); };
    preset0.onClick       = [this] { reference.setValue (0.0f, true); };
    needleVu.onClick   = [this] { settings.peakMode = false; changed(); };
    needlePeak.onClick = [this] { settings.peakMode = true; changed(); };
    reset.onClick = [this] { setSettings (MeterSettings {}); changed(); };
    done.onClick  = [this] { if (onClose) onClose(); };
}

void CalPanel::setSettings (const MeterSettings& s)
{
    settings = s;
    reference.setValue (s.refDbfs, false);
    rise.setValue (s.riseMs, false);
    fall.setValue (s.fallMs, false);
    overshoot.setValue (s.overshootPct, false);
    changed();
}

void CalPanel::changed()
{
    presetMinus20.setLit (settings.refDbfs == -20.0f);
    presetMinus18.setLit (settings.refDbfs == -18.0f);
    presetMinus14.setLit (settings.refDbfs == -14.0f);
    preset0.setLit (settings.refDbfs == 0.0f);
    reset.setLit (false);
    needleVu.setLit (! settings.peakMode);
    needlePeak.setLit (settings.peakMode);
    for (auto* s : { &rise, &fall, &overshoot })
        s->setEnabled (! settings.peakMode);  // peak mode uses fixed fast ballistics
    repaint();
    if (onChange)
        onChange (settings);
}

void CalPanel::resized()
{
    const int w = getWidth(), h = PillButton::height;
    done.setBounds (w - 16 - done.getIdealWidth(), 8, done.getIdealWidth(), h);
    reset.setBounds (done.getX() - 8 - reset.getIdealWidth(), 8, reset.getIdealWidth(), h);

    // NEEDLE  [VU] [PEAK] after the title.
    needleLabelX = 176;
    const int needleW = juce::jmax (needleVu.getIdealWidth(), needlePeak.getIdealWidth());
    needleVu.setBounds (needleLabelX + 49, 8, needleW, h);
    needlePeak.setBounds (needleVu.getRight() + 5, 8, needleW, h);

    const int x = 16, rowW = w - 32, rowH = 20, gap = 5;

    // Presets share one width (the widest label) and sit clear of the reference readout.
    int presetW = 0;
    for (auto* p : { &presetMinus20, &presetMinus18, &presetMinus14, &preset0 })
        presetW = juce::jmax (presetW, p->getIdealWidth());
    constexpr int presetGap = 6, readoutGap = 14;
    const int presetsW = 4 * presetW + 3 * presetGap;
    int px = x + rowW - presetsW;
    for (auto* p : { &presetMinus20, &presetMinus18, &presetMinus14, &preset0 })
    {
        p->setBounds (px, 36 + (rowH - h) / 2, presetW, h);
        px += presetW + presetGap;
    }
    // One grid: every track the same length, values in one column, presets / hints in the last column.
    const int sliderW = rowW - presetsW - readoutGap;
    hintX = x + rowW - presetsW;
    reference.setBounds (x, 36, sliderW, rowH);
    rise.setBounds (x, 36 + rowH + gap, sliderW, rowH);
    fall.setBounds (x, 36 + 2 * (rowH + gap), sliderW, rowH);
    overshoot.setBounds (x, 36 + 3 * (rowH + gap), sliderW, rowH);
}

void CalPanel::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();

    // Smoked-glass slab floating over the fascia.
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (0, 3).expanded (1.0f), 7.0f);
    g.setGradientFill (ColourGradient (Colour (0xf4121418), 0, 0, Colour (0xf8070809), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (pal.legend.withAlpha (0.28f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 0.8f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (b.getX() + 8, b.getY() + 1.2f, b.getRight() - 8, b.getY() + 1.2f, 0.8f);

    g.setColour (pal.legend);
    // Header row: title, NEEDLE label and all pills share one centre line.
    const float headerCentre = (float) done.getBounds().getCentreY();
    draw::text (g, "METER CALIBRATION", fonts::legend (8.0f, 0.4f), 16, draw::capsBaseline (headerCentre, 8.0f));
    g.setColour (pal.legend.withAlpha (0.2f));
    g.drawLine (16, 30, b.getRight() - 16, 30, 0.6f);

    g.setColour (pal.sub);
    draw::text (g, "NEEDLE", fonts::legend (6.5f, 0.3f), (float) needleLabelX, draw::capsBaseline (headerCentre, 6.5f));

    // Hints in the preset column for the ballistics rows.
    const auto hintFont = fonts::legend (6.0f, 0.2f);
    const bool peak = settings.peakMode;
    for (auto [row, hint] : { std::pair<CalSlider*, const char*> { &rise, peak ? "FIXED IN PEAK MODE" : "TO 99% OF A STEP" },
                              std::pair<CalSlider*, const char*> { &fall, peak ? "PEAK FALLS 20 dB / 1.7 s" : "BACK DOWN 99%" },
                              std::pair<CalSlider*, const char*> { &overshoot, peak ? "NO OVERSHOOT" : "SWING PAST TARGET" } })
        draw::text (g, hint, hintFont, (float) hintX + 2.0f, draw::capsBaseline ((float) row->getBounds().getCentreY(), 6.0f));
}
