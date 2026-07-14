#pragma once

#include <JuceHeader.h>

/**
    NeonLookAndFeel — dark, neon-accented theme used across VocalOne.
*/
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    // Colour palette.
    static const juce::Colour background;
    static const juce::Colour panel;
    static const juce::Colour neonCyan;
    static const juce::Colour neonMagenta;
    static const juce::Colour neonGreen;
    static const juce::Colour textColour;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
};
