#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/NeonLookAndFeel.h"
#include "UI/MainPanel.h"
#include "UI/AnalyzerPanel.h"
#include "UI/SmartTunePanel.h"

/**
    VocalOneAudioProcessorEditor

    Tabbed shell: Mixer / Analyzer / Smart Tune / AI Assistant, all under the
    neon theme.
*/
class VocalOneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalOneAudioProcessorEditor (VocalOneAudioProcessor&);
    ~VocalOneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class AssistantView;   // defined in the .cpp

    VocalOneAudioProcessor& processor;
    NeonLookAndFeel lnf;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    std::unique_ptr<MainPanel> mainPanel;
    std::unique_ptr<AnalyzerPanel> analyzerPanel;
    std::unique_ptr<SmartTunePanel> smartTunePanel;
    std::unique_ptr<AssistantView> assistantView;

    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalOneAudioProcessorEditor)
};
