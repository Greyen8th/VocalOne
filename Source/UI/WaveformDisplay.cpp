#include "WaveformDisplay.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"

WaveformDisplay::WaveformDisplay (VocalOneAudioProcessor& p) : processor (p)
{
    snapshot.assign (VocalOneAudioProcessor::scopeSize, 0.0f);
    startTimerHz (30);
}

WaveformDisplay::~WaveformDisplay() { stopTimer(); }

void WaveformDisplay::timerCallback()
{
    const auto& sb = processor.getScopeBuffer();
    const int w = sb.writePos.load();
    const int n = VocalOneAudioProcessor::scopeSize;
    for (int i = 0; i < n; ++i)
    {
        const int idx = (w + i) % n;
        snapshot[(size_t) i] = 0.5f * (sb.left[(size_t) idx] + sb.right[(size_t) idx]);
    }
    repaint();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (NeonLookAndFeel::panel);
    g.fillRoundedRectangle (bounds, 6.0f);

    auto area = bounds.reduced (6.0f);
    g.setColour (NeonLookAndFeel::panel.brighter (0.25f));
    g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

    const int n = (int) snapshot.size();
    if (n < 2) return;

    juce::Path path;
    const float mid = area.getCentreY();
    const float amp = area.getHeight() * 0.45f;
    for (int i = 0; i < n; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / (float) (n - 1);
        const float y = mid - juce::jlimit (-1.0f, 1.0f, snapshot[(size_t) i]) * amp;
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
    }

    g.setColour (NeonLookAndFeel::neonCyan);
    g.strokePath (path, juce::PathStrokeType (1.4f));
}
