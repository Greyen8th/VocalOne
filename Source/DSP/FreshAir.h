#pragma once

#include <JuceHeader.h>

/**
    FreshAir — dual high-shelf "air" enhancer (mid-air ~ 8 kHz, high-air ~ 16 kHz)
    with a subtle dynamic component so it lifts detail without harshness.
*/
class FreshAir
{
public:
    FreshAir() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float midAir01, float highAir01);

private:
    void updateCoeffs();

    double sr = 44100.0;
    bool   on = true;
    float  midAir = 0.3f;
    float  highAir = 0.3f;

    juce::dsp::IIR::Filter<float> midL, midR, highL, highR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreshAir)
};
