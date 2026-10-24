#pragma once
// 8cee Vintage — boilerplate shared by every processor
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace ecee
{
class ProcessorBase : public juce::AudioProcessor
{
public:
    ProcessorBase()
        : juce::AudioProcessor (BusesProperties()
                                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {}

    using juce::AudioProcessor::processBlock;
    virtual juce::AudioProcessorValueTreeState& apvts() = 0;

    const juce::String getName() const override        { return JucePlugin_Name; }
    bool acceptsMidi() const override                  { return false; }
    bool producesMidi() const override                 { return false; }
    bool isMidiEffect() const override                 { return false; }
    double getTailLengthSeconds() const override       { return 0.0; }
    int getNumPrograms() override                      { return 1; }
    int getCurrentProgram() override                   { return 0; }
    void setCurrentProgram (int) override              {}
    const juce::String getProgramName (int) override   { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    bool hasEditor() const override                    { return true; }
    void releaseResources() override                   {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto out = layouts.getMainOutputChannelSet();
        if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
            return false;
        return out == layouts.getMainInputChannelSet();
    }

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        if (auto xml = apvts().copyState().createXml())
            copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary (data, size))
            if (xml->hasTagName (apvts().state.getType()))
                apvts().replaceState (juce::ValueTree::fromXml (*xml));
    }

    /** Counts processed samples, so the UI can tell whether audio is running (tape reels). */
    std::atomic<juce::int64> samplesProcessed { 0 };

protected:
    static float raw (std::atomic<float>* p) noexcept { return p->load (std::memory_order_relaxed); }
};
} // namespace ecee
