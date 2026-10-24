#include "PluginEditor.h"

using namespace ecee::ui;

namespace
{
const juce::Colour kInk      { 0xff26231d };
const juce::Colour kInkSoft  { 0xff4a4538 };
const juce::Colour kDeck     { 0xff17181a };
const juce::Colour kDeckText { 0xffe8e0c8 };
const juce::Rectangle<float> kDeckArea { 60.0f, 58.0f, 880.0f, 226.0f };
}

//==============================================================================
ReelView::ReelView (Reel8Processor& p, float tapePack, juce::Colour deck)
    : proc (p), pack (tapePack), deckColour (deck)
{
    setOpaque (true);
    startTimerHz (40);
}

void ReelView::timerCallback()
{
    const auto count = proc.samplesProcessed.load();
    if (count != lastCount) { lastCount = count; idleTicks = 0; }
    else ++idleTicks;

    if (idleTicks < 6) // audio flowing
    {
        const int speedIdx = juce::roundToInt (proc.state.getRawParameterValue ("speed")->load());
        const float revPerSec = 0.45f * (float) (1 << speedIdx);              // 7.5 / 15 / 30 ips
        const float packR = 0.30f + 0.55f * pack;                              // fuller reel turns slower
        angle += juce::MathConstants<float>::twoPi * revPerSec * (0.55f / packR) / 40.0f;
        if (angle > juce::MathConstants<float>::twoPi) angle -= juce::MathConstants<float>::twoPi;
        repaint();
    }
}

void ReelView::paint (juce::Graphics& g)
{
    g.fillAll (deckColour);
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = std::min (b.getWidth(), b.getHeight()) * 0.5f - 4.0f;

    // shadow
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - R + 3.0f, c.y - R + 5.0f, R * 2.0f, R * 2.0f);

    // tape pack
    const float packR = R * (0.30f + 0.55f * pack);
    juce::ColourGradient tape (juce::Colour (0xff4a3426), c.x - packR, c.y - packR, juce::Colour (0xff1f150f), c.x + packR, c.y + packR, false);
    g.setGradientFill (tape);
    g.fillEllipse (c.x - packR, c.y - packR, packR * 2.0f, packR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (float rr = R * 0.32f; rr < packR; rr += 2.5f)
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);

    // aluminium flange with three windows (rotates)
    juce::Path flange;
    flange.setUsingNonZeroWinding (false);
    flange.addEllipse (-R, -R, R * 2.0f, R * 2.0f);
    for (int i = 0; i < 3; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::twoPi / 3.0f;
        juce::Path w;
        w.addPieSegment (-R * 0.86f, -R * 0.86f, R * 1.72f, R * 1.72f, a - 0.62f, a + 0.62f, 0.34f);
        flange.addPath (w);
    }
    const auto rot = juce::AffineTransform::rotation (angle).translated (c);
    juce::ColourGradient al (juce::Colour (0xffe4e4df), c.x - R, c.y - R, juce::Colour (0xff77776f), c.x + R, c.y + R, false);
    g.setGradientFill (al);
    g.fillPath (flange, rot);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (flange, juce::PathStrokeType (1.0f), rot);

    // rim highlight (static, light comes from top-left)
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    juce::Path rim;
    rim.addCentredArc (c.x, c.y, R - 1.5f, R - 1.5f, 0.0f, -2.4f, -0.6f, true);
    g.strokePath (rim, juce::PathStrokeType (1.6f));

    // hub with three locking tabs
    const float hr = R * 0.17f;
    juce::ColourGradient hub (juce::Colour (0xfff0f0ec), c.x - hr, c.y - hr, juce::Colour (0xff5e5e58), c.x + hr, c.y + hr, false);
    g.setGradientFill (hub);
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
    g.setColour (juce::Colour (0xff202020));
    g.fillEllipse (c.x - hr * 0.42f, c.y - hr * 0.42f, hr * 0.84f, hr * 0.84f);
    for (int i = 0; i < 3; ++i)
    {
        juce::Path tab;
        tab.addRoundedRectangle (-hr * 0.16f, -hr * 0.95f, hr * 0.32f, hr * 0.45f, hr * 0.08f);
        g.fillPath (tab, juce::AffineTransform::rotation (angle + (float) i * juce::MathConstants<float>::twoPi / 3.0f).translated (c));
    }
}

