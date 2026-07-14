#pragma once

#include <JuceHeader.h>
#include <memory>

#include "WaveformDisplay.h"
#include "StereoScope.h"
#include "LUFSMeter.h"

class VocalOneAudioProcessor;

/**
    AnalyzerPanel — shows the live analysis read-out (key, BPM, genre, gender,
    levels, sibilance, etc.) alongside the waveform, goniometer and LUFS meter.
*/
class AnalyzerPanel : public juce::Component, private juce::Timer
{
public:
    explicit AnalyzerPanel (VocalOneAudioProcessor& p);
    ~AnalyzerPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VocalOneAudioProcessor& processor;
    std::unique_ptr<WaveformDisplay> waveform;
    std::unique_ptr<StereoScope> scope;
    std::unique_ptr<LUFSMeter> meter;

    juce::String infoText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyzerPanel)
};
