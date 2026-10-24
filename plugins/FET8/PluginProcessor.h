#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** FET-8 Limiting Amplifier — fast FET-style peak compressor with transformer colour. */
class Fet8Processor : public ecee::ProcessorBase
{
public:
    Fet8Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;

    // meter feeds (read by the UI)
    std::atomic<float> meterGr { 0.0f }, meterOut { -100.0f };

    static juce::StringArray ratioNames()  { return { "4", "8", "12", "20", "ALL" }; }
    static juce::StringArray meterNames()  { return { "GR", "+8", "+4", "OFF" }; }
    static juce::StringArray hpfNames()    { return { "OFF", "60", "100", "150", "250" }; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters (int hpfIndex);

    std::atomic<float>* pInput; std::atomic<float>* pOutput; std::atomic<float>* pAttack; std::atomic<float>* pRelease;
    std::atomic<float>* pRatio; std::atomic<float>* pHpf;   std::atomic<float>* pMix;    std::atomic<float>* pLink;

    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double osRate = 88200.0;

    float gr[2] { 0.0f, 0.0f };
    ecee::Biquad scHpf[2], xfmrShelf[2], xfmrLp[2];
    ecee::DCBlocker dc[2];
    juce::SmoothedValue<float> inGain, outGain, mix;
    int lastHpf = -1;
};
