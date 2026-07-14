#pragma once

#include <JuceHeader.h>

#include "DSP/AIAnalyzer.h"
#include "DSP/AIGate.h"
#include "DSP/NoiseRemoval.h"
#include "DSP/AIDeEsser.h"
#include "DSP/SmartTune.h"
#include "DSP/AIEqualizer.h"
#include "DSP/AICompressor.h"
#include "DSP/Saturation.h"
#include "DSP/HarmonicExciter.h"
#include "DSP/FreshAir.h"
#include "DSP/Delay.h"
#include "DSP/Reverb.h"
#include "DSP/Doubler.h"
#include "DSP/Chorus.h"
#include "DSP/StereoWidth.h"
#include "DSP/FinalLimiter.h"
#include "AI/AIAssistant.h"

/**
    VocalOneAudioProcessor

    Hosts the full vocal chain (17+ modules) and exposes every parameter through
    an AudioProcessorValueTreeState. The signal is processed strictly in the
    order defined by the product spec.
*/
class VocalOneAudioProcessor : public juce::AudioProcessor
{
public:
    VocalOneAudioProcessor();
    ~VocalOneAudioProcessor() override;

    // AudioProcessor -------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Access for the editor -------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    AIAnalyzer&   getAnalyzer()   noexcept { return analyzer; }
    AIAssistant&  getAssistant()  noexcept { return assistant; }
    AIEqualizer&  getEqualizer()  noexcept { return eq; }
    SmartTune&    getSmartTune()  noexcept { return smartTune; }
    FinalLimiter& getLimiter()    noexcept { return limiter; }
    AICompressor& getCompressor() noexcept { return compressor; }

    /** Build a human-readable analysis context string for the AI assistant. */
    juce::String buildAnalysisContext() const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- Lock-free scope capture for the visualisers ----------------------
    static constexpr int scopeSize = 2048;
    struct ScopeBuffer
    {
        std::array<float, scopeSize> left  {};
        std::array<float, scopeSize> right {};
        std::atomic<int> writePos { 0 };
    };
    const ScopeBuffer& getScopeBuffer() const noexcept { return scope; }
    float getPitchHistoryHz() const noexcept { return analyzer.getCurrentF0(); }

private:
    void syncParametersToModules();

    juce::AudioProcessorValueTreeState apvts;

    // The chain modules (declared in processing order).
    AIAnalyzer      analyzer;
    AIGate          gate;
    NoiseRemoval    noiseRemoval;
    AIDeEsser       deEsser;
    SmartTune       smartTune;
    AIEqualizer     eq;
    AICompressor    compressor;
    Saturation      saturation;
    HarmonicExciter exciter;
    FreshAir        freshAir;
    Delay           delay;
    VocalReverb     reverb;
    Doubler         doubler;
    Chorus          chorus;
    StereoWidth     stereoWidth;
    FinalLimiter    limiter;

    AIAssistant     assistant;

    double currentSampleRate = 44100.0;
    juce::SmoothedValue<float> inGainSmoothed, outGainSmoothed;

    ScopeBuffer scope;
    void pushToScope (const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalOneAudioProcessor)
};
