#pragma once
#include "PluginProcessor.h"
#include "EceeUI.h"

class Fet8Panel : public juce::Component
{
public:
    explicit Fet8Panel (Fet8Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1000, H = 360;

private:
    Fet8Processor& proc;
    ecee::ui::Knob input, output, attack, release, mixKnob, hpf;
    ecee::ui::ChoiceButtons ratioButtons, meterButtons;
    ecee::ui::BatSwitch link;
    ecee::ui::VUMeter vu;
    juce::ParameterAttachment meterMode;
};

class Fet8Editor : public ecee::ui::ScaledEditor
{
public:
    explicit Fet8Editor (Fet8Processor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Fet8Panel::W, Fet8Panel::H); }

private:
    Fet8Panel panel;
};
