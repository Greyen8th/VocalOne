#pragma once

#include <JuceHeader.h>

/**
    HarmonicExciter — generates high-frequency harmonics from a high-passed copy
    of the signal (Aphex-style). Adds sparkle and presence without a broad EQ
    boost.
*/
class HarmonicExciter
{
public:
    HarmonicExciter() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float freqHz, float amount01);

private:
    double sr = 44100.0;
    bool   on = true;
    float  amount = 0.3f;
    float  freq = 3000.0f;

    juce::dsp::LinkwitzRileyFilter<float> hp;
    juce::AudioBuffer<float> band;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicExciter)
};
