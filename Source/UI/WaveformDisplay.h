#pragma once

#include <JuceHeader.h>

class VocalOneAudioProcessor;

/**
    WaveformDisplay — scrolling oscilloscope of the processed output.
*/
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    explicit WaveformDisplay (VocalOneAudioProcessor& p);
    ~WaveformDisplay() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    VocalOneAudioProcessor& processor;
    std::vector<float> snapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
