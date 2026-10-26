#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Bus8Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB = juce::AudioParameterFloatAttributes().withLabel ("dB");

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "threshold", 1 }, "Threshold",
                    juce::NormalisableRange<float> (-30.0f, 10.0f, 0.1f), -10.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ratio", 1 }, "Ratio", ratioNames(), 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "attack", 1 }, "Attack (ms)", attackNames(), 5));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "release", 1 }, "Release (s)", releaseNames(), 4));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "makeup", 1 }, "Make-Up",
                    juce::NormalisableRange<float> (0.0f, 20.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "schpf", 1 }, "Sidechain HPF", hpfNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, juce::AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "color", 1 }, "Analog Colour", true));

    return { p.begin(), p.end() };
}

Bus8Processor::Bus8Processor()
    : state (*this, nullptr, "BUS8", createLayout())
{
    pThresh  = state.getRawParameterValue ("threshold");
    pRatio   = state.getRawParameterValue ("ratio");
    pAttack  = state.getRawParameterValue ("attack");
    pRelease = state.getRawParameterValue ("release");
    pMakeup  = state.getRawParameterValue ("makeup");
    pHpf     = state.getRawParameterValue ("schpf");
    pMix     = state.getRawParameterValue ("mix");
    pColor   = state.getRawParameterValue ("color");

    // ratio: 0=2 1=4 2=10 | attack: 0=.1 1=.3 2=1 3=3 4=10 5=30 ms | release: 0=.1 1=.3 2=.6 3=1.2 s 4=AUTO | schpf: 0=off 1=60 2=90 3=150 4=250 Hz
    setPresets ({
        { "Init", {} },
        { "Mix Bus Glue",         { { "threshold", -12 }, { "ratio", 0 }, { "attack", 5 }, { "release", 4 }, { "makeup", 2 },  { "schpf", 1 } } },
        { "Drum Bus Punch",       { { "threshold", -18 }, { "ratio", 1 }, { "attack", 4 }, { "release", 0 }, { "makeup", 4 },  { "schpf", 2 } } },
        { "Boom Bap Pump",        { { "threshold", -22 }, { "ratio", 2 }, { "attack", 3 }, { "release", 0 }, { "makeup", 7 } } },
        { "Parallel Drum Crush",  { { "threshold", -28 }, { "ratio", 2 }, { "attack", 0 }, { "release", 0 }, { "makeup", 12 }, { "mix", 40 } } },
        { "Trap Drums Tight",     { { "threshold", -16 }, { "ratio", 1 }, { "attack", 2 }, { "release", 1 }, { "makeup", 4 },  { "schpf", 2 } } },
        { "Sample Chop Glue",     { { "threshold", -14 }, { "ratio", 1 }, { "attack", 3 }, { "release", 1 }, { "makeup", 3 },  { "schpf", 3 } } },
        { "Loop Leveler",         { { "threshold", -16 }, { "ratio", 1 }, { "attack", 2 }, { "release", 2 }, { "makeup", 4 } } },
        { "Vocal Bus",            { { "threshold", -14 }, { "ratio", 0 }, { "attack", 4 }, { "release", 1 }, { "makeup", 2 },  { "schpf", 3 } } },
        { "Master Gentle",        { { "threshold", -8 },  { "ratio", 0 }, { "attack", 5 }, { "release", 4 }, { "makeup", 1 },  { "schpf", 1 } } },
        { "Master Loud",          { { "threshold", -16 }, { "ratio", 1 }, { "attack", 4 }, { "release", 4 }, { "makeup", 5 },  { "schpf", 2 } } },
        { "Clean Glue (No Colour)", { { "threshold", -12 }, { "ratio", 0 }, { "attack", 5 }, { "release", 4 }, { "makeup", 2 }, { "color", 0 } } },
    });
}

