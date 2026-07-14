#pragma once

#include <JuceHeader.h>

/**
    AIGate — an automatic noise gate.

    Uses an envelope follower with independent attack / hold / release and a
    soft knee. When "auto" mode is enabled the threshold tracks the analysed
    noise floor so the gate opens only for real vocal content.
*/
class AIGate
{
public:
    AIGate() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float thresholdDb, float rangeDb,
                        float attackMs, float holdMs, float releaseMs,
                        bool autoMode);

    /** Auto mode uses this to place the threshold above the noise floor. */
    void setNoiseFloorDb (float dB) noexcept { noiseFloorDb = dB; }

    float getGainReductionDb() const noexcept { return currentGrDb; }

private:
    double sr = 44100.0;
    bool   on = true;
    bool   autoM = true;
    float  thresh = -45.0f;
    float  range  = -60.0f;   // attenuation when closed
    float  attCoef = 0.0f, relCoef = 0.0f;
    float  holdSamples = 0.0f;
    float  noiseFloorDb = -90.0f;

    float  env = 0.0f;
    float  gain = 0.0f;
    int    holdCounter = 0;
    float  currentGrDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIGate)
};
