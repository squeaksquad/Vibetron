#include "TopBar.h"

namespace
{
    const juce::Colour updateRed (0xffff4a3d);

    // Hooked arrow: head on the left, curling round to finish under the shaft. Mirrored for redo.
    juce::Path undoGlyph (juce::Point<float> c)
    {
        juce::Path p;
        const float l = c.x - 5.5f, r = c.x + 5.0f, top = c.y - 2.5f, bottom = c.y + 4.0f;
        p.startNewSubPath (l + 3.0f, top);
        p.lineTo (r - 3.5f, top);
        p.quadraticTo (r, top, r, (top + bottom) * 0.5f);
        p.quadraticTo (r, bottom, r - 3.5f, bottom);
        p.lineTo (c.x - 1.5f, bottom);
        return p;
    }

    juce::Path undoHead (juce::Point<float> c)
    {
        juce::Path p;
        const float l = c.x - 5.5f, top = c.y - 2.5f;
        p.addTriangle (l, top, l + 4.0f, top - 3.2f, l + 4.0f, top + 3.2f);
        return p;
    }
}

//==============================================================================
BarButton::BarButton (const Palette& palette, Kind k)
    : pal (palette), kind (k)
{
    setTitle (kind == Kind::undo ? "Undo" : kind == Kind::redo ? "Redo" : "About");
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void BarButton::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();
    const bool hot = isEnabled() && (isMouseOver() || hasKeyboardFocus (false));
    const auto ink = ! isEnabled() ? pal.sub.withAlpha (0.35f) : lit ? pal.glowHi : hot ? pal.legend : pal.legend.withAlpha (0.7f);

    if (hot || lit)
    {
        g.setColour (lit ? pal.glow.withAlpha (0.14f) : Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (b.reduced (0.5f), 4.0f);
    }

    if (kind == Kind::about)
    {
        const auto font = fonts::legend (6.5f, 0.3f);
        const float textW = draw::textWidth ("ABOUT", font);
        g.setColour (ink);
        draw::text (g, "ABOUT", font, b.getCentreX(), draw::capsBaseline (b.getCentreY(), 6.5f), Justification::horizontallyCentred);
        if (dot)
        {
            const Point<float> d (b.getCentreX() + textW * 0.5f + 4.0f, b.getCentreY() - 3.0f);
            g.setColour (updateRed.withAlpha (0.35f));
            g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (d));
            g.setColour (updateRed);
            g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre (d));
        }
        return;
    }

    const auto flip = kind == Kind::redo ? AffineTransform::scale (-1.0f, 1.0f, b.getCentreX(), 0.0f) : AffineTransform();
    g.setColour (ink);
    g.strokePath (undoGlyph (b.getCentre()), PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded), flip);
    g.fillPath (undoHead (b.getCentre()), flip);
}

void BarButton::mouseUp (const juce::MouseEvent& e)
{
    if (contains (e.getPosition()) && onClick)
        onClick();
}

bool BarButton::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::returnKey && onClick)
    {
        onClick();
        return true;
    }
    return false;
}

//==============================================================================
TopBar::TopBar (VibetronProcessor& p, const Palette& palette)
    : proc (p), pal (palette),
      undo (palette, BarButton::Kind::undo),
      redo (palette, BarButton::Kind::redo),
      about (palette, BarButton::Kind::about)
{
    for (auto* b : { &undo, &redo, &about })
        addAndMakeVisible (b);

    undo.onClick = [this] { proc.history.undo(); };
    redo.onClick = [this] { proc.history.redo(); };
    about.onClick = [this] { if (onAbout) onAbout(); };

    proc.history.changes().addChangeListener (this);
    proc.updates->addChangeListener (this);
    changeListenerCallback (nullptr);
    setOpaque (true);
}

TopBar::~TopBar()
{
    proc.history.changes().removeChangeListener (this);
    proc.updates->removeChangeListener (this);
}

void TopBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    undo.setEnabled (proc.history.canUndo());
    redo.setEnabled (proc.history.canRedo());
    about.setDot (proc.updates->getStatus() == UpdateChecker::Status::available);
}

void TopBar::resized()
{
    undo.setBounds (10, 3, 24, height - 6);
    redo.setBounds (36, 3, 24, height - 6);
    about.setBounds (getWidth() - 10 - 62, 3, 62, height - 6);
}

void TopBar::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff1b1d21), 0, 0, Colour (0xff0c0d0f), 0, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawLine (0, 0.5f, b.getRight(), 0.5f, 1.0f);
    g.setColour (Colours::black);
    g.drawLine (0, b.getBottom() - 0.5f, b.getRight(), b.getBottom() - 0.5f, 1.0f);

    // Hairline divider between the history arrows and the rest of the strip.
    g.setColour (pal.legend.withAlpha (0.15f));
    g.drawLine (66.0f, 7.0f, 66.0f, b.getBottom() - 7.0f, 0.8f);
}

//==============================================================================
AboutPanel::AboutPanel (UpdateChecker& checker, const Palette& palette)
    : updates (checker), pal (palette),
      check (palette, "CHECK FOR UPDATES"),
      download (palette, "DOWNLOAD UPDATE")
{
    addAndMakeVisible (check);
    addChildComponent (download);
    download.setLit (true);
    check.onClick = [this] { updates.check(); };
    download.onClick = [this] { updates.openDownload(); };
    updates.addChangeListener (this);
    changeListenerCallback (nullptr);
}

AboutPanel::~AboutPanel()
{
    updates.removeChangeListener (this);
}

void AboutPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    download.setVisible (updates.getStatus() == UpdateChecker::Status::available);
    repaint();
}

juce::String AboutPanel::statusText() const
{
    switch (updates.getStatus())
    {
        case UpdateChecker::Status::checking:  return juce::String::fromUTF8 ("Checking for updates\xe2\x80\xa6");
        case UpdateChecker::Status::upToDate:  return "You have the latest version.";
        case UpdateChecker::Status::available: return "Version " + updates.getLatestVersion() + " is available.";
        case UpdateChecker::Status::failed:    return "Couldn't reach the update server.";
        case UpdateChecker::Status::idle:      break;
    }
    return {};
}

void AboutPanel::resized()
{
    const int y = height - 14 - PillButton::height;
    check.setBounds (16, y, check.getIdealWidth(), PillButton::height);
    download.setBounds (check.getRight() + 8, y, download.getIdealWidth(), PillButton::height);
}

void AboutPanel::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();

    // Same smoked-glass slab as the calibration panel.
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (0, 3).expanded (1.0f), 7.0f);
    g.setGradientFill (ColourGradient (Colour (0xf4121418), 0, 0, Colour (0xf8070809), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (pal.legend.withAlpha (0.28f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 0.8f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (b.getX() + 8, b.getY() + 1.2f, b.getRight() - 8, b.getY() + 1.2f, 0.8f);

    g.setColour (pal.legend);
    draw::text (g, "VIBETRON VT-369", fonts::legend (8.0f, 0.4f), 16, draw::capsBaseline (16.0f, 8.0f));
    g.setColour (pal.sub);
    draw::text (g, "VERSION " + UpdateChecker::currentVersion(), fonts::legend (6.5f, 0.3f), b.getRight() - 16,
                draw::capsBaseline (16.0f, 6.5f), Justification::right);
    g.setColour (pal.legend.withAlpha (0.2f));
    g.drawLine (16, 28, b.getRight() - 16, 28, 0.6f);

    g.setColour (pal.sub);
    draw::text (g, "STEREO HARMONIC RESOLUTION AMPLIFIER", fonts::legend (6.0f, 0.3f), 16, 44);
    g.setColour (Colour (0xffcfc9bd));
    draw::text (g, statusText(), fonts::body (9.5f), 16, 64);
}
