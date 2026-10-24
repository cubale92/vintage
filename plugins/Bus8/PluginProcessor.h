#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** Bus-8 — stereo VCA bus compressor ("glue") with auto release and analog colour. */
class Bus8Processor : public ecee::ProcessorBase
{
public:
    Bus8Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meterGr { 0.0f };

    static juce::StringArray ratioNames()   { return { "2", "4", "10" }; }
    static juce::StringArray attackNames()  { return { ".1", ".3", "1", "3", "10", "30" }; }
    static juce::StringArray releaseNames() { return { ".1", ".3", ".6", "1.2", "AUTO" }; }
    static juce::StringArray hpfNames()     { return { "OFF", "60", "90", "150", "250" }; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters (int hpfIndex);

    std::atomic<float>* pThresh; std::atomic<float>* pRatio; std::atomic<float>* pAttack; std::atomic<float>* pRelease;
    std::atomic<float>* pMakeup; std::atomic<float>* pHpf;   std::atomic<float>* pMix;    std::atomic<float>* pColor;

    double fs = 44100.0;
    float env2 = 0.0f, gr = 0.0f, grAvg = 0.0f;
    ecee::Biquad scHpf[2];
    ecee::DCBlocker dc[2];
    juce::SmoothedValue<float> makeup, mix, thresh;
    int lastHpf = -1;
};
