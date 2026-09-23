#include "PluginEditor.h"
#include "Textures.h"

namespace
{
    const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");
    constexpr double themeFadeSeconds = 0.6;
    constexpr double staleAudioSeconds = 0.15;
    constexpr int meterColumnRight = 326 + 258;  // right edge of the right-hand meter

    juce::RangedAudioParameter& param (VibetronProcessor& p, const char* id)
    {
        return *p.apvts.getParameter (id);
    }
}

//==============================================================================
CalButton::CalButton (const Palette& palette)
    : pal (palette)
{
    setTitle ("Meter calibration");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void CalButton::setState (float refDbfs, bool peak, bool panelOpen)
{
    if (refDbfs == ref && peak == peakMode && panelOpen == open)
        return;
    ref = refDbfs;
    peakMode = peak;
    open = panelOpen;
    repaint();
}

void CalButton::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto pillFont = fonts::legend (6.5f, 0.22f), textFont = fonts::legend (7.0f, 0.3f);
    const int r = roundToInt (ref);
    const auto refText = String (peakMode ? "PEAK  " : "") + "0 VU = " + (r < 0 ? minus + String (-r) : String (r)) + " dBFS";
    const float pillW = draw::textWidth ("VU CAL", pillFont) + 22.0f;
    const float total = pillW + 10.0f + draw::textWidth (refText, textFont);
    const float x0 = (float) getWidth() * 0.5f - total * 0.5f, cy = (float) getHeight() * 0.5f;

    const auto pill = Rectangle<float> (x0, cy - 7.5f, pillW, 15.0f);
    g.setColour (open ? pal.glow.withAlpha (0.18f) : Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (pill, 7.5f);
    g.setColour (open ? pal.glow.withAlpha (0.85f) : pal.legend.withAlpha (0.4f));
    g.drawRoundedRectangle (pill, 7.5f, 0.8f);
    g.setColour (open ? pal.glowHi : pal.legend);
    draw::text (g, "VU CAL", pillFont, pill.getCentreX(), draw::capsBaseline (cy, 6.5f), Justification::horizontallyCentred);

    g.setColour (pal.sub);
    draw::text (g, refText, textFont, pill.getRight() + 10.0f, draw::capsBaseline (cy, 7.0f));
}

void CalButton::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()) && onClick)
        onClick();
}

//==============================================================================
OperateButton::OperateButton (const Palette& palette, juce::RangedAudioParameter& p)
    : pal (palette),
      attachment (p, [this] (float v) { bypassed = v > 0.5f; repaint(); }, nullptr)
{
    setTitle ("Operate");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void OperateButton::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto font = fonts::legend (7.0f, 0.34f);
    const float textW = draw::textWidth ("OPERATE", font);
    const float totalW = 6.0f + 8.0f + textW;
    const float x0 = (float) getWidth() * 0.5f - totalW * 0.5f;
    const Point<float> lamp (x0 + 3.0f, (float) getHeight() * 0.5f);

    if (! bypassed)
    {
        g.setGradientFill (ColourGradient (pal.glow.withAlpha (0.7f), lamp, pal.glow.withAlpha (0.0f), lamp.translated (8, 0), true));
        g.fillEllipse (Rectangle<float> (16, 16).withCentre (lamp));
    }
    g.setColour (bypassed ? Colour (0xff222222) : pal.glow);
    g.fillEllipse (Rectangle<float> (6, 6).withCentre (lamp));
    g.setColour (bypassed ? pal.sub.withAlpha (0.6f) : pal.legend);
    draw::text (g, "OPERATE", font, x0 + 14.0f, lamp.y + 2.6f);
}

void OperateButton::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()))
        attachment.setValueAsCompleteGesture (bypassed ? 0.0f : 1.0f);
}

