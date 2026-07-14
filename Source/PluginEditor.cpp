#include "PluginEditor.h"
#include "AI/AIAssistant.h"

//==============================================================================
// AI Assistant chat view (kept in the .cpp to keep the header lean).
//==============================================================================
class VocalOneAudioProcessorEditor::AssistantView : public juce::Component
{
public:
    explicit AssistantView (VocalOneAudioProcessor& p) : processor (p)
    {
        transcript.setMultiLine (true);
        transcript.setReadOnly (true);
        transcript.setScrollbarsShown (true);
        transcript.setCaretVisible (false);
        transcript.setColour (juce::TextEditor::backgroundColourId, NeonLookAndFeel::panel);
        addAndMakeVisible (transcript);

        input.setMultiLine (false);
        input.setReturnKeyStartsNewLine (false);
        input.setTextToShowWhenEmpty ("Ask the VocalOne assistant...", NeonLookAndFeel::textColour.withAlpha (0.5f));
        addAndMakeVisible (input);
        input.onReturnKey = [this] { send(); };

        sendBtn.setButtonText ("Send");
        sendBtn.onClick = [this] { send(); };
        addAndMakeVisible (sendBtn);

        clearBtn.setButtonText ("Clear");
        clearBtn.onClick = [this]
        {
            processor.getAssistant().clearHistory();
            transcript.clear();
        };
        addAndMakeVisible (clearBtn);

        auto& assistant = processor.getAssistant();
        assistant.onResponse = [this] (juce::String reply)
        {
            append ("Assistant", reply);
        };
        assistant.onError = [this] (juce::String err)
        {
            append ("Error", err);
        };

        if (processor.getAssistant().getConfig().apiKey.isEmpty())
            append ("System",
                    "No API key found. Set ABACUS_API_KEY or OPENAI_API_KEY in the "
                    "host environment to enable the chat assistant. The analyzer and "
                    "all DSP still work without it.");
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        auto bottom = area.removeFromBottom (32);
        clearBtn.setBounds (bottom.removeFromRight (70));
        bottom.removeFromRight (6);
        sendBtn.setBounds (bottom.removeFromRight (70));
        bottom.removeFromRight (6);
        input.setBounds (bottom);
        area.removeFromBottom (8);
        transcript.setBounds (area);
    }

    void paint (juce::Graphics& g) override { g.fillAll (NeonLookAndFeel::background); }

private:
    void append (const juce::String& who, const juce::String& text)
    {
        transcript.moveCaretToEnd();
        transcript.insertTextAtCaret (who + ":  " + text + "\n\n");
    }

    void send()
    {
        const auto text = input.getText().trim();
        if (text.isEmpty()) return;
        append ("You", text);
        processor.getAssistant().sendMessage (text);
        input.clear();
    }

    VocalOneAudioProcessor& processor;
    juce::TextEditor transcript, input;
    juce::TextButton sendBtn, clearBtn;
};

//==============================================================================
VocalOneAudioProcessorEditor::VocalOneAudioProcessorEditor (VocalOneAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);

    mainPanel      = std::make_unique<MainPanel> (p);
    analyzerPanel  = std::make_unique<AnalyzerPanel> (p);
    smartTunePanel = std::make_unique<SmartTunePanel> (p);
    assistantView  = std::make_unique<AssistantView> (p);

    tabs.setColour (juce::TabbedComponent::backgroundColourId, NeonLookAndFeel::background);
    tabs.setColour (juce::TabbedComponent::outlineColourId, NeonLookAndFeel::neonCyan.withAlpha (0.3f));
    tabs.addTab ("MIXER",       NeonLookAndFeel::background, mainPanel.get(),      false);
    tabs.addTab ("ANALYZER",    NeonLookAndFeel::background, analyzerPanel.get(),  false);
    tabs.addTab ("SMART TUNE",  NeonLookAndFeel::background, smartTunePanel.get(), false);
    tabs.addTab ("AI ASSISTANT",NeonLookAndFeel::background, assistantView.get(),  false);
    addAndMakeVisible (tabs);

    titleLabel.setText ("VocalOne", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, NeonLookAndFeel::neonMagenta);
    addAndMakeVisible (titleLabel);

    setResizable (true, true);
    setResizeLimits (900, 560, 1800, 1200);
    setSize (1080, 680);
}

VocalOneAudioProcessorEditor::~VocalOneAudioProcessorEditor()
{
    processor.getAssistant().onResponse = nullptr;
    processor.getAssistant().onError = nullptr;
    setLookAndFeel (nullptr);
}

void VocalOneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (NeonLookAndFeel::background);
    auto header = getLocalBounds().removeFromTop (40);
    juce::ColourGradient grad (NeonLookAndFeel::neonMagenta.withAlpha (0.15f), header.getTopLeft().toFloat(),
                               NeonLookAndFeel::neonCyan.withAlpha (0.15f), header.getTopRight().toFloat(), false);
    g.setGradientFill (grad);
    g.fillRect (header);
}

void VocalOneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (40);
    titleLabel.setBounds (header.reduced (14, 4));
    tabs.setBounds (area);
}
