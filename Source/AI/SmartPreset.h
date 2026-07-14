#pragma once

#include <JuceHeader.h>

/**
    SmartPreset — factory + user preset manager, plus an "AI auto-preset"
    generator that maps an AIAnalyzer snapshot to a full set of parameter
    values (genre-aware starting points).
*/
class SmartPreset
{
public:
    struct AnalysisInput
    {
        int   genre = 0;      // AIAnalyzer::genreGuess
        int   gender = 0;
        float bpm = 120.0f;
        float crestDb = 12.0f;
        float sibilanceDb = -40.0f;
        float noiseFloorDb = -70.0f;
        float brightness = 0.5f;
    };

    SmartPreset() = default;

    /** Populate an APVTS with a genre-aware auto preset derived from analysis. */
    static void applyAutoPreset (juce::AudioProcessorValueTreeState& apvts,
                                 const AnalysisInput& in);

    /** Return the list of built-in factory preset names. */
    static juce::StringArray getFactoryPresetNames();

    /** Apply a named factory preset. */
    static void applyFactoryPreset (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& name);

    /** Save the current APVTS state to a .vocalpreset file. */
    static bool saveUserPreset (juce::AudioProcessorValueTreeState& apvts,
                                const juce::File& file);

    /** Load a .vocalpreset file into the APVTS. */
    static bool loadUserPreset (juce::AudioProcessorValueTreeState& apvts,
                                const juce::File& file);

    static juce::File getUserPresetDirectory();

private:
    static void setParam (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& id, float value);
};
