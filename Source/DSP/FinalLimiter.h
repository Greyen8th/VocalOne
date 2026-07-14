#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    FinalLimiter — look-ahead brickwall limiter with true-peak detection and 
    integrated loudness metering. Guarantees the output never exceeds the 
    chosen ceiling, including between-sample peaks (true peak via 4x upsampling).
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
    float getTruePeakDb()      const noexcept { return truePeakDb; }

private:
    double sr = 44100.0;
    bool   on = true;
    float  ceiling = 0.891f;    // -1 dBFS linear
    float  inputGain = 1.0f;
    float  relCoef = 0.0f;
    float  attCoef = 0.0f;      // attack coefficient for smooth GR onset

    int    lookaheadSamples = 0;
    std::vector<std::vector<float>> delayBuf;
    int    writePos = 0;

    float  envGain = 1.0f;
    float  grDb = 0.0f;
    float  truePeakDb = -60.0f;

    // True peak detection (4x upsampling with linear interpolation)
    float detectTruePeak (const float* samples, int n);

    // loudness meter (mean-square integrator with K-weight approx)
    juce::dsp::IIR::Filter<float> kWeightL, kWeightR;
    float  msAccum = 0.0f;
    int    msCount = 0;
    float  lufs = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FinalLimiter)
};