//==============================================================================
TameButton::TameButton (const Palette& palette, juce::RangedAudioParameter& p)
    : pal (palette),
      attachment (p, [this] (float v) { on = v > 0.5f; repaint(); }, nullptr)
{
    setTitle ("Output tame");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void TameButton::paint (juce::Graphics& g)
{
    using namespace juce;
    const float cx = (float) getWidth() * 0.5f;
    g.setColour (pal.sub);
    draw::text (g, "OUTPUT", fonts::legend (6.5f, 0.34f), cx, 8.0f, Justification::horizontallyCentred);

    // Machined push button: raised when off, seated with a faint inner glow when on.
    const auto btn = Rectangle<float> (6.0f, 13.0f, (float) getWidth() - 12.0f, 22.0f);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (btn.expanded (1.5f), 5.0f);
    g.setGradientFill (ColourGradient (on ? Colour (0xff111215) : Colour (0xff2a2d32), 0, btn.getY(),
                                       on ? Colour (0xff1c1e22) : Colour (0xff0d0e10), 0, btn.getBottom(), false));
    g.fillRoundedRectangle (btn, 4.0f);
    if (on)
    {
        g.setGradientFill (ColourGradient (pal.glow.withAlpha (0.16f), btn.getCentre(), pal.glow.withAlpha (0.0f), btn.getCentre().translated (btn.getWidth() * 0.5f, 0), true));
        g.fillRoundedRectangle (btn, 4.0f);
    }
    g.setColour (Colours::white.withAlpha (on ? 0.05f : 0.14f));
    g.drawLine (btn.getX() + 4, btn.getY() + 0.8f, btn.getRight() - 4, btn.getY() + 0.8f, 0.8f);

    // LED: lit in the mode colour while engaged.
    const Point<float> led (btn.getX() + 12.0f, btn.getCentreY());
    if (on)
    {
        g.setGradientFill (ColourGradient (pal.glow.withAlpha (0.7f), led, pal.glow.withAlpha (0.0f), led.translated (7, 0), true));
        g.fillEllipse (Rectangle<float> (14, 14).withCentre (led));
    }
    g.setColour (on ? pal.glow : Colour (0xff222222));
    g.fillEllipse (Rectangle<float> (5, 5).withCentre (led));

    g.setColour (on ? pal.legend : pal.sub);
    draw::text (g, "TAME", fonts::legend (9.0f, 0.3f), btn.getCentreX() + 6.0f, btn.getCentreY() + 3.2f, Justification::horizontallyCentred);
}

void TameButton::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()))
        attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}

//==============================================================================
FacePlate::FacePlate (VibetronProcessor& p)
    : proc (p),
      palette (Palette::forMode (0)),
      fromPalette (palette),
      selector (palette, param (p, VibetronProcessor::modeId)),
      calButton (palette),
      operate (palette, param (p, VibetronProcessor::bypassId)),
      tame (palette, param (p, VibetronProcessor::tameId)),
      left (palette, "LEFT CHANNEL"),
      right (palette, "RIGHT CHANNEL"),
      calPanel (palette),
      vblank (this, [this] (double ts) { tick (ts); })
{
    themeMode = selector.getMode();
    palette = fromPalette = Palette::forMode (themeMode);
    selector.onModeChanged = [this] (int m) { startThemeChange (m); };

    for (auto* c : std::initializer_list<juce::Component*> { &left, &right, &selector, &calButton, &operate, &tame })
        addAndMakeVisible (c);

    addChildComponent (calPanel);  // on top, hidden until VU CAL is clicked
    calButton.onClick = [this]
    {
        calPanel.setVisible (! calPanel.isVisible());
        calButton.setState (meterSettings.refDbfs, meterSettings.peakMode, calPanel.isVisible());
    };
    calPanel.onClose = [this] { calPanel.setVisible (false); calButton.setState (meterSettings.refDbfs, meterSettings.peakMode, false); };
    calPanel.onChange = [this] (const MeterSettings& s)
    {
        applyMeterSettings (s);
        proc.setMeterSettings (s);
    };
    calPanel.setSettings (proc.getMeterSettings());

    setOpaque (true);
    setSize (designWidth, designHeight);
}

void FacePlate::resized()
{
    left.setBounds (44, 50, 258, 150);
    right.setBounds (326, 50, 258, 150);
    selector.setBounds (676 - 92, 160 - 92, 184, 184);
    calButton.setBounds (204, 205, 220, 18);
    calPanel.setBounds (44, 242, meterColumnRight - 44, 142);
    operate.setBounds (676 - 50, 236, 100, 16);
    tame.setBounds (676 - 43, 298, 86, 38);  // centred under the knob and OPERATE
}

void FacePlate::applyMeterSettings (const MeterSettings& s)
{
    meterSettings = s;
    ballistics = Ballistics::from (s);
    calButton.setState (s.refDbfs, s.peakMode, calPanel.isVisible());
}

void FacePlate::startThemeChange (int newMode)
{
    if (newMode == themeMode)
        return;
    fromPalette = palette;
    themeMode = newMode;
    themeT = 0.0f;
}