void Bus8Processor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;
    env2 = gr = grAvg = 0.0f;
    for (auto& d : dc) { d.prepare (fs); d.reset(); }
    for (auto& f : scHpf) f.reset();
    lastHpf = -1;
    makeup.reset (fs, 0.03); makeup.setCurrentAndTargetValue (ecee::dbToGain (raw (pMakeup)));
    mix.reset (fs, 0.03);    mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
    thresh.reset (fs, 0.05); thresh.setCurrentAndTargetValue (raw (pThresh));
}

void Bus8Processor::updateFilters (int hpfIndex)
{
    if (hpfIndex == lastHpf) return;
    lastHpf = hpfIndex;
    const double freqs[] = { 0.0, 60.0, 90.0, 150.0, 250.0 };
    for (auto& f : scHpf)
    {
        if (hpfIndex <= 0) f.setIdentity();
        else               f.highPass (freqs[hpfIndex], 0.707, fs);
    }
}

void Bus8Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    samplesProcessed += n;
    if (numCh == 0) return;

    updateFilters (juce::roundToInt (raw (pHpf)));
    makeup.setTargetValue (ecee::dbToGain (raw (pMakeup)));
    mix.setTargetValue (raw (pMix) * 0.01f);
    thresh.setTargetValue (raw (pThresh));

    const float ratios[]   = { 2.0f, 4.0f, 10.0f };
    const float attacks[]  = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
    const float releases[] = { 100.0f, 300.0f, 600.0f, 1200.0f, -1.0f };

    const float ratio   = ratios[juce::jlimit (0, 2, juce::roundToInt (raw (pRatio)))];
    const float atkMs   = attacks[juce::jlimit (0, 5, juce::roundToInt (raw (pAttack)))];
    const float relMs   = releases[juce::jlimit (0, 4, juce::roundToInt (raw (pRelease)))];
    const bool  autoRel = relMs < 0.0f;
    const bool  colour  = raw (pColor) > 0.5f;
    const float knee    = 6.0f;

    const float atkC   = ecee::timeCoeff (atkMs, fs);
    const float relC   = ecee::timeCoeff (autoRel ? 600.0f : relMs, fs);
    const float relFast = ecee::timeCoeff (80.0f, fs);
    const float relSlow = ecee::timeCoeff (1100.0f, fs);
    const float rmsC   = ecee::timeCoeff (4.0f, fs);
    const float avgC   = ecee::timeCoeff (1500.0f, fs);

    float* ch[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // linked stereo detector, short RMS window
        float s = 0.0f;
        for (int c = 0; c < numCh; ++c)
            s = std::max (s, std::abs (scHpf[c].process (ch[c][i])));
        env2 = rmsC * env2 + (1.0f - rmsC) * s * s;
        const float levelDb = ecee::gainToDb (std::sqrt (env2) * 1.4142f);

        const float t = ecee::computeGR (levelDb, thresh.getNextValue(), ratio, knee);

        if (t > gr) gr = atkC * gr + (1.0f - atkC) * t;
        else
        {
            float rc = relC;
            if (autoRel)
            {
                // transients (GR well above its long-term average) recover fast, sustained GR recovers slowly
                const float sustained = ecee::clamp01 (grAvg / std::max (gr, 0.05f));
                rc = relFast + (relSlow - relFast) * sustained;
            }
            gr = rc * gr + (1.0f - rc) * t;
        }
        grAvg = avgC * grAvg + (1.0f - avgC) * gr;

        const float g = ecee::dbToGain (-gr) * makeup.getNextValue();
        const float m = mix.getNextValue();

        for (int c = 0; c < numCh; ++c)
        {
            const float dry = ch[c][i];
            float y = dry * g;
            if (colour)
            {
                // VCA + output stage: a hint of 2nd harmonic and gentle rounding of the hottest peaks
                y = y + 0.006f * y * y;
                y = std::tanh (y * 0.5f) * 2.0f;
                y = dc[c].process (y);
            }
            ch[c][i] = y * m + dry * (1.0f - m);
        }
        maxGr = std::max (maxGr, gr);
    }

    meterGr.store (maxGr);
}

juce::AudioProcessorEditor* Bus8Processor::createEditor() { return new Bus8Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Bus8Processor(); }
