#include "MainPanel.h"
#include "NeonLookAndFeel.h"
#include "../PluginProcessor.h"
#include "../AI/SmartPreset.h"

MainPanel::MainPanel (VocalOneAudioProcessor& p) : processor (p)
{
    buildPresetBar();

    auto& gate = newSection ("GATE");
    addToggle (gate, "gate_on", "On");
    addToggle (gate, "gate_auto", "Auto");
    addSlider (gate, "gate_thresh", "Thresh");
    addSlider (gate, "gate_release", "Release");

    auto& noise = newSection ("NOISE REMOVAL");
    addToggle (noise, "noise_on", "On");
    addSlider (noise, "noise_amount", "Amount");

    auto& deess = newSection ("DE-ESSER");
    addToggle (deess, "deess_on", "On");
    addToggle (deess, "deess_auto", "Auto");
    addSlider (deess, "deess_freq", "Freq");
    addSlider (deess, "deess_ratio", "Ratio");

    auto& eq = newSection ("AI EQ");
    addToggle (eq, "eq_on", "On");
    addToggle (eq, "eq_auto", "Auto");
    addSlider (eq, "eq_amount", "Amount");

    auto& comp = newSection ("COMPRESSOR");
    addToggle (comp, "comp_on", "On");
    addCombo  (comp, "comp_mode", "Mode",
               { "Auto","Rap","Pop","Rock","Podcast","Voice Over","Manual" });
    addSlider (comp, "comp_thresh", "Thresh");
    addSlider (comp, "comp_ratio", "Ratio");
    addSlider (comp, "comp_makeup", "Makeup");

    auto& sat = newSection ("SATURATION");
    addToggle (sat, "sat_on", "On");
    addCombo  (sat, "sat_model", "Model", { "Tube","Tape","Vintage","Warm","Digital" });
    addSlider (sat, "sat_drive", "Drive");
    addSlider (sat, "sat_mix", "Mix");

    auto& exc = newSection ("EXCITER / AIR");
    addToggle (exc, "exc_on", "Exciter");
    addSlider (exc, "exc_amount", "Amount");
    addToggle (exc, "air_on", "Air");
    addSlider (exc, "air_mid", "Air Mid");
    addSlider (exc, "air_high", "Air Hi");

    auto& delay = newSection ("DELAY");
    addToggle (delay, "dly_on", "On");
    addSlider (delay, "dly_time", "Time");
    addSlider (delay, "dly_fb", "FB");
    addSlider (delay, "dly_mix", "Mix");

    auto& rev = newSection ("REVERB");
    addToggle (rev, "rev_on", "On");
    addSlider (rev, "rev_size", "Size");
    addSlider (rev, "rev_mix", "Mix");
    addSlider (rev, "rev_predelay", "PreDly");

    auto& dbl = newSection ("DOUBLER / CHORUS");
    addToggle (dbl, "dbl_on", "Double");
    addSlider (dbl, "dbl_mix", "Dbl Mix");
    addToggle (dbl, "chor_on", "Chorus");
    addSlider (dbl, "chor_mix", "Cho Mix");

    auto& st = newSection ("STEREO / OUTPUT");
    addToggle (st, "st_on", "Width On");
    addSlider (st, "st_width", "Width");
    addToggle (st, "lim_on", "Limiter");
    addSlider (st, "lim_ceiling", "Ceiling");
    addSlider (st, "out_gain", "Output");
}

MainPanel::~MainPanel() = default;

void MainPanel::buildPresetBar()
{
    presetBox.addItemList (SmartPreset::getFactoryPresetNames(), 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const auto name = presetBox.getText();
        SmartPreset::applyFactoryPreset (processor.getAPVTS(), name);
    };
    addAndMakeVisible (presetBox);

    savePresetBtn.onClick = [this]
    {
        auto dir = SmartPreset::getUserPresetDirectory();
        auto file = dir.getChildFile ("UserPreset.vocalpreset");
        SmartPreset::saveUserPreset (processor.getAPVTS(), file);
    };
    addAndMakeVisible (savePresetBtn);

    autoPresetBtn.onClick = [this]
    {
        auto snap = processor.getAnalyzer().getSnapshot();
        SmartPreset::AnalysisInput in;
        in.genre = snap.genreGuess; in.gender = snap.genderGuess;
        in.bpm = snap.bpm; in.crestDb = snap.crestDb;
        in.sibilanceDb = snap.sibilanceDb; in.noiseFloorDb = snap.noiseFloorDb;
        in.brightness = snap.brightness;
        SmartPreset::applyAutoPreset (processor.getAPVTS(), in);
    };
    addAndMakeVisible (autoPresetBtn);
}

