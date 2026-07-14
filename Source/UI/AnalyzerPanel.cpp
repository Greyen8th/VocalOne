#include "AnalyzerPanel.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"
#include "../DSP/AIAnalyzer.h"

AnalyzerPanel::AnalyzerPanel (VocalOneAudioProcessor& p) : processor (p)
{
    waveform = std::make_unique<WaveformDisplay> (p);
    scope    = std::make_unique<StereoScope> (p);
    meter    = std::make_unique<LUFSMeter> (p);
    addAndMakeVisible (*waveform);
    addAndMakeVisible (*scope);
    addAndMakeVisible (*meter);
    startTimerHz (10);
}

AnalyzerPanel::~AnalyzerPanel() { stopTimer(); }

void AnalyzerPanel::timerCallback()
{
    auto s = processor.getAnalyzer().getSnapshot();
    juce::String t;
    t << "KEY  " << AIAnalyzer::keyName (s.keyIndex, s.isMinor) << "\n"
      << "BPM  " << juce::String (s.bpm, 0) << "\n"
      << "GENRE  " << AIAnalyzer::genreName (s.genreGuess) << "\n"
      << "GENDER  " << AIAnalyzer::genderName (s.genderGuess) << "\n"
      << "F0  " << juce::String (s.f0, 1) << " Hz\n"
      << "RANGE  " << juce::String (s.f0Min, 0) << "-" << juce::String (s.f0Max, 0) << " Hz\n"
      << "RMS  " << juce::String (s.rmsDb, 1) << " dB\n"
      << "PEAK  " << juce::String (s.peakDb, 1) << " dB\n"
      << "CREST  " << juce::String (s.crestDb, 1) << " dB\n"
      << "NOISE  " << juce::String (s.noiseFloorDb, 1) << " dB\n"
      << "SIBIL.  " << juce::String (s.sibilanceDb, 1) << " dB\n"
      << "BREATH  " << juce::String (s.breathiness, 2) << "\n"
      << "MIC DIST  " << juce::String (s.micDistance, 2);
    infoText = t;
    repaint();
}

void AnalyzerPanel::paint (juce::Graphics& g)
{
    g.fillAll (NeonLookAndFeel::background);

    auto infoArea = getLocalBounds().reduced (8).removeFromLeft (170);
    g.setColour (NeonLookAndFeel::panel);
    g.fillRoundedRectangle (infoArea.toFloat(), 8.0f);
    g.setColour (NeonLookAndFeel::neonMagenta.withAlpha (0.35f));
    g.drawRoundedRectangle (infoArea.toFloat(), 8.0f, 1.0f);

    g.setColour (NeonLookAndFeel::neonCyan);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText ("AI ANALYZER", infoArea.removeFromTop (24).reduced (10, 4),
                juce::Justification::centredLeft);
    g.setColour (NeonLookAndFeel::textColour);
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    g.drawMultiLineText (infoText, infoArea.getX() + 10, infoArea.getY() + 6,
                         infoArea.getWidth() - 16, juce::Justification::left, 1.3f);
}

void AnalyzerPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromLeft (170 + 8);   // info column

    meter->setBounds (area.removeFromRight (90));
    area.removeFromRight (8);

    waveform->setBounds (area.removeFromTop (area.getHeight() / 2).reduced (0, 0));
    area.removeFromTop (8);
    scope->setBounds (area);
}
