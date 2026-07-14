#include "StereoScope.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"

StereoScope::StereoScope (VocalOneAudioProcessor& p) : processor (p)
{
    startTimerHz (30);
}

StereoScope::~StereoScope() { stopTimer(); }

void StereoScope::timerCallback()
{
    const auto& sb = processor.getScopeBuffer();
    const int w = sb.writePos.load();
    const int n = VocalOneAudioProcessor::scopeSize;

    points.clear();
    points.reserve ((size_t) n);

    float sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
    for (int i = 0; i < n; i += 2)
    {
        const int idx = (w + i) % n;
        const float L = sb.left[(size_t) idx];
        const float R = sb.right[(size_t) idx];
        // rotate 45deg: mid/side axes
        points.push_back ({ (L - R) * 0.7071f, (L + R) * 0.7071f });
        sumLR += L * R; sumL2 += L * L; sumR2 += R * R;
    }
    const float denom = std::sqrt (sumL2 * sumR2) + 1.0e-9f;
    correlation = juce::jlimit (-1.0f, 1.0f, sumLR / denom);
    repaint();
}

void StereoScope::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (NeonLookAndFeel::panel);
    g.fillRoundedRectangle (bounds, 6.0f);

    auto area = bounds.reduced (8.0f);
    auto scopeArea = area.withTrimmedBottom (18.0f);
    const auto centre = scopeArea.getCentre();
    const float scale = juce::jmin (scopeArea.getWidth(), scopeArea.getHeight()) * 0.45f;

    // axes
    g.setColour (NeonLookAndFeel::panel.brighter (0.25f));
    g.drawLine (centre.x, scopeArea.getY(), centre.x, scopeArea.getBottom());
    g.drawLine (scopeArea.getX(), centre.y, scopeArea.getRight(), centre.y);

    g.setColour (NeonLookAndFeel::neonCyan.withAlpha (0.6f));
    for (auto& pt : points)
    {
        const float x = centre.x + pt.x * scale;
        const float y = centre.y - pt.y * scale;
        g.fillEllipse (x - 0.8f, y - 0.8f, 1.6f, 1.6f);
    }

    // correlation bar
    auto corrArea = area.removeFromBottom (14.0f);
    g.setColour (NeonLookAndFeel::panel.brighter (0.2f));
    g.fillRoundedRectangle (corrArea, 3.0f);
    const float t = (correlation + 1.0f) * 0.5f;
    const float cx = corrArea.getX() + corrArea.getWidth() * t;
    g.setColour (correlation < 0.0f ? NeonLookAndFeel::neonMagenta : NeonLookAndFeel::neonGreen);
    g.fillRoundedRectangle (corrArea.getCentreX(), corrArea.getY(),
                            cx - corrArea.getCentreX(), corrArea.getHeight(), 2.0f);
    g.setColour (NeonLookAndFeel::textColour);
    g.setFont (10.0f);
    g.drawText ("corr " + juce::String (correlation, 2), corrArea, juce::Justification::centred);
}
