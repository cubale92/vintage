#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Fet8Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB = juce::AudioParameterFloatAttributes().withLabel ("dB");

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "input", 1 }, "Input",
                    juce::NormalisableRange<float> (-20.0f, 40.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                    juce::NormalisableRange<float> (-30.0f, 12.0f, 0.1f), 0.0f, dB));

    juce::NormalisableRange<float> atk (0.02f, 0.8f, 0.001f);
    atk.setSkewForCentre (0.15f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", 1 }, "Attack", atk, 0.25f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 1000.0f)) + " us"; })));

    juce::NormalisableRange<float> rel (50.0f, 1100.0f, 1.0f);
    rel.setSkewForCentre (300.0f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 }, "Release", rel, 250.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ratio", 1 }, "Ratio", ratioNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "meter", 1 }, "Meter", meterNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "schpf", 1 }, "Sidechain HPF", hpfNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, juce::AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "link", 1 }, "Stereo Link", true));

    return { p.begin(), p.end() };
}

Fet8Processor::Fet8Processor()
    : state (*this, nullptr, "FET8", createLayout())
{
    pInput   = state.getRawParameterValue ("input");
    pOutput  = state.getRawParameterValue ("output");
    pAttack  = state.getRawParameterValue ("attack");
    pRelease = state.getRawParameterValue ("release");
    pRatio   = state.getRawParameterValue ("ratio");
    pHpf     = state.getRawParameterValue ("schpf");
    pMix     = state.getRawParameterValue ("mix");
    pLink    = state.getRawParameterValue ("link");

    // ratio: 0=4 1=8 2=12 3=20 4=ALL | schpf: 0=off 1=60 2=100 3=150 4=250 Hz | attack in ms (0.02-0.8), release in ms
    setPresets ({
        { "Init", {} },
        { "Drum Bus Smash",       { { "input", 18 }, { "output", -13 }, { "attack", 0.40f }, { "release", 80 },  { "ratio", 4 }, { "schpf", 2 }, { "mix", 55 } }, "90s NY Boom Bap" },
        { "Parallel NY Drums",    { { "input", 22 }, { "output", -15 }, { "attack", 0.10f }, { "release", 100 }, { "ratio", 4 }, { "schpf", 1 }, { "mix", 35 } }, "90s NY Boom Bap" },
        { "Dusty Breaks",         { { "input", 14 }, { "output", -9 },  { "attack", 0.25f }, { "release", 90 },  { "ratio", 2 }, { "schpf", 1 }, { "mix", 70 } }, "90s NY Boom Bap" },
        { "Room Mic Crush",       { { "input", 26 }, { "output", -4 },  { "attack", 0.02f }, { "release", 50 },  { "ratio", 3 } }, "90s NY Boom Bap" },
        { "Snare Crack",          { { "input", 11 }, { "output", -5 },  { "attack", 0.80f }, { "release", 60 },  { "ratio", 1 }, { "schpf", 2 } }, "90s NY Boom Bap" },
        { "Kick Punch",           { { "input", 7 },  { "output", -3 },  { "attack", 0.80f }, { "release", 150 }, { "ratio", 0 } }, "90s NY Boom Bap" },
        { "Sample Flattener",     { { "input", 12 }, { "output", -7 },  { "attack", 0.15f }, { "release", 250 }, { "ratio", 1 }, { "schpf", 1 } }, "90s NY Boom Bap" },
        { "New Jack Snare",       { { "input", 12 }, { "output", -5 },  { "attack", 0.80f }, { "release", 50 },  { "ratio", 1 }, { "schpf", 2 } }, "New Jack Swing" },
        { "New Jack Drum Machine",{ { "input", 10 }, { "output", -6 },  { "attack", 0.50f }, { "release", 80 },  { "ratio", 0 }, { "schpf", 1 }, { "mix", 75 } }, "New Jack Swing" },
        { "New Jack Synth Bass",  { { "input", 8 },  { "output", -4 },  { "attack", 0.30f }, { "release", 120 }, { "ratio", 0 } }, "New Jack Swing" },
        { "R&B Vocal Smooth",     { { "input", 7 },  { "output", -3 },  { "attack", 0.50f }, { "release", 500 }, { "ratio", 0 }, { "schpf", 3 } }, "R&B" },
        { "R&B Background Vox",   { { "input", 12 }, { "output", -7 },  { "attack", 0.30f }, { "release", 300 }, { "ratio", 1 }, { "schpf", 4 } }, "R&B" },
        { "R&B Bass Smooth",      { { "input", 5 },  { "output", -2 },  { "attack", 0.40f }, { "release", 400 }, { "ratio", 0 } }, "R&B" },
        { "Jazz Drums",           { { "input", 8 },  { "output", -4 },  { "attack", 0.60f }, { "release", 220 }, { "ratio", 0 }, { "schpf", 1 }, { "mix", 60 } }, "Conscious" },
        { "Spoken Word Upfront",  { { "input", 11 }, { "output", -6 },  { "attack", 0.35f }, { "release", 180 }, { "ratio", 1 }, { "schpf", 3 } }, "Conscious" },
        { "Upright Bass",         { { "input", 6 },  { "output", -3 },  { "attack", 0.55f }, { "release", 300 }, { "ratio", 0 } }, "Conscious" },
        { "Vocal Leveler",        { { "input", 9 },  { "output", -4 },  { "attack", 0.45f }, { "release", 400 }, { "ratio", 0 }, { "schpf", 2 } }, "Vocals & Tools" },
        { "Rap Vocal Upfront",    { { "input", 15 }, { "output", -8 },  { "attack", 0.20f }, { "release", 120 }, { "ratio", 2 }, { "schpf", 3 } }, "Vocals & Tools" },
        { "808 Control",          { { "input", 6 },  { "output", -2 },  { "attack", 0.30f }, { "release", 350 }, { "ratio", 0 } }, "Vocals & Tools" },
        { "Peak Catcher",         { { "input", 2 },  { "output", -1 },  { "attack", 0.02f }, { "release", 200 }, { "ratio", 3 } }, "Vocals & Tools" },
    });
}

