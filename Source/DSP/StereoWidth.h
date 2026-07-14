#pragma once

#include <JuceHeader.h>

/**
    StereoWidth — mid/side width control with mono-bass protection and an
    overall pan/balance. Keeps low frequencies centred to preserve punch.
*/
class StereoWidth
{
public:
    StereoWidth() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float width /*0..2*/, float bassMonoHz);

private:
    double sr = 44100.0;
    bool   on = true;
    float  width = 1.0f;
    float  bassMonoHz = 150.0f;

    juce::dsp::LinkwitzRileyFilter<float> lpForBass;   // extract low freqs of side
    juce::AudioBuffer<float> sideLow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoWidth)
};
