#include "LUFSMeter.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"

LUFSMeter::LUFSMeter (VocalOneAudioProcessor& p) : processor (p)
{
    startTimerHz (30);
}

LUFSMeter::~LUFSMeter() { stopTimer(); }

void LUFSMeter::timerCallback()
{
    lufs = processor.getLimiter().getOutputLufs();
    grDb = processor.getLimiter().getGainReductionDb();
    repaint();
}

void LUFSMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (NeonLookAndFeel::panel);
    g.fillRoundedRectangle (bounds, 6.0f);

    auto area = bounds.reduced (8.0f);
    const float labelH = 16.0f;
    auto meterArea = area.withTrimmedBottom (labelH);

    // Split into loudness bar (left) and gain reduction (right).
    auto lufsBar = meterArea.removeFromLeft (meterArea.getWidth() * 0.5f).reduced (3.0f);
    auto grBar   = meterArea.reduced (3.0f);

    auto mapDb = [] (float dB, float lo, float hi) { return juce::jlimit (0.0f, 1.0f, (dB - lo) / (hi - lo)); };

    // Loudness (-40 .. 0 LUFS)
    const float lNorm = mapDb (lufs, -40.0f, 0.0f);
    g.setColour (NeonLookAndFeel::panel.brighter (0.2f));
    g.fillRoundedRectangle (lufsBar, 3.0f);
    juce::ColourGradient grad (NeonLookAndFeel::neonGreen, lufsBar.getBottomLeft(),
                               NeonLookAndFeel::neonMagenta, lufsBar.getTopLeft(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (lufsBar.withTop (lufsBar.getBottom() - lufsBar.getHeight() * lNorm), 3.0f);

    // Gain reduction (0 .. 12 dB downward)
    const float grNorm = mapDb (grDb, -12.0f, 0.0f);
    g.setColour (NeonLookAndFeel::panel.brighter (0.2f));
    g.fillRoundedRectangle (grBar, 3.0f);
    g.setColour (NeonLookAndFeel::neonMagenta.withAlpha (0.85f));
    g.fillRoundedRectangle (grBar.withBottom (grBar.getY() + grBar.getHeight() * (1.0f - grNorm)), 3.0f);

    g.setColour (NeonLookAndFeel::textColour);
    g.setFont (11.0f);
    g.drawText (juce::String (lufs, 1) + " LUFS", area.removeFromLeft (area.getWidth() * 0.5f).removeFromBottom (labelH),
                juce::Justification::centred);
    g.drawText (juce::String (grDb, 1) + " GR", area.removeFromBottom (labelH),
                juce::Justification::centred);
}
