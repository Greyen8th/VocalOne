#include "PitchGraph.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"

PitchGraph::PitchGraph (VocalOneAudioProcessor& p) : processor (p)
{
    startTimerHz (30);
}

PitchGraph::~PitchGraph() { stopTimer(); }

void PitchGraph::timerCallback()
{
    const float in = processor.getAnalyzer().getCurrentF0();
    const float tgt = processor.getSmartTune().getTargetHz();

    inputHz.push_back (in);
    targetHz.push_back (tgt);
    while ((int) inputHz.size()  > maxPoints) inputHz.pop_front();
    while ((int) targetHz.size() > maxPoints) targetHz.pop_front();
    repaint();
}

float PitchGraph::hzToY (float hz, juce::Rectangle<float> area) const
{
    // log scale 80 .. 1000 Hz
    const float lo = std::log2 (80.0f), hi = std::log2 (1000.0f);
    if (hz < 20.0f) return area.getBottom();
    const float t = juce::jlimit (0.0f, 1.0f, (std::log2 (hz) - lo) / (hi - lo));
    return area.getBottom() - t * area.getHeight();
}

void PitchGraph::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (NeonLookAndFeel::panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    auto area = bounds.reduced (8.0f);

    // grid lines at octave references
    g.setColour (NeonLookAndFeel::panel.brighter (0.2f));
    for (float f : { 110.0f, 220.0f, 440.0f, 880.0f })
    {
        const float y = hzToY (f, area);
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
        g.setColour (NeonLookAndFeel::textColour.withAlpha (0.4f));
        g.setFont (9.0f);
        g.drawText (juce::String ((int) f), area.getX(), (int) y - 10, 30, 10, juce::Justification::left);
        g.setColour (NeonLookAndFeel::panel.brighter (0.2f));
    }

    auto drawSeries = [&] (const std::deque<float>& s, juce::Colour c)
    {
        if (s.size() < 2) return;
        juce::Path path;
        bool started = false;
        for (size_t i = 0; i < s.size(); ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) (maxPoints - 1);
            if (s[i] < 20.0f) { started = false; continue; }
            const float y = hzToY (s[i], area);
            if (! started) { path.startNewSubPath (x, y); started = true; }
            else           path.lineTo (x, y);
        }
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (1.6f));
    };

    drawSeries (targetHz, NeonLookAndFeel::neonGreen.withAlpha (0.7f));
    drawSeries (inputHz,  NeonLookAndFeel::neonCyan);
}
