#include "PluginEditor.h"

using namespace ecee::ui;

namespace
{
const juce::Colour kText   { 0xffeae5d6 };
const juce::Colour kScale  { 0xffbfb9a8 };
const juce::Colour kAccent { 0xffd7c08a };
}

Fet8Panel::Fet8Panel (Fet8Processor& p)
    : proc (p),
      input   (p.state, "input",   "INPUT",   { "-20", "-10", "0", "10", "20", "30", "40" }),
      output  (p.state, "output",  "OUTPUT",  { "-30", "-24", "-18", "-12", "-6", "0", "+6", "+12" }),
      attack  (p.state, "attack",  "ATTACK",  { "7", "6", "5", "4", "3", "2", "1" }),
      release (p.state, "release", "RELEASE", { "7", "6", "5", "4", "3", "2", "1" }),
      mixKnob (p.state, "mix",     "MIX",     { "DRY", "", "", "", "WET" }),
      hpf     (p.state, "schpf",   "SC HPF",  Fet8Processor::hpfNames(), "chicken"),
      ratioButtons (*p.state.getParameter ("ratio"), Fet8Processor::ratioNames(), true),
      meterButtons (*p.state.getParameter ("meter"), Fet8Processor::meterNames(), true, juce::Colour (0xffe9f0ff)),
      link (*p.state.getParameter ("link"), { "OFF", "ON" }, "LINK", kText),
      // In level modes the meter follows the output; in GR mode it follows gain reduction.
      vu ([&p]
          {
              const int m = juce::roundToInt (p.state.getRawParameterValue ("meter")->load());
              return m == 0 ? p.meterGr.load() : p.meterOut.load();
          }, VUMeter::Mode::gainReduction, "GAIN REDUCTION"),
      meterMode (*p.state.getParameter ("meter"), [this] (float v)
                 {
                     const int m = juce::roundToInt (v);
                     vu.setLit (m != 3);
                     if (m == 0) { vu.setMode (VUMeter::Mode::gainReduction); vu.setCaption ("GAIN REDUCTION"); }
                     else
                     {
                         vu.setMode (VUMeter::Mode::level);
                         vu.setReference (m == 1 ? -14.0f : -18.0f);
                         vu.setCaption (m == 1 ? "OUTPUT +8" : "OUTPUT +4");
                     }
                 }, nullptr)
{
    attack.setReversed();
    release.setReversed();
    hpf.setKnobScale (0.95f);
    mixKnob.setKnobScale (0.8f);
    attack.setKnobScale (0.85f);
    release.setKnobScale (0.85f);

    for (auto* k : { &input, &output, &attack, &release, &mixKnob, &hpf })
    {
        k->setColours (kText, kScale);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    addAndMakeVisible (ratioButtons);
    addAndMakeVisible (meterButtons);
    addAndMakeVisible (link);
    addAndMakeVisible (vu);
    meterMode.sendInitialUpdate();
}

void Fet8Panel::paint (juce::Graphics& g)
{
    auto panel = drawRackUnit (g, getLocalBounds().toFloat(), juce::Colour (0xff232427), juce::Colour (0xff0e0f10),
                               juce::Colour (0xffb7b8b4), 0.03f);

    // silver header strip
    auto strip = panel.withHeight (44.0f);
    juce::ColourGradient sg (juce::Colour (0xffd9d9d4), strip.getX(), strip.getY(), juce::Colour (0xff8b8b86), strip.getX(), strip.getBottom(), false);
    g.setGradientFill (sg);
    g.fillRect (strip);
    brushed (g, strip, 0.05f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (strip.getX(), strip.getBottom() - 1.5f, strip.getWidth(), 1.5f);

    g.setColour (juce::Colour (0xff141414));
    g.setFont (font (28.0f, true, 0.06f));
    g.drawText ("FET-8", strip.withTrimmedLeft (24.0f), juce::Justification::centredLeft, false);
    g.setFont (font (13.0f, true, 0.35f));
    g.drawText ("LIMITING AMPLIFIER", strip.withTrimmedLeft (132.0f), juce::Justification::centredLeft, false);
    g.setFont (font (22.0f, true, 0.02f));
    g.drawText ("8cee", strip.withTrimmedRight (54.0f), juce::Justification::centredRight, false);
    drawLed (g, { strip.getRight() - 30.0f, strip.getCentreY() }, 5.0f, juce::Colour (0xffff3a24), true);

    // section labels
    panelText (g, "RATIO", { 528.0f, 58.0f, 70.0f, 16.0f }, kAccent, font (12.0f, true, 0.15f));
    panelText (g, "METER", { 612.0f, 58.0f, 70.0f, 16.0f }, kAccent, font (12.0f, true, 0.15f));

    // divider lines
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    for (float x : { 392.0f, 518.0f, 692.0f })
        g.fillRect (x, panel.getY() + 56.0f, 1.5f, panel.getHeight() - 70.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (float x : { 393.5f, 519.5f, 693.5f })
        g.fillRect (x, panel.getY() + 56.0f, 1.0f, panel.getHeight() - 70.0f);

    drawScrew (g, { 60.0f, 336.0f }, 6.0f, 0.3f);
    drawScrew (g, { 940.0f, 336.0f }, 6.0f, 1.1f);
}

void Fet8Panel::resized()
{
    input.setBounds   (50, 80, 170, 220);
    output.setBounds  (218, 80, 170, 220);
    attack.setBounds  (398, 56, 116, 140);
    release.setBounds (398, 200, 116, 140);
    ratioButtons.setBounds (530, 78, 66, 250);
    meterButtons.setBounds (614, 78, 66, 200);
    vu.setBounds (708, 58, 240, 160);
    mixKnob.setBounds (700, 226, 92, 118);
    hpf.setBounds (790, 226, 100, 118);
    link.setBounds (892, 240, 64, 90);
}
