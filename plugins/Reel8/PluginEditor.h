#pragma once
#include "PluginProcessor.h"
#include "EceeUI.h"

/** One spinning tape reel. Turns only while the host is actually running audio through the plug-in. */
class ReelView : public juce::Component, private juce::Timer
{
public:
    ReelView (Reel8Processor&, float tapePack, juce::Colour deck);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    Reel8Processor& proc;
    float pack;           // 0..1, how much tape is on this reel
    juce::Colour deckColour;
    float angle = 0.0f;
    juce::int64 lastCount = 0;
    int idleTicks = 100;
};

class Reel8Panel : public juce::Component
{
public:
    explicit Reel8Panel (Reel8Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1000, H = 470;

private:
    ReelView leftReel, rightReel;
    ecee::ui::VUMeter vuL, vuR;
    ecee::ui::BatSwitch speed, formula, autoGain;
    ecee::ui::Knob input, bias, bump, wow, hiss, mixKnob, output;
};

class Reel8Editor : public ecee::ui::ScaledEditor
{
public:
    explicit Reel8Editor (Reel8Processor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Reel8Panel::W, Reel8Panel::H); }

private:
    Reel8Panel panel;
};
