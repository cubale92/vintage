#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Reel8Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB  = juce::AudioParameterFloatAttributes().withLabel ("dB");
    auto pct = juce::AudioParameterFloatAttributes().withLabel ("%");

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "input", 1 }, "Input",
                    juce::NormalisableRange<float> (-12.0f, 18.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                    juce::NormalisableRange<float> (-18.0f, 12.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "speed", 1 }, "Tape Speed (ips)", speedNames(), 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "formula", 1 }, "Tape Formula", formulaNames(), 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bias", 1 }, "Bias",
                    juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        if (std::abs (v) < 0.02f) return juce::String ("Nominal");
                        return (v < 0 ? juce::String ("Under ") : juce::String ("Over ")) + juce::String (std::abs (v) * 100.0f, 0) + "%";
                    })));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bump", 1 }, "Head Bump",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "wow", 1 }, "Wow & Flutter",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 15.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "hiss", 1 }, "Hiss",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "autogain", 1 }, "Auto Gain", true));

    return { p.begin(), p.end() };
}

Reel8Processor::Reel8Processor()
    : state (*this, nullptr, "REEL8", createLayout())
{
    pInput   = state.getRawParameterValue ("input");
    pOutput  = state.getRawParameterValue ("output");
    pSpeed   = state.getRawParameterValue ("speed");
    pFormula = state.getRawParameterValue ("formula");
    pBias    = state.getRawParameterValue ("bias");
    pBump    = state.getRawParameterValue ("bump");
    pWow     = state.getRawParameterValue ("wow");
    pHiss    = state.getRawParameterValue ("hiss");
    pMix     = state.getRawParameterValue ("mix");
    pAuto    = state.getRawParameterValue ("autogain");

    // speed: 0=7.5 1=15 2=30 ips | formula: 0=ferric 1=classic 2=hi-out | bias -1 (under: brighter, dirtier) .. +1 (over: darker, cleaner)
    setPresets ({
        { "Init", {} },
        { "Dusty Sampler",        { { "input", 9 },  { "speed", 0 }, { "formula", 0 }, { "bias", 0.5f },  { "bump", 80 }, { "wow", 30 }, { "hiss", 20 } }, "90s NY Boom Bap" },
        { "Drum Tape Smash",      { { "input", 12 }, { "speed", 1 }, { "formula", 0 }, { "bias", -0.3f }, { "bump", 60 }, { "wow", 8 } }, "90s NY Boom Bap" },
        { "Cassette Deck",        { { "input", 6 },  { "speed", 0 }, { "formula", 0 }, { "bias", 0.4f },  { "bump", 70 }, { "wow", 45 }, { "hiss", 35 } }, "90s NY Boom Bap" },
        { "Bass Thickener",       { { "input", 6 },  { "speed", 1 }, { "formula", 1 }, { "bump", 90 }, { "wow", 0 } }, "90s NY Boom Bap" },
        { "Parallel Saturation",  { { "input", 15 }, { "speed", 1 }, { "formula", 0 }, { "bump", 60 }, { "wow", 6 },  { "mix", 35 } }, "90s NY Boom Bap" },
        { "New Jack Shine",       { { "input", 4 },  { "speed", 2 }, { "formula", 2 }, { "bias", -0.4f }, { "bump", 40 }, { "wow", 3 } }, "New Jack Swing" },
        { "New Jack Drum Punch",  { { "input", 8 },  { "speed", 1 }, { "formula", 2 }, { "bias", -0.2f }, { "bump", 50 }, { "wow", 2 } }, "New Jack Swing" },
        { "R&B Silk 30 IPS",      { { "input", 2 },  { "speed", 2 }, { "formula", 1 }, { "bias", 0.1f },  { "bump", 35 }, { "wow", 4 } }, "R&B" },
        { "R&B Warm Keys",        { { "input", 4 },  { "speed", 1 }, { "formula", 1 }, { "bias", 0.3f },  { "bump", 50 }, { "wow", 15 }, { "hiss", 4 } }, "R&B" },
        { "Keys & Rhodes Warmth", { { "input", 5 },  { "speed", 1 }, { "formula", 1 }, { "bias", 0.2f },  { "bump", 45 }, { "wow", 22 }, { "hiss", 8 } }, "R&B" },
        { "Jazz Sample Warmth",   { { "input", 6 },  { "speed", 1 }, { "formula", 0 }, { "bias", 0.4f },  { "bump", 60 }, { "wow", 18 }, { "hiss", 10 } }, "Conscious" },
        { "Spoken Word Tape",     { { "input", 3 },  { "speed", 1 }, { "formula", 1 }, { "bias", 0.2f },  { "bump", 30 }, { "wow", 5 } }, "Conscious" },
        { "Horns & Upright Bass", { { "input", 5 },  { "speed", 1 }, { "formula", 1 }, { "bias", 0.3f },  { "bump", 65 }, { "wow", 10 }, { "hiss", 6 } }, "Conscious" },
        { "Mix Bus Glue",         { { "input", 0 },  { "speed", 2 }, { "formula", 2 }, { "bump", 30 }, { "wow", 5 } }, "Mix & FX" },
        { "Warm 15 IPS",          { { "input", 3 },  { "speed", 1 }, { "formula", 1 }, { "bump", 55 }, { "wow", 12 }, { "hiss", 5 } }, "Mix & FX" },
        { "Vocal Tape Sheen",     { { "input", 2 },  { "speed", 2 }, { "formula", 2 }, { "bias", -0.2f }, { "bump", 20 }, { "wow", 4 } }, "Mix & FX" },
        { "Wobbly VHS",           { { "input", 4 },  { "speed", 0 }, { "formula", 0 }, { "bias", 0.6f },  { "bump", 65 }, { "wow", 85 }, { "hiss", 40 } }, "Mix & FX" },
        { "Clean Hi-Fi Print",    { { "input", -3 }, { "speed", 2 }, { "formula", 2 }, { "bias", 0.2f },  { "bump", 20 }, { "wow", 0 } }, "Mix & FX" },
    });
}

