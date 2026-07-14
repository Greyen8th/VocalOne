#pragma once

#include <JuceHeader.h>

class VocalOneAudioProcessor;

/**
    StereoScope — Lissajous goniometer + correlation meter for the output.
*/
class StereoScope : public juce::Component, private juce::Timer
{
public:
    explicit StereoScope (VocalOneAudioProcessor& p);
    ~StereoScope() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    VocalOneAudioProcessor& processor;
    std::vector<juce::Point<float>> points;
    float correlation = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoScope)
};
