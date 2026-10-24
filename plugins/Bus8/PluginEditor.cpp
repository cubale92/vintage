#include "PluginEditor.h"

using namespace ecee::ui;

namespace
{
const juce::Colour kText  { 0xfff0ead8 };
const juce::Colour kScale { 0xffd9d2bd };
const juce::Colour kRed   { 0xffd2473a };
}

Bus8Panel::Bus8Panel (Bus8Processor& p)
    : threshold (p.state, "threshold", "THRESHOLD", { "-30", "-20", "-10", "0", "+10" }),
      ratio     (p.state, "ratio",     "RATIO",     Bus8Processor::ratioNames(),   "chicken"),
      attack    (p.state, "attack",    "ATTACK ms", Bus8Processor::attackNames(),  "chicken"),
      release   (p.state, "release",   "RELEASE s", Bus8Processor::releaseNames(), "chicken"),
      makeup    (p.state, "makeup",    "MAKE-UP",   { "0", "4", "8", "12", "16", "20" }),
      hpf       (p.state, "schpf",     "SC HPF",    Bus8Processor::hpfNames(),     "chicken"),
      mixKnob   (p.state, "mix",       "MIX",       { "DRY", "", "", "", "WET" }),
      colour    (*p.state.getParameter ("color"), { "CLEAN", "COLOUR" }, "ANALOG", kText),
      vu ([&p] { return p.meterGr.load(); }, VUMeter::Mode::gainReduction, "COMP")
{
    for (auto* k : { &threshold, &ratio, &attack, &release, &makeup, &hpf, &mixKnob })
    {
        k->setColours (kText, kScale);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    for (auto* k : { &ratio, &attack, &release, &hpf })
        k->setKnobScale (0.92f);
    mixKnob.setKnobScale (0.85f);
    addAndMakeVisible (colour);
    addAndMakeVisible (vu);
}

void Bus8Panel::paint (juce::Graphics& g)
{
    auto panel = drawRackUnit (g, getLocalBounds().toFloat(), juce::Colour (0xff59636e), juce::Colour (0xff353c44),
                               juce::Colour (0xff6b6e72), 0.03f);

    panelText (g, "BUS-8", { panel.getX() + 22.0f, 14.0f, 140.0f, 34.0f }, kText, font (28.0f, true, 0.05f), juce::Justification::centredLeft);
    panelText (g, "STEREO BUS COMPRESSOR", { panel.getX() + 140.0f, 14.0f, 300.0f, 34.0f }, kText.withAlpha (0.8f),
               font (12.5f, true, 0.35f), juce::Justification::centredLeft);
    panelText (g, "8cee", { panel.getRight() - 140.0f, 14.0f, 116.0f, 34.0f }, kText, font (22.0f, true), juce::Justification::centredRight);

    // red pinstripe under the header
    g.setColour (kRed);
    g.fillRect (panel.getX() + 20.0f, 52.0f, panel.getWidth() - 40.0f, 2.0f);

    // meter well
    drawRecess (g, { 60.0f, 70.0f, 262.0f, 176.0f }, 6.0f, juce::Colour (0xff1b1e22));

    // section boxes
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle ({ 470.0f, 210.0f, 270.0f, 118.0f }, 6.0f, 1.2f);
    panelText (g, "SIDECHAIN / BLEND", { 470.0f, 316.0f, 270.0f, 14.0f }, kText.withAlpha (0.55f), font (9.5f, true, 0.3f));

    drawLed (g, { 92.0f, 280.0f }, 5.0f, juce::Colour (0xff59ff6a), true);
    panelText (g, "POWER", { 104.0f, 272.0f, 80.0f, 16.0f }, kText.withAlpha (0.8f), font (11.0f, true, 0.2f), juce::Justification::centredLeft);
    drawScrew (g, { 60.0f, 318.0f }, 6.0f, 0.4f);
    drawScrew (g, { 940.0f, 318.0f }, 6.0f, 1.3f);
}

void Bus8Panel::resized()
{
    vu.setBounds (66, 76, 250, 164);
    threshold.setBounds (330, 62, 150, 170);
    ratio.setBounds   (482, 66, 118, 140);
    attack.setBounds  (600, 66, 118, 140);
    release.setBounds (718, 66, 118, 140);
    makeup.setBounds  (830, 62, 124, 170);
    hpf.setBounds     (478, 212, 118, 108);
    mixKnob.setBounds (604, 212, 118, 108);
    colour.setBounds  (770, 236, 110, 80);
}