void Reel8Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    osRate = sampleRate * 2.0;

    // wow/flutter delay: wet and dry both run through a fixed base delay so the Mix knob stays phase-coherent
    baseDelay = (int) std::ceil (0.001 * osRate) + 4;
    if (baseDelay % 2 != 0) ++baseDelay;
    int size = 1;
    while (size < baseDelay * 2 + 8) size <<= 1;
    delayMask = size - 1;
    for (int c = 0; c < 2; ++c)
    {
        delayBuf[c].assign ((size_t) size, 0.0f);
        dryBuf[c].assign ((size_t) size, 0.0f);
        yPrev[c] = 0.0f;
        dc[c].prepare (osRate); dc[c].reset();
        for (auto* f : { &preEmph[c], &deEmph[c], &biasShelf[c], &bump[c], &bumpDip[c], &lowCut[c], &hfLoss[c], &hissHp[c], &hissTilt[c] })
            f->reset();
        hissHp[c].highPass (250.0, 0.6, osRate);
        hissTilt[c].highShelf (5000.0, 4.0, osRate);
    }
    writePos = 0;
    wowPhase = flutterPhase = 0.0;
    driftLp1.setCutoff (1.2, osRate); driftLp1.reset();
    driftLp2.setCutoff (1.2, osRate); driftLp2.reset();

    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()) + baseDelay / 2);

    lastSpeed = lastFormula = -1;
    lastBias = 99.0f; lastBump = -1.0f;
    updateFilters();

    inGain.reset (osRate, 0.03);   inGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pInput)));
    outGain.reset (osRate, 0.03);  outGain.setCurrentAndTargetValue (1.0f);
    mix.reset (osRate, 0.03);      mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
    wowDepth.reset (osRate, 0.1);  wowDepth.setCurrentAndTargetValue (0.0f);
    hissGain.reset (osRate, 0.05); hissGain.setCurrentAndTargetValue (0.0f);
}

void Reel8Processor::updateFilters()
{
    const int   speed   = juce::jlimit (0, 2, juce::roundToInt (raw (pSpeed)));
    const int   formula = juce::jlimit (0, 2, juce::roundToInt (raw (pFormula)));
    const float bias    = raw (pBias);
    const float bumpAmt = raw (pBump) * 0.01f;

    if (speed == lastSpeed && formula == lastFormula && std::abs (bias - lastBias) < 0.005f && std::abs (bumpAmt - lastBump) < 0.002f)
        return;
    lastSpeed = speed; lastFormula = formula; lastBias = bias; lastBump = bumpAmt;

    const double emphF[]  = { 2000.0, 3200.0, 4500.0 };
    const double bumpF[]  = { 38.0, 58.0, 95.0 };
    const double hfF[]    = { 14000.0, 20000.0, 24000.0 };
    const float  flutF[]  = { 4.0f, 6.5f, 9.0f };
    const float  hissOff[] = { 4.0f, 0.0f, -4.0f };

    const float  kF[]     = { 1.35f, 1.0f, 0.75f };   // drive into the curve (lower = more headroom)
    const float  hF[]     = { 0.28f, 0.20f, 0.13f };  // hysteresis (memory) amount
    const float  aF[]     = { 0.10f, 0.06f, 0.03f };  // asymmetry -> even harmonics
    const double emphDb[] = { 7.0, 6.0, 5.0 };

    satK = kF[formula] * (1.0f - 0.25f * bias);   // under-bias drives harder, over-bias is cleaner
    satH = hF[formula];
    satA = aF[formula];
    satTa = std::tanh (satK * satA);
    satNorm = 1.0f / (satK * (1.0f - satTa * satTa));
    flutterHz = flutF[speed];
    hissOffsetDb = hissOff[speed];

    for (int c = 0; c < 2; ++c)
    {
        preEmph[c].highShelf (emphF[speed], emphDb[formula], osRate);
        deEmph[c].highShelf (emphF[speed], -emphDb[formula], osRate);
        biasShelf[c].highShelf (7000.0, -2.5 * bias, osRate);
        bump[c].peak (bumpF[speed], 1.1, 4.5 * bumpAmt, osRate);
        bumpDip[c].peak (bumpF[speed] * 2.3, 1.0, -1.6 * bumpAmt, osRate);
        lowCut[c].highPass (bumpF[speed] * 0.42, 0.55, osRate);
        hfLoss[c].lowPass (hfF[speed], 0.707, osRate);
    }
}

