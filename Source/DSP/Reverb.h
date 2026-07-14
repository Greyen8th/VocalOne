#pragma once

#include <JuceHeader.h>

/**
    Reverb — wrapper around juce::dsp::Reverb with a pre-delay line and a wet
    high-pass so the tail stays clean on vocals.
*/
class VocalReverb
{
public:
    VocalReverb() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float size01, float damping01,
                        float width01, float mix01, float preDelayMs);

private:
    double sr = 44100.0;
    bool   on = false;
    float  mix = 0.2f;

    juce::Reverb reverb;
    juce::Reverb::Parameters params;

    juce::AudioBuffer<float> preDelayBuf;
    int preDelayWrite = 0;
    int preDelaySamples = 0;

    juce::dsp::IIR::Filter<float> hpL, hpR;
    juce::AudioBuffer<float> wet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalReverb)
};
