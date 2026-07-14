#include "SmartTunePanel.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"

SmartTunePanel::SmartTunePanel (VocalOneAudioProcessor& p) : processor (p)
{
    auto& apvts = processor.getAPVTS();

    pitchGraph = std::make_unique<PitchGraph> (p);
    addAndMakeVisible (*pitchGraph);

    addAndMakeVisible (onBtn);
    onAtt = std::make_unique<ButtonAtt> (apvts, "tune_on", onBtn);
    addAndMakeVisible (formantBtn);
    formantAtt = std::make_unique<ButtonAtt> (apvts, "tune_formant", formantBtn);

    keyBox.addItemList ({ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 1);
    addAndMakeVisible (keyBox);
    keyAtt = std::make_unique<ComboAtt> (apvts, "tune_key", keyBox);

    scaleBox.addItemList ({ "Chromatic","Major","Minor","Major Pentatonic","Minor Pentatonic" }, 1);
    addAndMakeVisible (scaleBox);
    scaleAtt = std::make_unique<ComboAtt> (apvts, "tune_scale", scaleBox);

    auto setupSlider = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };
    setupSlider (speedSlider, speedLbl, "Retune Speed");
    setupSlider (strengthSlider, strengthLbl, "Strength");
    speedAtt    = std::make_unique<SliderAtt> (apvts, "tune_speed", speedSlider);
    strengthAtt = std::make_unique<SliderAtt> (apvts, "tune_strength", strengthSlider);

    keyLbl.setText ("Key", juce::dontSendNotification);
    scaleLbl.setText ("Scale", juce::dontSendNotification);
    keyLbl.setJustificationType (juce::Justification::centredLeft);
    scaleLbl.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (keyLbl);
    addAndMakeVisible (scaleLbl);
}

SmartTunePanel::~SmartTunePanel() = default;

void SmartTunePanel::paint (juce::Graphics& g)
{
    g.fillAll (NeonLookAndFeel::background);
    g.setColour (NeonLookAndFeel::neonCyan);
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("SMART TUNE", getLocalBounds().removeFromTop (26).reduced (12, 4),
                juce::Justification::centredLeft);
}

void SmartTunePanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (24);

    auto controls = area.removeFromTop (150);

    auto left = controls.removeFromLeft (220);
    onBtn.setBounds (left.removeFromTop (26));
    formantBtn.setBounds (left.removeFromTop (26));
    left.removeFromTop (6);

    auto keyRow = left.removeFromTop (26);
    keyLbl.setBounds (keyRow.removeFromLeft (50));
    keyBox.setBounds (keyRow);
    auto scaleRow = left.removeFromTop (26);
    scaleLbl.setBounds (scaleRow.removeFromLeft (50));
    scaleBox.setBounds (scaleRow);

    auto knobs = controls.removeFromLeft (200);
    auto k1 = knobs.removeFromLeft (100);
    speedLbl.setBounds (k1.removeFromTop (16));
    speedSlider.setBounds (k1);
    auto k2 = knobs;
    strengthLbl.setBounds (k2.removeFromTop (16));
    strengthSlider.setBounds (k2);

    area.removeFromTop (8);
    pitchGraph->setBounds (area);
}
