#include "NeonLookAndFeel.h"

const juce::Colour NeonLookAndFeel::background   { 0xff0d1117 };
const juce::Colour NeonLookAndFeel::panel        { 0xff161b26 };
const juce::Colour NeonLookAndFeel::neonCyan     { 0xff00e5ff };
const juce::Colour NeonLookAndFeel::neonMagenta  { 0xffff2bd6 };
const juce::Colour NeonLookAndFeel::neonGreen    { 0xff39ff88 };
const juce::Colour NeonLookAndFeel::textColour   { 0xffe6edf3 };

NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Slider::textBoxTextColourId, textColour);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, textColour);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, textColour);
    setColour (juce::ComboBox::outlineColourId, neonCyan.withAlpha (0.4f));
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, textColour);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neonCyan.withAlpha (0.3f));
    setColour (juce::TextEditor::backgroundColourId, panel);
    setColour (juce::TextEditor::textColourId, textColour);
    setColour (juce::TextEditor::outlineColourId, neonCyan.withAlpha (0.3f));
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (panel.brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (3.0f));

    // value arc (neon)
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    const auto accent = slider.isEnabled() ? neonCyan : neonCyan.withAlpha (0.3f);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // knob
    g.setColour (panel.brighter (0.4f));
    g.fillEllipse (juce::Rectangle<float> (radius, radius).withCentre (centre).reduced (radius * 0.35f));

    // pointer
    juce::Path pointer;
    const float pl = radius * 0.55f;
    pointer.addRoundedRectangle (-1.5f, -pl, 3.0f, pl * 0.6f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (accent);
    g.fillPath (pointer);
}

void NeonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool vertical = style == juce::Slider::LinearVertical;
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

    g.setColour (panel.brighter (0.15f));
    if (vertical)
        g.fillRoundedRectangle (bounds.getCentreX() - 2.0f, bounds.getY(), 4.0f, bounds.getHeight(), 2.0f);
    else
        g.fillRoundedRectangle (bounds.getX(), bounds.getCentreY() - 2.0f, bounds.getWidth(), 4.0f, 2.0f);

    g.setColour (slider.isEnabled() ? neonCyan : neonCyan.withAlpha (0.3f));
    if (vertical)
        g.fillRoundedRectangle (bounds.getCentreX() - 2.0f, sliderPos, 4.0f, bounds.getBottom() - sliderPos, 2.0f);
    else
        g.fillRoundedRectangle (bounds.getX(), bounds.getCentreY() - 2.0f, sliderPos - bounds.getX(), 4.0f, 2.0f);

    g.setColour (textColour);
    if (vertical)
        g.fillEllipse (bounds.getCentreX() - 6.0f, sliderPos - 6.0f, 12.0f, 12.0f);
    else
        g.fillEllipse (sliderPos - 6.0f, bounds.getCentreY() - 6.0f, 12.0f, 12.0f);
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                            const juce::Colour&, bool highlighted, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
    auto base = panel.brighter (down ? 0.05f : (highlighted ? 0.3f : 0.15f));
    g.setColour (base);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (neonCyan.withAlpha (highlighted ? 0.8f : 0.4f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.2f);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                        bool highlighted, bool)
{
    auto bounds = b.getLocalBounds().toFloat();
    const float sz = juce::jmin (18.0f, bounds.getHeight());
    juce::Rectangle<float> box (bounds.getX(), bounds.getCentreY() - sz * 0.5f, sz, sz);

    g.setColour (panel.brighter (0.2f));
    g.fillRoundedRectangle (box, 3.0f);

    if (b.getToggleState())
    {
        g.setColour (neonGreen);
        g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
    }
    g.setColour (neonCyan.withAlpha (highlighted ? 0.9f : 0.5f));
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    g.setColour (textColour);
    g.setFont (13.0f);
    g.drawText (b.getButtonText(), bounds.withTrimmedLeft (sz + 6.0f),
                juce::Justification::centredLeft, true);
}

juce::Font NeonLookAndFeel::getLabelFont (juce::Label& l)
{
    return juce::Font (juce::jmin (15.0f, (float) l.getHeight() * 0.7f));
}
