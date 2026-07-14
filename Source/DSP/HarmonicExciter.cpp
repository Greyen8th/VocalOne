#include "HarmonicExciter.h"

void HarmonicExciter::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, (juce::uint32) juce::jmax (1, numChannels) };
    hp.prepare (spec);
    hp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    hp.setCutoffFrequency (freq);
    band.setSize (juce::jmax (1, numChannels), maxBlock);
    reset();
}

void HarmonicExciter::reset()
{
    hp.reset();
}

void HarmonicExciter::setParameters (bool enabled, float freqHz, float amount01)
{
    on = enabled;
    amount = juce::jlimit (0.0f, 1.0f, amount01);
    if (! juce::approximatelyEqual (freq, freqHz))
    {
        freq = freqHz;
        hp.setCutoffFrequency (freq);
    }
}

void HarmonicExciter::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || amount <= 0.0f)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    band.setSize (numCh, numSmp, false, false, true);

    for (int ch = 0; ch < numCh; ++ch)
        band.copyFrom (ch, 0, buffer, ch, 0, numSmp);

    {
        juce::dsp::AudioBlock<float> b (band);
        juce::dsp::ProcessContextReplacing<float> ctx (b);
        hp.process (ctx);
    }

    // Generate harmonics via a soft nonlinearity and blend back in.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* out = buffer.getWritePointer (ch);
        const auto* hb = band.getReadPointer (ch);
        for (int i = 0; i < numSmp; ++i)
        {
            const float x = hb[i] * 3.0f;
            const float harm = std::tanh (x) - x * 0.5f;   // extra harmonic content
            out[i] += harm * amount * 0.5f;
        }
    }
}
