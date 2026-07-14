#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    Doubler — creates artificial double-tracking by generating a few detuned,
    time-shifted voices panned across the stereo field.
*/
class Doubler
{
public:
    Doubler() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, int voices, float detuneCents,
                        float delayMs, float spread01, float mix01);

private:
    static constexpr int maxVoices = 4;

    double sr = 44100.0;
    bool   on = false;
    int    voices = 2;
    float  detune = 12.0f;
    float  baseDelay = 20.0f;
    float  spread = 1.0f;
    float  mix = 0.5f;

    std::vector<float> line;   // mono source ring
    int    size = 0, writePos = 0;

    struct Voice { float phase = 0.0f; float rate = 0.0f; float delaySamp = 0.0f; float panL = 1.0f, panR = 1.0f; };
    std::array<Voice, maxVoices> vs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Doubler)
};
