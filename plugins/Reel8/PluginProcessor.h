#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** Reel-8 — two-track tape machine: hysteresis saturation, head bump, HF loss, wow & flutter, hiss. */
class Reel8Processor : public ecee::ProcessorBase
{
public:
    Reel8Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meterL { -100.0f }, meterR { -100.0f };

    static juce::StringArray speedNames()   { return { "7.5", "15", "30" }; }
    static juce::StringArray formulaNames() { return { "FERRIC", "CLASSIC", "HI-OUT" }; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    std::atomic<float>* pInput; std::atomic<float>* pOutput; std::atomic<float>* pSpeed; std::atomic<float>* pFormula;
    std::atomic<float>* pBias;  std::atomic<float>* pBump;   std::atomic<float>* pWow;   std::atomic<float>* pHiss;
    std::atomic<float>* pMix;   std::atomic<float>* pAuto;

    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double osRate = 88200.0;

    // per channel
    ecee::Biquad preEmph[2], deEmph[2], biasShelf[2], bump[2], bumpDip[2], lowCut[2], hfLoss[2], hissHp[2], hissTilt[2];
    ecee::DCBlocker dc[2];
    float yPrev[2] { 0, 0 };
    ecee::Noise hissNoise[2] { ecee::Noise (0x1234567u), ecee::Noise (0x7654321u) };

    // wow & flutter (shared by both channels: it's one tape)
    std::vector<float> delayBuf[2];
    int writePos = 0;
    double wowPhase = 0.0, flutterPhase = 0.0;
    ecee::Noise driftNoise { 0xBADC0DEu };
    ecee::OnePole driftLp1, driftLp2;

    juce::SmoothedValue<float> inGain, outGain, mix, wowDepth, hissGain;

    // cached settings to know when filters must be recomputed
    int lastSpeed = -1, lastFormula = -1;
    float lastBias = 99.0f, lastBump = -1.0f;

    // saturation curve (set by formula + bias)
    float satK = 1.0f, satH = 0.2f, satA = 0.05f, satTa = 0.0f, satNorm = 1.0f;
    float flutterHz = 6.5f, hissOffsetDb = 0.0f;
    int baseDelay = 100, delayMask = 0;
    std::vector<float> dryBuf[2];
};
