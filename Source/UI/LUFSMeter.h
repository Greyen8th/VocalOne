#pragma once

#include <JuceHeader.h>

class VocalOneAudioProcessor;

/**
    LUFSMeter — vertical loudness + gain-reduction meter driven by the limiter.
*/
class LUFSMeter : public juce::Component, private juce::Timer
{
public:
    explicit LUFSMeter (VocalOneAudioProcessor& p);
    ~LUFSMeter() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    VocalOneAudioProcessor& processor;
    float lufs = -60.0f;
    float grDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LUFSMeter)
};