void Fet8Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    osRate = sampleRate * 2.0;
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    for (int c = 0; c < 2; ++c)
    {
        gr[c] = 0.0f;
        xfmrShelf[c].lowShelf (50.0, 0.8, osRate);
        xfmrLp[c].lowPass (32000.0, 0.6, osRate);
        xfmrShelf[c].reset(); xfmrLp[c].reset(); scHpf[c].reset();
        dc[c].prepare (osRate); dc[c].reset();
    }
    lastHpf = -1;

    inGain.reset (osRate, 0.03);  inGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pInput)));
    outGain.reset (osRate, 0.03); outGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.reset (osRate, 0.03);     mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
}

void Fet8Processor::updateFilters (int hpfIndex)
{
    if (hpfIndex == lastHpf) return;
    lastHpf = hpfIndex;
    const double freqs[] = { 0.0, 60.0, 100.0, 150.0, 250.0 };
    for (auto& f : scHpf)
    {
        if (hpfIndex <= 0) f.setIdentity();
        else               f.highPass (freqs[hpfIndex], 0.707, osRate);
    }
}

void Fet8Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());
    samplesProcessed += buffer.getNumSamples();
    if (numCh == 0) return;

    updateFilters (juce::roundToInt (raw (pHpf)));
    inGain.setTargetValue (ecee::dbToGain (raw (pInput)));
    outGain.setTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.setTargetValue (raw (pMix) * 0.01f);

    const int ratioIdx = juce::roundToInt (raw (pRatio));
    const bool allMode = ratioIdx == 4;
    const float ratios[] = { 4.0f, 8.0f, 12.0f, 20.0f, 20.0f };
    const float ratio  = ratios[ratioIdx];
    const float knee   = allMode ? 0.0f : 4.0f - (float) ratioIdx * 0.6f;
    const float thresh = -10.0f;
    const bool  link   = raw (pLink) > 0.5f || numCh == 1;

    const float attackMs = raw (pAttack) * (allMode ? 1.6f : 1.0f);
    const float atkCoeff = ecee::timeCoeff (attackMs, osRate);
    const float relFast  = ecee::timeCoeff (raw (pRelease), osRate);
    const float relSlow  = ecee::timeCoeff (raw (pRelease) * 2.5f, osRate);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) buffer.getNumSamples());
    auto up = oversampler.processSamplesUp (block);
    const int N = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

    auto target = [&] (float level)
    {
        const float db = ecee::gainToDb (level);
        float t = ecee::computeGR (db, thresh, ratio, knee);
        if (allMode) t *= 1.22f; // "all buttons" over-compression: output dips as input climbs
        return t;
    };
    auto smooth = [&] (float& g, float t)
    {
        if (t > g) g = atkCoeff * g + (1.0f - atkCoeff) * t;
        else
        {
            // program-dependent release: deeper gain reduction recovers more slowly
            const float w = ecee::clamp01 (g / 14.0f);
            const float rc = relFast + (relSlow - relFast) * w;
            g = rc * g + (1.0f - rc) * t;
        }
    };

    float maxGr = 0.0f, peakOut = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        const float ig = inGain.getNextValue(), og = outGain.getNextValue(), m = mix.getNextValue();
        float dry[2], x[2], sc[2];
        for (int c = 0; c < numCh; ++c)
        {
            dry[c] = ch[c][i];
            x[c]   = dry[c] * ig;
            sc[c]  = scHpf[c].process (x[c]);
        }

        if (link)
        {
            float lvl = std::abs (sc[0]);
            if (numCh > 1) lvl = std::max (lvl, std::abs (sc[1]));
            smooth (gr[0], target (lvl));
            gr[1] = gr[0];
        }
        else
        {
            for (int c = 0; c < numCh; ++c)
                smooth (gr[c], target (std::abs (sc[c])));
        }

        for (int c = 0; c < numCh; ++c)
        {
            const float g = gr[c];
            float y = x[c] * ecee::dbToGain (-g);

            // FET stage: distortion rises with gain reduction (and a lot in ALL mode)
            const float depth = ecee::clamp01 (g / 20.0f);
            const float even  = 0.015f + 0.035f * depth + (allMode ? 0.04f : 0.0f);
            const float drive = 0.45f + 0.5f * depth + (allMode ? 0.6f : 0.0f);
            y = y + even * y * y;
            y = std::tanh (y * drive) / drive;
            y = dc[c].process (y);

            // output transformer: a touch of low-end weight and a soft top
            y = xfmrLp[c].process (xfmrShelf[c].process (y));

            y *= og;
            ch[c][i] = y * m + dry[c] * (1.0f - m);

            peakOut = std::max (peakOut, std::abs (ch[c][i]));
            maxGr = std::max (maxGr, g);
        }
    }

    oversampler.processSamplesDown (block);

    meterGr.store (maxGr);
    meterOut.store (ecee::gainToDb (peakOut));
}

juce::AudioProcessorEditor* Fet8Processor::createEditor() { return new Fet8Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Fet8Processor(); }
