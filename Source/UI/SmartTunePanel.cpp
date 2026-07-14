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
    addAndMakeVisible (vibratoBtn);
    vibratoAtt = std::make_unique<ButtonAtt> (apvts, "tune_vibrato", vibratoBtn);

    keyBox.addItemList ({ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 1);
    addAndMakeVisible (keyBox);
    keyAtt = std::make_unique<ComboAtt> (apvts, "tune_key", keyBox);

    scaleBox.addItemList ({ "Chromatic","Major","Minor","Major Pentatonic","Minor Pentatonic",
                             "Blues","Dorian","Phrygian","Lydian","Mixolydian",
                             "Harmonic Minor","Melodic Minor","Diminished","Whole Tone",
                             "Arabic","Japanese" }, 1);
    addAndMakeVisible (scaleBox);
    scaleAtt = std::make_unique<ComboAtt> (apvts, "tune_scale", scaleBox);

    profileBox.addItemList ({ "Tight","Natural","Loose" }, 1);
    addAndMakeVisible (profileBox);
    profileAtt = std::make_unique<ComboAtt> (apvts, "tune_profile", profileBox);

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
    setupSlider (humanizeSlider, humanizeLbl, "Humanize");
    speedAtt    = std::make_unique<SliderAtt> (apvts, "tune_speed", speedSlider);
    strengthAtt = std::make_unique<SliderAtt> (apvts, "tune_strength", strengthSlider);
    humanizeAtt = std::make_unique<SliderAtt> (apvts, "tune_humanize", humanizeSlider);

    keyLbl.setText ("Key", juce::dontSendNotification);
    scaleLbl.setText ("Scale", juce::dontSendNotification);
    profileLbl.setText ("Profile", juce::dontSendNotification);
    keyLbl.setJustificationType (juce::Justification::centredLeft);
    scaleLbl.setJustificationType (juce::Justification::centredLeft);
    profileLbl.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (keyLbl);
    addAndMakeVisible (scaleLbl);
    addAndMakeVisible (profileLbl);
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

    auto controls = area.removeFromTop (170);

    auto left = controls.removeFromLeft (220);
    onBtn.setBounds (left.removeFromTop (26));
    formantBtn.setBounds (left.removeFromTop (26));
    vibratoBtn.setBounds (left.removeFromTop (26));
    left.removeFromTop (6);

    auto keyRow = left.removeFromTop (26);
    keyLbl.setBounds (keyRow.removeFromLeft (50));
    keyBox.setBounds (keyRow);
    auto scaleRow = left.removeFromTop (26);
    scaleLbl.setBounds (scaleRow.removeFromLeft (50));
    scaleBox.setBounds (scaleRow);
    auto profileRow = left.removeFromTop (26);
    profileLbl.setBounds (profileRow.removeFromLeft (50));
    profileBox.setBounds (profileRow);

    auto knobs = controls.removeFromLeft (300);
    auto k1 = knobs.removeFromLeft (100);
    speedLbl.setBounds (k1.removeFromTop (16));
    speedSlider.setBounds (k1);
    auto k2 = knobs.removeFromLeft (100);
    strengthLbl.setBounds (k2.removeFromTop (16));
    strengthSlider.setBounds (k2);
    auto k3 = knobs;
    humanizeLbl.setBounds (k3.removeFromTop (16));
    humanizeSlider.setBounds (k3);

    area.removeFromTop (8);
    pitchGraph->setBounds (area);
}
