#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    Delay — stereo tempo-syncable delay with feedback, ping-pong option and a
    damping filter in the feedback path.
*/
class Delay
{
public:
    Delay() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float timeMs, float feedback01,
                        float mix01, bool pingPong, float dampingHz);

private:
    double sr = 44100.0;
    bool   on = false;
    float  delaySamples = 22050.0f;
    float  feedback = 0.35f;
    float  mix = 0.25f;
    bool   pingPong = false;

    std::vector<std::vector<float>> lines;
    int    writePos = 0;
    int    size = 0;

    juce::dsp::IIR::Filter<float> dampL, dampR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Delay)
};