void Reel8Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());
    samplesProcessed += buffer.getNumSamples();
    if (numCh == 0) return;

    updateFilters();

    const float inDb = raw (pInput);
    const float comp = raw (pAuto) > 0.5f ? -0.9f * inDb : 0.0f;
    inGain.setTargetValue (ecee::dbToGain (inDb));
    outGain.setTargetValue (ecee::dbToGain (raw (pOutput) + comp));
    mix.setTargetValue (raw (pMix) * 0.01f);

    const float wf = raw (pWow) * 0.01f;
    wowDepth.setTargetValue (wf * wf * (float) (0.001 * osRate));   // squared: fine control at low settings
    const float h = raw (pHiss) * 0.01f;
    hissGain.setTargetValue (h < 0.001f ? 0.0f : ecee::dbToGain (-96.0f + 46.0f * h + hissOffsetDb));

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) buffer.getNumSamples());
    auto up = oversampler.processSamplesUp (block);
    const int N = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

    const double wowInc = 0.55 / osRate, flutInc = (double) flutterHz / osRate;

    for (int i = 0; i < N; ++i)
    {
        const float ig = inGain.getNextValue(), og = outGain.getNextValue(), m = mix.getNextValue();
        const float depth = wowDepth.getNextValue(), hg = hissGain.getNextValue();

        // tape speed modulation
        wowPhase += wowInc;   if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += flutInc; if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        const float drift = juce::jlimit (-0.4f, 0.4f, 70.0f * driftLp2.process (driftLp1.process (driftNoise.next())));
        const float mod = 0.55f * std::sin ((float) (wowPhase * 6.283185307))
                        + 0.15f * std::sin ((float) (flutterPhase * 6.283185307)) + drift;
        const float delay = juce::jlimit (2.0f, (float) (delayMask - 4), (float) baseDelay + depth * mod);

        const float rp = (float) writePos - delay;
        const int   i1 = (int) std::floor (rp);
        const float fr = rp - (float) i1;

        for (int c = 0; c < numCh; ++c)
        {
            const float dry = ch[c][i];
            float x = preEmph[c].process (dry * ig);

            // hysteresis-flavoured saturation: soft asymmetric curve with feedback memory
            const float s = x + satH * yPrev[c];
            float y = (std::tanh (satK * (s + satA)) - satTa) * satNorm;
            yPrev[c] = y;
            y *= (1.0f - satH);

            y = dc[c].process (deEmph[c].process (y));
            y = biasShelf[c].process (y);
            y = bumpDip[c].process (bump[c].process (y));
            y = hfLoss[c].process (lowCut[c].process (y));

            // wow & flutter through a modulated delay (cubic interpolation)
            auto& buf = delayBuf[c];
            buf[(size_t) writePos] = y;
            dryBuf[c][(size_t) writePos] = dry;
            const float y0 = buf[(size_t) ((i1 - 1) & delayMask)], y1 = buf[(size_t) (i1 & delayMask)];
            const float y2 = buf[(size_t) ((i1 + 1) & delayMask)], y3 = buf[(size_t) ((i1 + 2) & delayMask)];
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            float wet = ((c3 * fr + c2) * fr + c1) * fr + y1;

            if (hg > 0.0f)
                wet += hissTilt[c].process (hissHp[c].process (hissNoise[c].next())) * hg;

            const float dryAligned = dryBuf[c][(size_t) ((writePos - baseDelay) & delayMask)];
            ch[c][i] = wet * og * m + dryAligned * (1.0f - m);
        }
        writePos = (writePos + 1) & delayMask;
    }

    oversampler.processSamplesDown (block);

    // VU feed: block RMS per channel
    const int n = buffer.getNumSamples();
    for (int c = 0; c < 2; ++c)
    {
        const int src = std::min (c, numCh - 1);
        const float rms = buffer.getRMSLevel (src, 0, n);
        (c == 0 ? meterL : meterR).store (ecee::gainToDb (rms * 1.4142f));
    }
}

juce::AudioProcessorEditor* Reel8Processor::createEditor() { return new Reel8Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Reel8Processor(); }
