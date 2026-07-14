#include "AIGate.h"

void AIGate::prepare (double sampleRate, int /*maxBlock*/, int /*numChannels*/)
{
    sr = sampleRate;
    setParameters (on, thresh, range, 1.0f, 10.0f, 100.0f, autoM);
    reset();
}

void AIGate::reset()
{
    env = 0.0f;
    gain = 0.0f;
    holdCounter = 0;
    currentGrDb = 0.0f;
}

void AIGate::setParameters (bool enabled, float thresholdDb, float rangeDb,
                            float attackMs, float holdMs, float releaseMs,
                            bool autoMode)
{
    on     = enabled;
    thresh = thresholdDb;
    range  = rangeDb;
    autoM  = autoMode;

    attCoef = std::exp (-1.0f / (juce::jmax (0.1f, attackMs)  * 0.001f * (float) sr));
    relCoef = std::exp (-1.0f / (juce::jmax (1.0f, releaseMs) * 0.001f * (float) sr));
    holdSamples = juce::jmax (0.0f, holdMs) * 0.001f * (float) sr;
}

void AIGate::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
    {
        currentGrDb = 0.0f;
        return;
    }

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    // Effective threshold: in auto mode place ~9 dB above the noise floor.
    const float effThreshDb = autoM ? juce::jlimit (-80.0f, -20.0f, noiseFloorDb + 9.0f)
                                    : thresh;
    const float threshLin = juce::Decibels::decibelsToGain (effThreshDb);
    const float floorGain  = juce::Decibels::decibelsToGain (range);

    float maxGr = 0.0f;

    for (int i = 0; i < numSmp; ++i)
    {
        // side-chain detector = max across channels
        float in = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            in = juce::jmax (in, std::abs (buffer.getReadPointer (ch)[i]));

        // peak envelope
        env = juce::jmax (in, env * 0.999f);

        float target;
        if (env >= threshLin)
        {
            target = 1.0f;
            holdCounter = (int) holdSamples;
        }
        else if (holdCounter > 0)
        {
            target = 1.0f;
            --holdCounter;
        }
        else
        {
            target = floorGain;
        }

        const float coef = (target > gain) ? attCoef : relCoef;
        gain = target + (gain - target) * coef;

        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= gain;

        maxGr = juce::jmax (maxGr, 1.0f - gain);
    }

    currentGrDb = juce::Decibels::gainToDecibels (juce::jmax (1.0e-4f, 1.0f - maxGr));
}
