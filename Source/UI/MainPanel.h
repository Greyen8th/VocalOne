#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

class VocalOneAudioProcessor;

/**
    MainPanel — the primary control surface. Builds a scrollable column of
    module sections, each with an enable toggle and its key parameters, bound to
    the APVTS. Also hosts the preset selector.
*/
class MainPanel : public juce::Component
{
public:
    explicit MainPanel (VocalOneAudioProcessor& p);
    ~MainPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Control
    {
        std::unique_ptr<juce::Component> comp;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<SliderAtt> sAtt;
        std::unique_ptr<ButtonAtt> bAtt;
        std::unique_ptr<ComboAtt> cAtt;
    };

    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
        std::vector<int> controlIndices;
    };

    void addSlider (Section& s, const juce::String& paramID, const juce::String& name);
    void addToggle (Section& s, const juce::String& paramID, const juce::String& name);
    void addCombo  (Section& s, const juce::String& paramID, const juce::String& name,
                    const juce::StringArray& choices);
    Section& newSection (const juce::String& title);
    void buildPresetBar();

    VocalOneAudioProcessor& processor;
    std::vector<Control> controls;
    std::vector<Section> sections;

    juce::ComboBox presetBox;
    juce::TextButton savePresetBtn { "Save" }, autoPresetBtn { "AI Auto" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
