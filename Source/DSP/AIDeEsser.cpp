#include "AIDeEsser.h"

void AIDeEsser::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, (juce::uint32) juce::jmax (1, numChannels) };
    splitter.prepare (spec);
    splitter.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    splitter.setCutoffFrequency (freq);
    highBand.setSize (juce::jmax (1, numChannels), maxBlock);

    attCoef = std::exp (-1.0f / (0.5f * 0.001f * (float) sr));   // 0.5 ms
    relCoef = std::exp (-1.0f / (60.0f * 0.001f * (float) sr));  // 60 ms
    reset();
}

void AIDeEsser::reset()
{
    splitter.reset();
    envDb = -90.0f;
    grDb = 0.0f;
}

void AIDeEsser::setParameters (bool enabled, float freqHz, float thresholdDb,
                               float ratioV, bool autoMode)
{
    on = enabled;
    autoM = autoMode;
    ratio = juce::jmax (1.0f, ratioV);
    thresh = thresholdDb;
    if (! juce::approximatelyEqual (freq, freqHz))
    {
        freq = freqHz;
        splitter.setCutoffFrequency (freq);
    }
}

void AIDeEsser::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
    {
        grDb = 0.0f;
        return;
    }

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    highBand.setSize (numCh, numSmp, false, false, true);

    // Copy input to high band, then extract HF via Linkwitz-Riley HP.
    for (int ch = 0; ch < numCh; ++ch)
        highBand.copyFrom (ch, 0, buffer, ch, 0, numSmp);

    {
        juce::dsp::AudioBlock<float> block (highBand);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        splitter.process (ctx);
    }

    // Effective threshold.
    const float effThreshDb = autoM ? juce::jlimit (-45.0f, -18.0f, sibilanceDb + 6.0f)
                                    : thresh;

    float maxGr = 0.0f;

    for (int i = 0; i < numSmp; ++i)
    {
        float hf = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            hf = juce::jmax (hf, std::abs (highBand.getReadPointer (ch)[i]));

        const float levelDb = juce::Decibels::gainToDecibels (hf, -90.0f);
        const float coef = (levelDb > envDb) ? attCoef : relCoef;
        envDb = levelDb + (envDb - levelDb) * coef;

        float reductionDb = 0.0f;
        if (envDb > effThreshDb)
            reductionDb = (envDb - effThreshDb) * (1.0f - 1.0f / ratio);

        maxGr = juce::jmax (maxGr, reductionDb);
        const float g = juce::Decibels::decibelsToGain (-reductionDb);

        // Subtract the attenuated portion of the high band from the full signal.
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float hb = highBand.getReadPointer (ch)[i];
            buffer.getWritePointer (ch)[i] -= hb * (1.0f - g);
        }
    }

    grDb = maxGr;
}
