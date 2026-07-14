#pragma once

#include <JuceHeader.h>

/**
    Chorus — classic modulated-delay chorus (wraps juce::dsp::Chorus) with
    rate, depth, feedback, centre-delay and mix controls.
*/
class Chorus
{
public:
    Chorus() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float rateHz, float depth01,
                        float centreDelayMs, float feedback, float mix01);

private:
    bool on = false;
    juce::dsp::Chorus<float> chorus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};