//==============================================================================
Reel8Panel::Reel8Panel (Reel8Processor& p)
    : leftReel  (p, 0.85f, kDeck),
      rightReel (p, 0.35f, kDeck),
      vuL ([&p] { return p.meterL.load(); }, VUMeter::Mode::level, "VU  L"),
      vuR ([&p] { return p.meterR.load(); }, VUMeter::Mode::level, "VU  R"),
      speed    (*p.state.getParameter ("speed"),    Reel8Processor::speedNames(),   "IPS",     kDeckText),
      formula  (*p.state.getParameter ("formula"),  Reel8Processor::formulaNames(), "FORMULA", kDeckText),
      autoGain (*p.state.getParameter ("autogain"), { "OFF", "ON" },               "AUTO GAIN", kDeckText),
      input   (p.state, "input",  "INPUT",       { "-12", "-6", "0", "+6", "+12", "+18" }, "chrome"),
      bias    (p.state, "bias",   "BIAS",        { "UNDER", "", "", "", "OVER" }, "chrome"),
      bump    (p.state, "bump",   "HEAD BUMP",   { "0", "", "", "", "100" }, "chrome"),
      wow     (p.state, "wow",    "WOW / FLUT",  { "0", "", "", "", "100" }, "chrome"),
      hiss    (p.state, "hiss",   "HISS",        { "OFF", "", "", "", "MAX" }, "chrome"),
      mixKnob (p.state, "mix",    "MIX",         { "DRY", "", "", "", "WET" }, "chrome"),
      output  (p.state, "output", "OUTPUT",      { "-18", "-12", "-6", "0", "+6", "+12" }, "chrome")
{
    addAndMakeVisible (leftReel);
    addAndMakeVisible (rightReel);
    addAndMakeVisible (vuL);
    addAndMakeVisible (vuR);
    for (auto* s : { &speed, &formula, &autoGain })
        addAndMakeVisible (*s);

    for (auto* k : { &input, &bias, &bump, &wow, &hiss, &mixKnob, &output })
    {
        k->setColours (kInk, kInkSoft);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    for (auto* k : { &bias, &bump, &wow, &hiss, &mixKnob })
        k->setKnobScale (0.86f);
}

void Reel8Panel::paint (juce::Graphics& g)
{
    auto panel = drawRackUnit (g, getLocalBounds().toFloat(), juce::Colour (0xffded8c6), juce::Colour (0xffb9b19b),
                               juce::Colour (0xff8f8a7e), 0.025f);

    panelText (g, "REEL-8", { panel.getX() + 22.0f, 12.0f, 160.0f, 36.0f }, kInk, font (28.0f, true, 0.05f), juce::Justification::centredLeft);
    g.setColour (kInkSoft);
    g.setFont (font (12.5f, true, 0.35f));
    g.drawText ("MAGNETIC TAPE RECORDER", juce::Rectangle<float> (panel.getX() + 158.0f, 12.0f, 320.0f, 36.0f), juce::Justification::centredLeft, false);
    g.setColour (kInk);
    g.setFont (font (22.0f, true));
    g.drawText ("8cee", juce::Rectangle<float> (panel.getRight() - 160.0f, 12.0f, 136.0f, 36.0f), juce::Justification::centredRight, false);

    // transport deck
    drawRecess (g, kDeckArea, 8.0f, kDeck);
    brushed (g, kDeckArea.reduced (6.0f), 0.015f);

    // tape path: from the supply reel, round two guides, across the heads, to the take-up reel
    const juce::Point<float> guideL (300.0f, 272.0f), guideR (700.0f, 272.0f);
    g.setColour (juce::Colour (0xff3a2a1f));
    g.drawLine (juce::Line<float> ({ 160.0f, 272.0f }, guideL), 3.0f);
    g.drawLine (juce::Line<float> (guideL, guideR), 3.0f);
    g.drawLine (juce::Line<float> (guideR, { 840.0f, 272.0f }), 3.0f);
    for (auto gp : { guideL, guideR })
    {
        g.setColour (juce::Colour (0xffb8b8b2));
        g.fillEllipse (gp.x - 7.0f, gp.y - 7.0f, 14.0f, 14.0f);
        g.setColour (juce::Colour (0xff222222));
        g.fillEllipse (gp.x - 2.5f, gp.y - 2.5f, 5.0f, 5.0f);
    }

    // head block
    auto heads = juce::Rectangle<float> (470.0f, 263.0f, 60.0f, 18.0f);
    juce::ColourGradient hg (juce::Colour (0xffd4d4cf), heads.getX(), heads.getY(), juce::Colour (0xff6f6f69), heads.getX(), heads.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRoundedRectangle (heads, 3.0f);
    g.setColour (juce::Colour (0xff2b2b2b));
    for (int i = 0; i < 3; ++i)
        g.fillRect (heads.getX() + 9.0f + (float) i * 17.0f, heads.getY() + 3.0f, 8.0f, heads.getHeight() - 6.0f);

    // knob row label strip
    g.setColour (kInk.withAlpha (0.25f));
    g.fillRect (panel.getX() + 20.0f, 296.0f, panel.getWidth() - 40.0f, 1.0f);

    drawLed (g, { 520.0f, 30.0f }, 5.0f, juce::Colour (0xffff3b26), true);
    g.setColour (kInkSoft);
    g.setFont (font (11.0f, true, 0.25f));
    g.drawText ("REC", juce::Rectangle<float> (532.0f, 22.0f, 50.0f, 16.0f), juce::Justification::centredLeft, false);
}

void Reel8Panel::resized()
{
    leftReel.setBounds  (66, 70, 190, 190);
    rightReel.setBounds (744, 70, 190, 190);
    vuL.setBounds (300, 70, 196, 124);
    vuR.setBounds (504, 70, 196, 124);
    speed.setBounds    (304, 196, 120, 62);
    formula.setBounds  (430, 196, 140, 62);
    autoGain.setBounds (580, 196, 120, 62);

    input.setBounds   (60, 302, 140, 160);
    bias.setBounds    (200, 306, 120, 150);
    bump.setBounds    (320, 306, 120, 150);
    wow.setBounds     (440, 306, 120, 150);
    hiss.setBounds    (560, 306, 120, 150);
    mixKnob.setBounds (680, 306, 120, 150);
    output.setBounds  (800, 302, 140, 160);
}
