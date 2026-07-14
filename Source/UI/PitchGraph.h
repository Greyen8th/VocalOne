#pragma once

#include <JuceHeader.h>
#include <deque>

class VocalOneAudioProcessor;

/**
    PitchGraph — scrolling plot of detected pitch (input) and the Smart Tune
    target, so the user can see the correction in action.
*/
class PitchGraph : public juce::Component, private juce::Timer
{
public:
    explicit PitchGraph (VocalOneAudioProcessor& p);
    ~PitchGraph() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float hzToY (float hz, juce::Rectangle<float> area) const;

    VocalOneAudioProcessor& processor;
    std::deque<float> inputHz, targetHz;
    static constexpr int maxPoints = 200;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchGraph)
};
