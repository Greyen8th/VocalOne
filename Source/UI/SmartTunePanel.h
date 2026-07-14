#pragma once

#include <JuceHeader.h>
#include <memory>

#include "PitchGraph.h"

class VocalOneAudioProcessor;

/**
    SmartTunePanel — pitch correction controls plus the live pitch graph.
*/
class SmartTunePanel : public juce::Component
{
public:
    explicit SmartTunePanel (VocalOneAudioProcessor& p);
    ~SmartTunePanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    VocalOneAudioProcessor& processor;
    std::unique_ptr<PitchGraph> pitchGraph;

    juce::ToggleButton onBtn { "Enable" }, formantBtn { "Formant" };
    juce::ComboBox keyBox, scaleBox;
    juce::Slider speedSlider, strengthSlider;
    juce::Label keyLbl, scaleLbl, speedLbl, strengthLbl;

    std::unique_ptr<ButtonAtt> onAtt, formantAtt;
    std::unique_ptr<ComboAtt>  keyAtt, scaleAtt;
    std::unique_ptr<SliderAtt> speedAtt, strengthAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTunePanel)
};