MainPanel::Section& MainPanel::newSection (const juce::String& title)
{
    sections.push_back ({});
    sections.back().title = title;
    return sections.back();
}

void MainPanel::addSlider (Section& s, const juce::String& paramID, const juce::String& name)
{
    Control c;
    auto slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                  juce::Slider::TextBoxBelow);
    slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
    addAndMakeVisible (*slider);
    c.sAtt = std::make_unique<SliderAtt> (processor.getAPVTS(), paramID, *slider);

    auto label = std::make_unique<juce::Label> (juce::String(), name);
    label->setJustificationType (juce::Justification::centred);
    label->setFont (juce::Font (11.0f));
    addAndMakeVisible (*label);

    c.comp = std::move (slider);
    c.label = std::move (label);
    controls.push_back (std::move (c));
    s.controlIndices.push_back ((int) controls.size() - 1);
}

void MainPanel::addToggle (Section& s, const juce::String& paramID, const juce::String& name)
{
    Control c;
    auto btn = std::make_unique<juce::ToggleButton> (name);
    addAndMakeVisible (*btn);
    c.bAtt = std::make_unique<ButtonAtt> (processor.getAPVTS(), paramID, *btn);
    c.comp = std::move (btn);
    controls.push_back (std::move (c));
    s.controlIndices.push_back ((int) controls.size() - 1);
}

void MainPanel::addCombo (Section& s, const juce::String& paramID, const juce::String& name,
                          const juce::StringArray& choices)
{
    Control c;
    auto box = std::make_unique<juce::ComboBox>();
    box->addItemList (choices, 1);
    addAndMakeVisible (*box);
    c.cAtt = std::make_unique<ComboAtt> (processor.getAPVTS(), paramID, *box);

    auto label = std::make_unique<juce::Label> (juce::String(), name);
    label->setJustificationType (juce::Justification::centred);
    label->setFont (juce::Font (11.0f));
    addAndMakeVisible (*label);

    c.comp = std::move (box);
    c.label = std::move (label);
    controls.push_back (std::move (c));
    s.controlIndices.push_back ((int) controls.size() - 1);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (NeonLookAndFeel::background);
    for (auto& s : sections)
    {
        g.setColour (NeonLookAndFeel::panel);
        g.fillRoundedRectangle (s.bounds.toFloat(), 8.0f);
        g.setColour (NeonLookAndFeel::neonCyan.withAlpha (0.35f));
        g.drawRoundedRectangle (s.bounds.toFloat(), 8.0f, 1.0f);
        g.setColour (NeonLookAndFeel::neonCyan);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (s.title, s.bounds.removeFromTop (18).reduced (8, 0),
                    juce::Justification::centredLeft);
    }
}

void MainPanel::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto top = area.removeFromTop (28);
    presetBox.setBounds (top.removeFromLeft (220));
    top.removeFromLeft (8);
    autoPresetBtn.setBounds (top.removeFromLeft (80));
    top.removeFromLeft (6);
    savePresetBtn.setBounds (top.removeFromLeft (70));

    area.removeFromTop (8);

    // Grid layout of sections (2 columns).
    const int cols = 2;
    const int gap = 8;
    const int colW = (area.getWidth() - gap) / cols;
    const int rowH = 150;

    int col = 0, row = 0;
    for (auto& s : sections)
    {
        juce::Rectangle<int> sb (area.getX() + col * (colW + gap),
                                 area.getY() + row * (rowH + gap),
                                 colW, rowH);
        s.bounds = sb;

        auto inner = sb.reduced (8);
        inner.removeFromTop (20);

        // lay out controls in a wrapping flow
        const int cellW = 72, cellH = 60;
        int cx = inner.getX(), cy = inner.getY();
        for (int idx : s.controlIndices)
        {
            auto& c = controls[(size_t) idx];
            if (cx + cellW > inner.getRight())
            {
                cx = inner.getX();
                cy += cellH;
            }
            juce::Rectangle<int> cell (cx, cy, cellW, cellH);
            if (dynamic_cast<juce::ToggleButton*> (c.comp.get()) != nullptr)
            {
                c.comp->setBounds (cell.reduced (2).removeFromTop (24));
            }
            else if (dynamic_cast<juce::ComboBox*> (c.comp.get()) != nullptr)
            {
                if (c.label) c.label->setBounds (cell.removeFromTop (14));
                c.comp->setBounds (cell.removeFromTop (22).reduced (2, 0));
            }
            else
            {
                auto cc = cell;
                if (c.label) c.label->setBounds (cc.removeFromTop (12));
                c.comp->setBounds (cc);
            }
            cx += cellW;
        }

        if (++col >= cols) { col = 0; ++row; }
    }
}