void FacePlate::tick (double timestamp)
{
    const double dt = lastTimestamp > 0.0 ? juce::jlimit (0.0, 0.1, timestamp - lastTimestamp) : 1.0 / 60.0;
    lastTimestamp = timestamp;

    if (themeT < 1.0f)
    {
        themeT = juce::jmin (1.0f, themeT + (float) (dt / themeFadeSeconds));
        const float eased = themeT * themeT * (3.0f - 2.0f * themeT);
        palette = Palette::lerp (fromPalette, Palette::forMode (themeMode), eased);
        repaint();
    }

    // If the host stops calling processBlock, let the needles fall rather than freeze.
    const auto blocks = proc.meters.blocks.load (std::memory_order_acquire);
    if (blocks != lastBlocks)
    {
        lastBlocks = blocks;
        lastAudioTime = timestamp;
    }
    const bool live = timestamp - lastAudioTime < staleAudioSeconds;
    const bool mono = proc.meters.channels.load() == 1;

    auto level = [&] (int ch) { return live ? proc.meters.rectified[mono ? 0 : ch].load (std::memory_order_relaxed) : 0.0f; };
    auto peak = [&] (int ch) { const float p = proc.meters.peak[mono ? 0 : ch].exchange (0.0f); return live ? p : 0.0f; };

    // Each channel's held peak is taken once per frame and drives both the peak needle and the PEAK lamp.
    const float pk0 = peak (0), pk1 = mono ? pk0 : peak (1);
    left.advance (level (0), pk0, meterSettings, ballistics, dt);
    right.advance (level (1), pk1, meterSettings, ballistics, dt);

    // Pick up calibration restored by the host (e.g. loading a session) while the editor is open.
    if (--settingsPollCountdown <= 0)
    {
        settingsPollCountdown = 30;
        const auto stored = proc.getMeterSettings();
        if (stored != meterSettings)
            calPanel.setSettings (stored);
    }
    selector.advance (dt);
}

void FacePlate::paint (juce::Graphics& g)
{
    using namespace juce;
    const float W = designWidth, H = designHeight;
    const auto bounds = Rectangle<float> (W, H);

    // Machined aluminium frame.
    ColourGradient frame (Colour (0xffd6d6d4), 0, 0, Colour (0xffc9c9c7), 0, H, false);
    frame.addColour (0.5, Colour (0xff6f7070));
    g.setGradientFill (frame);
    g.fillRect (bounds);
    g.setOpacity (0.35f);
    g.drawImage (textures::brushedMetal(), bounds, RectanglePlacement::stretchToFit);

    // Smoked glass fascia with a diagonal sheen and faint grain.
    const auto glass = bounds.reduced (8.0f);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (glass.expanded (0.5f), 6.5f, 1.0f);
    g.setGradientFill (ColourGradient (Colour (0xff15171b), 0, glass.getY(), Colour (0xff050506), 0, glass.getBottom(), false));
    g.fillRoundedRectangle (glass, 6.0f);
    ColourGradient sheen (Colours::transparentWhite, glass.getX(), glass.getY(), Colours::transparentWhite, glass.getRight(), glass.getY() + glass.getHeight() * 0.6f, false);
    sheen.addColour (0.25, Colours::transparentWhite);
    sheen.addColour (0.42, Colours::white.withAlpha (0.07f));
    sheen.addColour (0.50, Colours::transparentWhite);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (glass, 6.0f);
    g.setOpacity (0.7f);
    g.drawImage (textures::filmGrain(), glass, RectanglePlacement::stretchToFit);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (glass.reduced (0.5f), 6.0f, 1.0f);

    // Brand row.
    const auto& m = kModes[(size_t) themeMode];
    g.setColour (palette.legend.withAlpha (0.35f));
    g.drawLine (44, 232, meterColumnRight, 232, 1.0f);  // flush with the meters on both sides
    const auto brandFont = fonts::legend (15.0f, 0.53f);
    g.setColour (palette.legend);
    draw::text (g, "VIBETRON", brandFont, 44, 258);
    g.setColour (palette.sub);
    draw::text (g, String::fromUTF8 ("VT-369  \xc2\xb7  STEREO HARMONIC RESOLUTION AMPLIFIER"), fonts::legend (7.5f, 0.35f),
                44 + draw::textWidth ("VIBETRON", brandFont) + 12, 258);

    // Liner notes.
    const auto nameFont = fonts::script (17.0f);
    g.setColour (palette.legend);
    draw::text (g, m.name, nameFont, 44, 289);
    g.setColour (palette.sub);
    draw::text (g, String ("MODE ") + m.numeral + String::fromUTF8 ("  \xc2\xb7  ") + String::fromUTF8 (m.tag),
                fonts::legend (7.0f, 0.34f), 44 + draw::textWidth (m.name, nameFont) + 12, 288);

    AttributedString body;
    body.setLineSpacing (4.0f);
    body.setWordWrap (AttributedString::byWord);
    body.append (m.notes, fonts::body (10.4f), Colour (0xffcfc9bd));
    TextLayout layout;
    const float notesWidth = meterColumnRight - 44.0f;  // ends with the divider line; the knob column stays clear
    layout.createLayout (body, notesWidth);
    layout.draw (g, Rectangle<float> (44, 298, notesWidth, 90));
}

//==============================================================================
VibetronEditor::VibetronEditor (VibetronProcessor& p)
    : AudioProcessorEditor (p), face (p)
{
    addAndMakeVisible (face);
    setResizable (true, true);
    setResizeLimits (600, 300, 1800, 900);
    getConstrainer()->setFixedAspectRatio ((double) FacePlate::designWidth / FacePlate::designHeight);
    setSize (1000, 500);
}

void VibetronEditor::resized()
{
    face.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) FacePlate::designWidth));
}
