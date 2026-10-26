#pragma once
#include "PluginProcessor.h"
#include "EceeUI.h"

class Bus8Panel : public juce::Component
{
public:
    explicit Bus8Panel (Bus8Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1000, H = 340;

private:
    ecee::ui::Knob threshold, ratio, attack, release, makeup, hpf, mixKnob;
    ecee::ui::BatSwitch colour;
    ecee::ui::VUMeter vu;
    ecee::ui::PresetBar presetBar;
};

class Bus8Editor : public ecee::ui::ScaledEditor
{
public:
    explicit Bus8Editor (Bus8Processor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Bus8Panel::W, Bus8Panel::H); }

private:
    Bus8Panel panel;
};
