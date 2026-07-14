#pragma once

#include <JuceHeader.h>
#include <memory>

#include "PitchGraph.h"

class VocalOneAudioProcessor;

/**
    SmartTunePanel — pitch correction controls plus the live pitch graph.
    Updated with vibrato, humanize, profile controls.
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

    juce::ToggleButton onBtn { "Enable" }, formantBtn { "Formant" }, vibratoBtn { "Vibrato" };
    juce::ComboBox keyBox, scaleBox, profileBox;
    juce::Slider speedSlider, strengthSlider, humanizeSlider;
    juce::Label keyLbl, scaleLbl, speedLbl, strengthLbl, humanizeLbl, profileLbl;

    std::unique_ptr<ButtonAtt> onAtt, formantAtt, vibratoAtt;
    std::unique_ptr<ComboAtt>  keyAtt, scaleAtt, profileAtt;
    std::unique_ptr<SliderAtt> speedAtt, strengthAtt, humanizeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTunePanel)
};
