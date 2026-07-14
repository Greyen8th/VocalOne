#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    FinalLimiter — look-ahead brickwall limiter with an integrated LUFS-ish
    loudness estimate for the output stage. Guarantees the output never exceeds
    the chosen ceiling.
*/
class FinalLimiter
{
public:
    FinalLimiter() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float ceilingDb, float releaseMs,
                        float inputGainDb);

    float getGainReductionDb() const noexcept { return grDb; }
    float getOutputLufs()      const noexcept { return lufs; }

private:
    double sr = 44100.0;
    bool   on = true;
    float  ceiling = 0.891f;    // -1 dBFS linear
    float  inputGain = 1.0f;
    float  relCoef = 0.0f;

    int    lookaheadSamples = 0;
    std::vector<std::vector<float>> delayBuf;
    int    writePos = 0;

    float  envGain = 1.0f;
    float  grDb = 0.0f;

    // loudness meter (mean-square integrator with K-weight approx)
    juce::dsp::IIR::Filter<float> kWeightL, kWeightR;
    float  msAccum = 0.0f;
    int    msCount = 0;
    float  lufs = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FinalLimiter)
};
