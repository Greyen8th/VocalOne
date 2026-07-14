#pragma once

#include <JuceHeader.h>

/**
    AIDeEsser — split-band de-esser.

    A linkwitz-style high band is extracted, compressed against a threshold and
    recombined. In auto mode the threshold is derived from the analysed
    sibilance energy so it adapts to the singer.
*/
class AIDeEsser
{
public:
    AIDeEsser() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float freqHz, float thresholdDb,
                        float ratio, bool autoMode);
    void setSibilanceDb (float dB) noexcept { sibilanceDb = dB; }

    float getGainReductionDb() const noexcept { return grDb; }

private:
    double sr = 44100.0;
    bool   on = true;
    bool   autoM = true;
    float  freq = 6500.0f;
    float  thresh = -30.0f;
    float  ratio = 4.0f;
    float  sibilanceDb = -90.0f;
    float  grDb = 0.0f;

    // per-channel high-pass state (one-pole -> use TPT SVF from juce::dsp)
    juce::dsp::LinkwitzRileyFilter<float> splitter;
    juce::AudioBuffer<float> highBand;

    float envDb = -90.0f;
    float attCoef = 0.0f, relCoef = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIDeEsser)
};
