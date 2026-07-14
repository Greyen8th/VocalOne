#include "Reverb.h"

void VocalReverb::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    reverb.setSampleRate (sampleRate);

    preDelayBuf.setSize (juce::jmax (2, numChannels), (int) (sr * 0.25) + 4);
    preDelayBuf.clear();
    preDelayWrite = 0;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    hpL.prepare (spec); hpR.prepare (spec);
    *hpL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 200.0f);
    *hpR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 200.0f);

    wet.setSize (juce::jmax (2, numChannels), maxBlock);
    reset();
}

void VocalReverb::reset()
{
    reverb.reset();
    preDelayBuf.clear();
    preDelayWrite = 0;
    hpL.reset(); hpR.reset();
}

void VocalReverb::setParameters (bool enabled, float size01, float damping01,
                            float width01, float mix01, float preDelayMs)
{
    on = enabled;
    mix = juce::jlimit (0.0f, 1.0f, mix01);
    params.roomSize   = juce::jlimit (0.0f, 1.0f, size01);
    params.damping    = juce::jlimit (0.0f, 1.0f, damping01);
    params.width      = juce::jlimit (0.0f, 1.0f, width01);
    params.wetLevel   = 1.0f;   // we do our own dry/wet
    params.dryLevel   = 0.0f;
    params.freezeMode = 0.0f;
    reverb.setParameters (params);
    preDelaySamples = juce::jlimit (0, preDelayBuf.getNumSamples() - 1,
                                    (int) (preDelayMs * 0.001f * (float) sr));
}

void VocalReverb::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || mix <= 0.0f)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    wet.setSize (numCh, numSmp, false, false, true);

    // Pre-delay copy into wet.
    for (int i = 0; i < numSmp; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* pd = preDelayBuf.getWritePointer (ch);
            pd[preDelayWrite] = buffer.getReadPointer (ch)[i];
            int rp = preDelayWrite - preDelaySamples;
            if (rp < 0) rp += preDelayBuf.getNumSamples();
            wet.getWritePointer (ch)[i] = pd[rp];
        }
        if (++preDelayWrite >= preDelayBuf.getNumSamples())
            preDelayWrite = 0;
    }

    // Run reverb on the wet buffer.
    if (numCh >= 2)
        reverb.processStereo (wet.getWritePointer (0), wet.getWritePointer (1), numSmp);
    else
        reverb.processMono (wet.getWritePointer (0), numSmp);

    // HP the tail + mix.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* w = wet.getWritePointer (ch);
        auto& hp = (ch == 0) ? hpL : hpR;
        auto* out = buffer.getWritePointer (ch);
        for (int i = 0; i < numSmp; ++i)
        {
            const float wv = hp.processSample (w[i]);
            out[i] = out[i] * (1.0f - mix) + wv * mix;
        }
    }
}
